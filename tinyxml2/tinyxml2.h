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

#include "makeplusplus.hpp"

#if defined( _DEBUG ) || defined (__DEBUG__)
#   ifndef TINYXML2_DEBUG
#       define TINYXML2_DEBUG
#   endif
#endif

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

#if !defined(TIXMLASSERT)
#if defined(TINYXML2_DEBUG)
#   if defined(_MSC_VER)
#       // "(void)0," is for suppressing C4127 warning in "assert(false)", "assert(true)" and the like
#       define TIXMLASSERT( x )           do { if ( !((void)0,(x))) { __debugbreak(); } } while(false)
#   elif defined (ANDROID_NDK)
#       include <android/log.h>
#       define TIXMLASSERT( x )           do { if ( !(x)) { __android_log_assert( "assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__ ); } } while(false)
#   else
#       include <assert.h>
#       define TIXMLASSERT                assert
#   endif
#else
#   define TIXMLASSERT( x )               do {} while(false)
#endif
#endif

/* Versioning, past 1.0.14:
	http://semver.org/
*/
static const int TIXML2_MAJOR_VERSION = 11;
static const int TIXML2_MINOR_VERSION = 0;
static const int TIXML2_PATCH_VERSION = 0;

#define TINYXML2_MAJOR_VERSION 11
#define TINYXML2_MINOR_VERSION 0
#define TINYXML2_PATCH_VERSION 0

// A fixed element depth limit is problematic. There needs to be a
// limit to avoid a stack overflow. However, that limit varies per
// system, and the capacity of the stack. On the other hand, it's a trivial
// attack that can result from ill, malicious, or even correctly formed XML,
// so there needs to be a limit in place.
static const int TINYXML2_MAX_ELEMENT_DEPTH = 500;

namespace msvc_xml
{
class document;
class element;
class XMLAttribute;
class XMLComment;
class XMLText;
class XMLDeclaration;
class XMLPrinter;

class StrPair {
public:
    enum Mode {
        NEEDS_ENTITY_PROCESSING			= 0x01,
        NEEDS_NEWLINE_NORMALIZATION		= 0x02,
        NEEDS_WHITESPACE_COLLAPSING     = 0x04,

        TEXT_ELEMENT		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
        TEXT_ELEMENT_LEAVE_ENTITIES		= NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_NAME		            = 0,
        ATTRIBUTE_VALUE		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_VALUE_LEAVE_ENTITIES  = NEEDS_NEWLINE_NORMALIZATION,
        COMMENT							= NEEDS_NEWLINE_NORMALIZATION
    };

    StrPair() : _flags( 0 ), _start( 0 ), _end( 0 ) {}
    ~StrPair();

    void Set( char* start, char* end, int flags ) {
        TIXMLASSERT( start );
        TIXMLASSERT( end );
        Reset();
        _start  = start;
        _end    = end;
        _flags  = flags | NEEDS_FLUSH;
    }

    const char* GetStr();

    bool Empty() const {
        return _start == _end;
    }

    void SetInternedStr( const char* str ) {
        Reset();
        _start = const_cast<char*>(str);
    }

    void SetStr( const char* str, int flags=0 );

    void TransferTo( StrPair* other );
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
        TIXMLASSERT( _size < INT_MAX );
        EnsureCapacity( _size+1 );
        _mem[_size] = t;
        ++_size;
    }

    T* PushArr( size_t count ) {
        TIXMLASSERT( _size <= SIZE_MAX - count );
        EnsureCapacity( _size+count );
        T* ret = &_mem[_size];
        _size += count;
        return ret;
    }

    T Pop() {
        TIXMLASSERT( _size > 0 );
        --_size;
        return _mem[_size];
    }

    void PopArr( size_t count ) {
        TIXMLASSERT( _size >= count );
        _size -= count;
    }

    bool Empty() const					{
        return _size == 0;
    }

    T& operator[](size_t i) {
        TIXMLASSERT( i < _size );
        return _mem[i];
    }

    const T& operator[](size_t i) const {
        TIXMLASSERT( i < _size );
        return _mem[i];
    }

    const T& PeekTop() const            {
        TIXMLASSERT( _size > 0 );
        return _mem[ _size - 1];
    }

    size_t Size() const {
        return _size;
    }

    size_t Capacity() const {
        TIXMLASSERT( _allocated >= INITIAL_SIZE );
        return _allocated;
    }

	void SwapRemove(size_t i) {
		TIXMLASSERT(i < _size);
		TIXMLASSERT(_size > 0);
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
        TIXMLASSERT( cap > 0 );
        if ( cap > _allocated ) {
            TIXMLASSERT( cap <= SIZE_MAX / 2 / sizeof(T));
            const size_t newAllocated = cap * 2;
            T* newMem = new T[newAllocated];
            TIXMLASSERT( newAllocated >= _size );
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


class MemPool
{
public:
    MemPool() {}
    virtual ~MemPool() {}

    virtual size_t ItemSize() const = 0;
    virtual void* Alloc() = 0;
    virtual void Free( void* ) = 0;
    virtual void SetTracked() = 0;
};


template< size_t ITEM_SIZE >
class MemPoolT : public MemPool
{
public:
    MemPoolT() : _blockPtrs(), _root(0), _currentAllocs(0), _nAllocs(0), _maxAllocs(0), _nUntracked(0)	{}
    ~MemPoolT() {
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

    virtual size_t ItemSize() const override {
        return ITEM_SIZE;
    }
    size_t CurrentAllocs() const {
        return _currentAllocs;
    }

    virtual void* Alloc() override{
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
        TIXMLASSERT( result != 0 );
        _root = _root->next;

        ++_currentAllocs;
        if ( _currentAllocs > _maxAllocs ) {
            _maxAllocs = _currentAllocs;
        }
        ++_nAllocs;
        ++_nUntracked;
        return result;
    }

    virtual void Free( void* mem ) override {
        if ( !mem ) {
            return;
        }
        --_currentAllocs;
        Item* item = static_cast<Item*>( mem );
#ifdef TINYXML2_DEBUG
        memset( item, 0xfe, sizeof( *item ) );
#endif
        item->next = _root;
        _root = item;
    }
    void Trace( const char* name ) {
        printf( "Mempool %s watermark=%d [%dk] current=%d size=%d nAlloc=%d blocks=%d\n",
                name, _maxAllocs, _maxAllocs * ITEM_SIZE / 1024, _currentAllocs,
                ITEM_SIZE, _nAllocs, _blockPtrs.Size() );
    }

    void SetTracked() override {
        --_nUntracked;
    }

    size_t Untracked() const {
        return _nUntracked;
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



class XMLVisitor
{
public:
    virtual ~XMLVisitor() {}

    /// Visit a document.
    virtual bool VisitEnter( const document& /*doc*/ )			{
        return true;
    }
    /// Visit a document.
    virtual bool VisitExit( const document& /*doc*/ )			{
        return true;
    }

    /// Visit an element.
    virtual bool VisitEnter( const element& /*element*/, const XMLAttribute* /*firstAttribute*/ )	{
        return true;
    }
    /// Visit an element.
    virtual bool VisitExit( const element& /*element*/ )			{
        return true;
    }

    /// Visit a declaration.
    virtual bool Visit( const XMLDeclaration& /*declaration*/ )		{
        return true;
    }
    /// Visit a text node.
    virtual bool Visit( const XMLText& /*text*/ )					{
        return true;
    }
    /// Visit a comment node.
    virtual bool Visit( const XMLComment& /*comment*/ )				{
        return true;
    }
};

// WARNING: must match XMLDocument::_errorNames[]
enum class error_t : std::uint32_t {
    success = 0,
    no_attrib,
    wrong_attrib_type,
    file_not_found,
    file_could_not_be_opened,
    file_read_error,
    error_parsing_element,
    error_parsing_attribute,
    error_parsing_text,
    error_parsing_cdata,
    error_parsing_comment,
    error_parsing_declaration,
    error_empty_document,
    error_mismatched_element,
    error_parsing,
    can_not_convert_text,
    no_text_node,
	depth_exceeded,

	error_count
};


class XMLUtil
{
public:

    inline static bool StringEqual( const char* p, const char* q)  {
        if ( p == q ) {
            return true;
        }
        TIXMLASSERT( p );
        TIXMLASSERT( q );
        return strcmp( p, q) == 0;
    }

    static const char* GetCharacterRef( const char* p, char* value, int* length );
    static void        ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );

};
	
class XMLNode
{
    friend class document;
    friend class element;
public:

    const document* GetDocument() const	{
        TIXMLASSERT( _document );
        return _document;
    }
    document* GetDocument()				{
        TIXMLASSERT( _document );
        return _document;
    }

    int ChildElementCount(const char *value) const;
    int ChildElementCount() const;

    const char* Value() const;

    void SetValue( const char* val, bool staticMem=false );

    int GetLineNum() const { return _parseLineNum; }

    const XMLNode*	Parent() const			{
        return _parent;
    }

    XMLNode*  Parent()						{
        return _parent;
    }

	template <class Ty>
	Ty*       ParentAs() {
    	return dynamic_cast<Ty*>(this->Parent());
    }

	template <class Ty>
	const Ty* ParentAs() const {
    	return dynamic_cast<const Ty*>(this->Parent());
    }

	element*       parent_elem()       { return ParentAs<element>(); }
	const element* parent_elem() const { return ParentAs<element>(); }

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
    virtual bool        Accept( XMLVisitor* visitor ) const = 0;

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
    virtual bool Accept( XMLVisitor* visitor ) const override;
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

    virtual bool Accept( XMLVisitor* visitor ) const override;

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

    virtual bool Accept( XMLVisitor* visitor ) const override;

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

    /// Gets the line number the attribute is in, if the document was parsed from a file.
    int GetLineNum() const { return _parseLineNum; }

    /// The next attribute in the list.
    const XMLAttribute* Next() const {
        return _next;
    }

    /// Set the attribute to a string value.
    void SetAttribute( const char* value );

private:
    enum { BUF_SIZE = 200 };

    XMLAttribute() : _name(), _value(),_parseLineNum( 0 ), _next( 0 ), _memPool( 0 ) {}
    virtual ~XMLAttribute() = default;

    void SetName( const char* name );

    mutable StrPair _name;
    mutable StrPair _value;
    int             _parseLineNum;
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
    void SetName( const char* str, bool staticMem=false )	{
        SetValue( str, staticMem );
    }
	
    virtual bool Accept( XMLVisitor* visitor ) const override;

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
    
    const char* GetText() const;
    
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


enum Whitespace {
    PRESERVE_WHITESPACE,
    COLLAPSE_WHITESPACE,
    PEDANTIC_WHITESPACE
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
    document( bool processEntities = true, Whitespace whitespaceMode = PRESERVE_WHITESPACE );
    ~document();
	

    void   SaveFile( const char* filename, bool compact = false );
    void   SaveFile(std::ofstream& f, bool compact = false );

    bool ProcessEntities() const		{
        return _processEntities;
    }
    Whitespace WhitespaceMode() const	{
        return _whitespaceMode;
    }
    

    /** Return the root element of DOM. Equivalent to FirstChildElement().
        To get the first node, use FirstChild().
    */
    element* root_elem()				{
        return begin_child_elem();
    }
    const element* root_elem() const	{
        return begin_child_elem();
    }
    
    void Print( XMLPrinter* streamer) const;
    virtual bool Accept( XMLVisitor* visitor ) const override;
    
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
    document( const document& );	// not supported
    void operator=( const document& );	// not supported

    bool			_processEntities;
    Whitespace		_whitespaceMode;
    char*			_charBuffer;
	
	// Memory tracking does add some overhead.
	// However, the code assumes that you don't
	// have a bunch of unlinked nodes around.
	// Therefore it takes less memory to track
	// in the document vs. a linked list in the XMLNode,
	// and the performance is the same.
	DynArray<XMLNode*, 10> _unlinked;

    MemPoolT< sizeof(element) >	 _elementPool;
    MemPoolT< sizeof(XMLAttribute) > _attributePool;
    MemPoolT< sizeof(XMLText) >		 _textPool;
    MemPoolT< sizeof(XMLComment) >	 _commentPool;

	static constexpr std::string_view _errorNames[static_cast<uint32_t>(error_t::error_count)] = {
		"XML_SUCCESS",
		"XML_NO_ATTRIBUTE",
		"XML_WRONG_ATTRIBUTE_TYPE",
		"XML_ERROR_FILE_NOT_FOUND",
		"XML_ERROR_FILE_COULD_NOT_BE_OPENED",
		"XML_ERROR_FILE_READ_ERROR",
		"XML_ERROR_PARSING_ELEMENT",
		"XML_ERROR_PARSING_ATTRIBUTE",
		"XML_ERROR_PARSING_TEXT",
		"XML_ERROR_PARSING_CDATA",
		"XML_ERROR_PARSING_COMMENT",
		"XML_ERROR_PARSING_DECLARATION",
		"XML_ERROR_EMPTY_DOCUMENT",
		"XML_ERROR_MISMATCHED_ELEMENT",
		"XML_ERROR_PARSING",
		"XML_CAN_NOT_CONVERT_TEXT",
		"XML_NO_TEXT_NODE",
		"XML_ELEMENT_DEPTH_EXCEEDED"
	};

    template<class NodeType, size_t PoolElementSize>
    NodeType* CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool );
};

template<class NodeType, size_t PoolElementSize>
inline NodeType* document::CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool )
{
    TIXMLASSERT( sizeof( NodeType ) == PoolElementSize );
    TIXMLASSERT( sizeof( NodeType ) == pool.ItemSize() );
    NodeType* returnNode = new (pool.Alloc()) NodeType( this );
    TIXMLASSERT( returnNode );
    returnNode->_memPool = &pool;

	_unlinked.Push(returnNode);
    return returnNode;
}
	
class XMLPrinter : public XMLVisitor
{
public:

    XMLPrinter(std::ostream& f, bool compact = false, int depth = 0);
    ~XMLPrinter() override = default;

    /** If streaming, start writing an element.
        The element must be closed with CloseElement()
    */
    void OpenElement( const char* name, bool compactMode=false );
    /// If streaming, add an attribute to an open element.
    void PushAttribute( const char* name, const char* value );
    /// If streaming, close the Element.
    virtual void CloseElement( bool compactMode=false );

    /// Add a text node.
    void PushText( const char* text);

    /// Add a comment
    void PushComment( const char* comment );

    void PushDeclaration( const char* value );

    virtual bool VisitEnter( const document& /*doc*/ ) override;
    virtual bool VisitExit( const document& /*doc*/ ) override	{
        return true;
    }

    virtual bool VisitEnter( const element& elem, const XMLAttribute* attribute ) override;
    virtual bool VisitExit( const element& element ) override;

    virtual bool Visit( const XMLText& text ) override;
    virtual bool Visit( const XMLComment& comment ) override;
    virtual bool Visit( const XMLDeclaration& declaration ) override;

protected:
	bool CompactMode( const element& )	{ return _compactMode; }

    inline void PrintSpace( int depth ) {
	    for( int i=0; i<depth; ++i ) {
	        Write( "  " );
	    }
	}
    
    inline void Write( const char* data, size_t size ) {
        _fp->write(data, static_cast<std::streamsize>(size));
    }
    
    inline void Putc( char ch ) {
        _fp->put(ch);
    }

    inline void Write(const char* data) { Write(data, strlen(data)); }

    void SealElementIfJustOpened();
    bool _elementJustOpened;
    DynArray< const char*, 10 > _stack;

private:
    void PrepareForNewNode( bool compactMode );
    void PrintString( const char*, bool restrictedEntitySet );	// prints out, after detecting entities.

    bool             _firstElement;
	std::ostream*    _fp;
    int              _depth;
    int              _textDepth;
    bool             _processEntities;
	bool             _compactMode;

    enum {
        ENTITY_RANGE = 64,
        BUF_SIZE = 200
    };
    bool _entityFlag[ENTITY_RANGE];
    bool _restrictedEntityFlag[ENTITY_RANGE];

};


} // namespace tinyxml2

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif // TINYXML2_INCLUDED