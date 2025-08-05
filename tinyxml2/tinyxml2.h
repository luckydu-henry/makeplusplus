/*
Original code by Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef TINYXML2_INCLUDED
#define TINYXML2_INCLUDED

#include <format>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <limits>
#include <variant>

#include "makeplusplus.hpp"

// A fixed element depth limit is problematic. There needs to be a
// limit to avoid a stack overflow. However, that limit varies per
// system, and the capacity of the stack. On the other hand, it's a trivial
// attack that can result from ill, malicious, or even correctly formed XML,
// so there needs to be a limit in place.
static const int TINYXML2_MAX_ELEMENT_DEPTH = 500;

namespace msvc_xml {
    
class document;
class element;
class printer;
    
class XMLAttribute;
class XMLComment;
class XMLText;
class XMLDeclaration;

class StrPair {
public:
    enum Mode {
        NEEDS_NEWLINE_NORMALIZATION		= 0x02,
        NEEDS_WHITESPACE_COLLAPSING     = 0x04,

        TEXT_ELEMENT		            = NEEDS_NEWLINE_NORMALIZATION,
        TEXT_ELEMENT_LEAVE_ENTITIES		= NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_NAME		            = 0,
        ATTRIBUTE_VALUE		            = NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_VALUE_LEAVE_ENTITIES  = NEEDS_NEWLINE_NORMALIZATION,
        COMMENT							= NEEDS_NEWLINE_NORMALIZATION
    };

    StrPair() : _flags( 0 ), _start( 0 ), _end( 0 ) {}
    ~StrPair();

    void Set( char* start, char* end, int flags ) {
        Reset();
        _start  = start;
        _end    = end;
        _flags  = flags | NEEDS_FLUSH;
    }

    const char* GetStr();
    
    void SetStr( const char* str, int flags=0 );

	void Reset();

private:

    enum {
        NEEDS_FLUSH = 0x100,
        NEEDS_DELETE = 0x200
    };

    int     _flags;
    char*   _start;
    char*   _end;

};


template <class T, size_t INITIAL_SIZE>
class DynArray
{
public:
    DynArray() :
        _mem( _pool ),
        _allocated( INITIAL_SIZE ),
        _size( 0 )
    {
    }

    ~DynArray() {
        if ( _mem != _pool ) {
            delete [] _mem;
        }
    }

    void Clear() {
        _size = 0;
    }

    void Push( T t ) {
        EnsureCapacity( _size+1 );
        _mem[_size] = t;
        ++_size;
    }

    T* PushArr( size_t count ) {
        EnsureCapacity( _size+count );
        T* ret = &_mem[_size];
        _size += count;
        return ret;
    }

    T Pop() {
        --_size;
        return _mem[_size];
    }

    void PopArr( size_t count ) {
        _size -= count;
    }

    bool Empty() const					{
        return _size == 0;
    }

    T& operator[](size_t i) {
        return _mem[i];
    }

    const T& operator[](size_t i) const {
        return _mem[i];
    }

    const T& PeekTop() const            {
        return _mem[ _size - 1];
    }

    size_t Size() const {
        return _size;
    }

    size_t Capacity() const {
        return _allocated;
    }

	void SwapRemove(size_t i) {
		_mem[i] = _mem[_size - 1];
		--_size;
	}

    const T* Mem() const				{
        TIXMLASSERT( _mem );
        return _mem;
    }

    T* Mem() {
        TIXMLASSERT( _mem );
        return _mem;
    }

private:

    void EnsureCapacity( size_t cap ) {
        if ( cap > _allocated ) {
            const size_t newAllocated = cap * 2;
            T* newMem = new T[newAllocated];
            memcpy( newMem, _mem, sizeof(T) * _size );	// warning: not using constructors, only works for PODs
            if ( _mem != _pool ) {
                delete [] _mem;
            }
            _mem = newMem;
            _allocated = newAllocated;
        }
    }

    T*  _mem;
    T   _pool[INITIAL_SIZE];
    size_t _allocated;		// objects allocated
    size_t _size;			// number objects in use
};


class MemPool {
public:
    virtual void Free(void*) = 0;
    virtual void SetTracked() = 0;
    virtual ~MemPool() = default;
};
    
template< size_t ITEM_SIZE >
class MemPoolT : public MemPool {
public:
    MemPoolT() : _blockPtrs(), _root(0), _currentAllocs(0), _nAllocs(0), _maxAllocs(0), _nUntracked(0)	{}
    ~MemPoolT() override {
        MemPoolT< ITEM_SIZE >::Clear();
    }

    void Clear() {
        // Delete the blocks.
        while( !_blockPtrs.Empty()) {
            Block* lastBlock = _blockPtrs.Pop();
            delete lastBlock;
        }
        _root = 0;
        _currentAllocs = 0;
        _nAllocs = 0;
        _maxAllocs = 0;
        _nUntracked = 0;
    }

    size_t ItemSize() const {
        return ITEM_SIZE;
    }
    size_t CurrentAllocs() const {
        return _currentAllocs;
    }

    void* Alloc() {
        if ( !_root ) {
            // Need a new block.
            Block* block = new Block;
            _blockPtrs.Push( block );

            Item* blockItems = block->items;
            for( size_t i = 0; i < ITEMS_PER_BLOCK - 1; ++i ) {
                blockItems[i].next = &(blockItems[i + 1]);
            }
            blockItems[ITEMS_PER_BLOCK - 1].next = 0;
            _root = blockItems;
        }
        Item* const result = _root;
        _root = _root->next;

        ++_currentAllocs;
        if ( _currentAllocs > _maxAllocs ) {
            _maxAllocs = _currentAllocs;
        }
        ++_nAllocs;
        ++_nUntracked;
        return result;
    }

    void Free( void* mem ) override {
        if ( !mem ) {
            return;
        }
        --_currentAllocs;
        Item* item = static_cast<Item*>( mem );
        item->next = _root;
        _root = item;
    }

    void SetTracked() override {
        --_nUntracked;
    }

	// This number is perf sensitive. 4k seems like a good tradeoff on my machine.
	// The test file is large, 170k.
	// Release:		VS2010 gcc(no opt)
	//		1k:		4000
	//		2k:		4000
	//		4k:		3900	21000
	//		16k:	5200
	//		32k:	4300
	//		64k:	4000	21000
    // Declared public because some compilers do not accept to use ITEMS_PER_BLOCK
    // in private part if ITEMS_PER_BLOCK is private
    enum { ITEMS_PER_BLOCK = (4 * 1024) / ITEM_SIZE };

private:
    MemPoolT( const MemPoolT& ); // not supported
    void operator=( const MemPoolT& ); // not supported

    union Item {
        Item*   next;
        char    itemData[static_cast<size_t>(ITEM_SIZE)];
    };
    struct Block {
        Item items[ITEMS_PER_BLOCK];
    };
    DynArray< Block*, 10 > _blockPtrs;
    Item* _root;

    size_t _currentAllocs;
    size_t _nAllocs;
    size_t _maxAllocs;
    size_t _nUntracked;
};


class XMLUtil
{
public:
    inline static bool StringEqual( std::string_view p, std::string_view q)  {
        return p == q;
    }
};

class XMLNode
{
    friend class document;
    friend class element;
public:

    const document* GetDocument() const	{
        return _document;
    }
    document* GetDocument()				{
        return _document;
    }

    int ChildElementCount(const char *value) const;
    int ChildElementCount() const;

    const char* Value() const;

    void SetValue( const char* val);
    
	element*       parent_elem()       { return reinterpret_cast<element*>(_parent); }
	const element* parent_elem() const { return reinterpret_cast<const element*>(_parent); }

    bool            NoChildren() const					{
        return !_firstChild;
    }

    const XMLNode*  FirstChild() const		{
        return _firstChild;
    }

    XMLNode*		FirstChild()			{
        return _firstChild;
    }

    const element*      begin_child_elem( const char* name = 0 ) const;

    element*            begin_child_elem( const char* name = 0 )	{
        return const_cast<element*>(const_cast<const XMLNode*>(this)->begin_child_elem( name ));
    }

    const XMLNode*	    LastChild() const						{
        return _lastChild;
    }

    XMLNode*		    LastChild()								{
        return _lastChild;
    }

    const element*      LastChildElement( const char* name = 0 ) const;

    element* LastChildElement( const char* name = 0 )	{
        return const_cast<element*>(const_cast<const XMLNode*>(this)->LastChildElement(name) );
    }

    const XMLNode*	    PreviousSibling() const					{
        return _prev;
    }

    XMLNode*	        PreviousSibling()							{
        return _prev;
    }

    const element*   	PreviousSiblingElement( const char* name = 0 ) const ;

    element*	PreviousSiblingElement( const char* name = 0 ) {
        return const_cast<element*>(const_cast<const XMLNode*>(this)->PreviousSiblingElement( name ) );
    }

    const XMLNode*	    NextSibling() const						{
        return _next;
    }

    XMLNode*	        NextSibling()								{
        return _next;
    }

    const element*	    next_sibling_elem( const char* name = 0 ) const;

    element*	        next_sibling_elem( const char* name = 0 )	{
        return const_cast<element*>(const_cast<const XMLNode*>(this)->next_sibling_elem( name ) );
    }

    XMLNode*            insert_child_end( XMLNode* addThis );

    XMLNode*            LinkEndChild( XMLNode* addThis )	{
        return insert_child_end( addThis );
    }
    XMLNode*            InsertFirstChild( XMLNode* addThis );
    XMLNode*            InsertAfterChild( XMLNode* afterThis, XMLNode* addThis );
    void                DeleteChildren();
    void                DeleteChild( XMLNode* node );
    virtual XMLNode*    ShallowClone( document* document ) const = 0;
	XMLNode*            DeepClone( document* target ) const;
    virtual bool        ShallowEqual( const XMLNode* compare ) const = 0;
    virtual bool        Accept(printer* visitor) const = 0;

	/**
		Set user data into the XMLNode. TinyXML-2 in
		no way processes or interprets user data.
		It is initially 0.
	*/
	void SetUserData(void* userData)	{ _userData = userData; }

	/**
		Get user data set into the XMLNode. TinyXML-2 in
		no way processes or interprets user data.
		It is initially 0.
	*/
	void* GetUserData() const			{ return _userData; }

protected:
    explicit XMLNode( document* );
    virtual ~XMLNode();

    document*	_document;
    XMLNode*		_parent;
    mutable StrPair	_value;
    int             _parseLineNum;

    XMLNode*		_firstChild;
    XMLNode*		_lastChild;

    XMLNode*		_prev;
    XMLNode*		_next;

	void*			_userData;

private:
    MemPool*		_memPool;
    void Unlink( XMLNode* child );
    static void DeleteNode( XMLNode* node );
    void InsertChildPreamble( XMLNode* insertThis ) const;
    const element* ToElementWithName( const char* name ) const;
};
    
class XMLText : public XMLNode
{
    friend class document;
public:
    virtual bool Accept(printer* visitor ) const override;
    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

protected:
    explicit XMLText( document* doc )	: XMLNode( doc ) {}
    virtual ~XMLText() override = default;
};


/** An XML Comment. */
class XMLComment : public XMLNode
{
    friend class document;
public:

    virtual bool Accept( printer* visitor ) const override;

    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

protected:
    explicit XMLComment( document* doc ) : XMLNode( doc ) {};
    virtual ~XMLComment() override = default;
};


class XMLDeclaration : public XMLNode
{
    friend class document;
public:

    virtual bool Accept(  printer* visitor ) const override;

    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

protected:
    explicit XMLDeclaration( document* doc ) : XMLNode(doc) {}
    virtual ~XMLDeclaration() override = default;
};

class XMLAttribute
{
    friend class element;
public:
    /// The name of the attribute.
    const char* Name() const;

    /// The value of the attribute.
    const char* Value() const;

    /// The next attribute in the list.
    const XMLAttribute* Next() const {
        return _next;
    }

    /// Set the attribute to a string value.
    void SetAttribute( const char* value );

private:

    XMLAttribute() : _name(), _value() , _next( 0 ), _memPool( 0 ) {}
    virtual ~XMLAttribute() = default;

    void SetName( const char* name );

    mutable StrPair _name;
    mutable StrPair _value;
    
    XMLAttribute*   _next;
    MemPool*        _memPool;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class element : public XMLNode
{
    friend class document;
public:
    /// Get the name of an element (which is the Value() of the node.)
    const char* Name() const		{
        return Value();
    }
    /// Set the name of the element.
    void SetName( const char* str)	{
        SetValue(str);
    }
	
    virtual bool Accept(  printer* visitor ) const override;

    const char* get_attrib( const char* name, const char* value=0 ) const;
	

	/// Sets the named attribute to value.
    element* set_attrib( const char* name, const char* value )	{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    	return this;
    }

    /**
    	Delete an attribute.
    */
    void DeleteAttribute( const char* name );

    /// Return the first attribute in the list.
    const XMLAttribute* FirstAttribute() const {
        return _rootAttribute;
    }
    /// Query a specific attribute in the list.
    const XMLAttribute* find_attrib( const char* name ) const;
    
	element* set_txt( const char* inText );
	
    /**
        Convenience method to create a new XMLElement and add it as last (right)
        child of this node. Returns the created and inserted element.
    */
    element* insert_child_elem(const char* name);
    /// See InsertNewChildElement()
    XMLComment* insert_comm(const char* comment);
    /// See InsertNewChildElement()
    XMLText* InsertNewText(const char* text);
    /// See InsertNewChildElement()
    XMLDeclaration* InsertNewDeclaration(const char* text);
    /// See InsertNewChildElement()


    // internal:
    enum ElementClosingType {
        OPEN,		// <foo>
        CLOSED,		// <foo/>
        CLOSING		// </foo>
    };
    ElementClosingType ClosingType() const {
        return _closingType;
    }
    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

private:
    element( document* doc );
    ~element() override;

    XMLAttribute* FindOrCreateAttribute( const char* name );
    static void DeleteAttribute( XMLAttribute* attribute );
    XMLAttribute* CreateAttribute();

    enum { BUF_SIZE = 200 };
    ElementClosingType _closingType;
    // The attribute list is ordered; there is no 'lastAttribute'
    // because the list needs to be scanned for dupes before adding
    // a new attribute.
    XMLAttribute* _rootAttribute;
};

/** A Document binds together all the functionality.
	It can be saved, loaded, and printed to the screen.
	All Nodes are connected and allocated to a Document.
	If the Document is deleted, all its Nodes are also deleted.
*/
class document : public XMLNode
{
    friend class element;
    // Gives access to SetError and Push/PopDepth, but over-access for everything else.
    // Wishing C++ had "internal" scope.
    friend class XMLNode;
    friend class XMLText;
    friend class XMLComment;
    friend class XMLDeclaration;
    friend class XMLUnknown;
public:
    /// constructor
    document();
    ~document();

    void   SaveFile( const char* filename, bool compact = false );
    void   SaveFile(std::ofstream& f, bool compact = false );

    /** Return the root element of DOM. Equivalent to FirstChildElement().
        To get the first node, use FirstChild().
    */
    element* root_elem()				{
        return begin_child_elem();
    }
    const element* root_elem() const	{
        return begin_child_elem();
    }
    
    void Print( printer* streamer) const;
    virtual bool Accept(  printer* visitor ) const override;
    
    element* make_elem( const char* name );

    XMLComment* make_comm( const char* comment );

    XMLText* NewText( const char* text );

    XMLDeclaration* make_decl( const char* text=0 );
    
    void DeleteNode( XMLNode* node );

    /// Clear the document, resetting it to the initial state.
    void Clear();

	void DeepCopy(document* target) const;

	// internal
	void MarkInUse(const XMLNode* const);

    virtual XMLNode* ShallowClone( document* /*document*/ ) const override{
        return 0;
    }
    virtual bool ShallowEqual( const XMLNode* /*compare*/ ) const override{
        return false;
    }

private:

    char*			_charBuffer;
	
	// Memory tracking does add some overhead.
	// However, the code assumes that you don't
	// have a bunch of unlinked nodes around.
	// Therefore it takes less memory to track
	// in the document vs. a linked list in the XMLNode,
	// and the performance is the same.
	DynArray<XMLNode*, 10> _unlinked;

    MemPoolT<sizeof(element)>	     _elementPool;
    MemPoolT<sizeof(XMLAttribute)>   _attributePool;
    MemPoolT<sizeof(XMLText)>		 _textPool;
    MemPoolT<sizeof(XMLComment)>	 _commentPool;

    template<class NodeType, size_t PoolElementSize>
    NodeType* CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool );
};

template<class NodeType, size_t PoolElementSize>
inline NodeType* document::CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool )
{
    NodeType* returnNode = new (pool.Alloc()) NodeType( this );
    returnNode->_memPool = &pool;

	_unlinked.Push(returnNode);
    return returnNode;
}
	
class printer final {
public:

    printer(std::ostream& f, bool compact = false, int depth = 0);
    ~printer() = default;

    /** If streaming, start writing an element.
        The element must be closed with CloseElement()
    */
    void OpenElement( const char* name, bool compactMode=false );
    /// If streaming, add an attribute to an open element.
    void PushAttribute( const char* name, const char* value );
    /// If streaming, close the Element.
    void CloseElement( bool compactMode=false );

    /// Add a text node.
    void PushText       ( const char* text);
    void PushComment    ( const char* comment );
    void PushDeclaration( const char* value );
    
    bool VisitEnter( const document& /*doc*/ );
    
    bool VisitExit( const document& /*doc*/ ) {
        return true;
    }

    bool VisitEnter( const element& elem, const XMLAttribute* attribute );
    bool VisitExit( const element& element );

    bool Visit( const XMLText& text );
    bool Visit( const XMLComment& comment );
    bool Visit( const XMLDeclaration& declaration );

protected:
	bool CompactMode( const element& )	{ return _compactMode; }
    void SealElementIfJustOpened();

private:
    void PrepareForNewNode( bool compactMode );

    bool                           _firstElement;
	std::ostream*                  _fp;
    int                            _depth;
    int                            _textDepth;
	bool                           _compactMode;
    bool                           _elementJustOpened;
    DynArray< const char*, 10 >    _stack;
};


} // namespace tinyxml2

#endif // TINYXML2_INCLUDED