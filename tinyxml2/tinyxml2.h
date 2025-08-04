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
#include <cstdint>
#include <cstring>
#include <limits>

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

/*
	A class that wraps strings. Normally stores the start and end
	pointers into the XML file itself, and will apply normalization
	and entity translation if actually read. Can also store (and memory
	manage) a traditional char[]

    Isn't clear why is needed; but seems to fix #719
*/
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

    char* ParseText( char* in, const char* endTag, int strFlags, int* curLineNumPtr );
    char* ParseName( char* in );

    void TransferTo( StrPair* other );
	void Reset();

private:
    void CollapseWhitespace();

    enum {
        NEEDS_FLUSH = 0x100,
        NEEDS_DELETE = 0x200
    };

    int     _flags;
    char*   _start;
    char*   _end;

};


/*
	A dynamic array of Plain Old Data. Doesn't support constructors, etc.
	Has a small initial memory pool, so that low or no usage will not
	cause a call to new/delete
*/
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
    DynArray( const DynArray& ); // not supported
    void operator=( const DynArray& ); // not supported

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


/*
	Parent virtual class of a pool for fast allocation
	and deallocation of objects.
*/
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


/*
	Template child class to create pools of the correct type.
*/
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



/**
	Implements the interface to the "Visitor pattern" (see the Accept() method.)
	If you call the Accept() method, it requires being passed a XMLVisitor
	class to handle callbacks. For nodes that contain other nodes (Document, Element)
	you will get called with a VisitEnter/VisitExit pair. Nodes that are always leafs
	are simply called with Visit().

	If you return 'true' from a Visit method, recursive parsing will continue. If you return
	false, <b>no children of this node or its siblings</b> will be visited.

	All flavors of Visit methods have a default implementation that returns 'true' (continue
	visiting). You need to only override methods that are interesting to you.

	Generally Accept() is called on the XMLDocument, although all nodes support visiting.

	You should never change the document from a callback.

	@sa XMLNode::Accept()
*/
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


/*
	Utility functionality.
*/
class XMLUtil
{
public:
    static const char* SkipWhiteSpace( const char* p, int* curLineNumPtr )	{
        TIXMLASSERT( p );

        while( IsWhiteSpace(*p) ) {
            if (curLineNumPtr && *p == '\n') {
                ++(*curLineNumPtr);
            }
            ++p;
        }
        TIXMLASSERT( p );
        return p;
    }
    static char* SkipWhiteSpace( char* const p, int* curLineNumPtr ) {
        return const_cast<char*>( SkipWhiteSpace( const_cast<const char*>(p), curLineNumPtr ) );
    }

    // Anything in the high order range of UTF-8 is assumed to not be whitespace. This isn't
    // correct, but simple, and usually works.
    static bool IsWhiteSpace( char p )					{
        return !IsUTF8Continuation(p) && isspace( static_cast<unsigned char>(p) );
    }

    inline static bool IsNameStartChar( unsigned char ch ) {
        if ( ch >= 128 ) {
            // This is a heuristic guess in attempt to not implement Unicode-aware isalpha()
            return true;
        }
        if ( isalpha( ch ) ) {
            return true;
        }
        return ch == ':' || ch == '_';
    }

    inline static bool IsNameChar( unsigned char ch ) {
        return IsNameStartChar( ch )
               || isdigit( ch )
               || ch == '.'
               || ch == '-';
    }

    inline static bool IsPrefixHex( const char* p) {
        p = SkipWhiteSpace(p, 0);
        return p && *p == '0' && ( *(p + 1) == 'x' || *(p + 1) == 'X');
    }

    inline static bool StringEqual( const char* p, const char* q, int nChar=INT_MAX )  {
        if ( p == q ) {
            return true;
        }
        TIXMLASSERT( p );
        TIXMLASSERT( q );
        TIXMLASSERT( nChar >= 0 );
        return strncmp( p, q, static_cast<size_t>(nChar) ) == 0;
    }

    inline static bool IsUTF8Continuation( const char p ) {
        return ( p & 0x80 ) != 0;
    }

    static const char* ReadBOM( const char* p, bool* hasBOM );
    // p is the starting location,
    // the UTF-8 value of the entity will be placed in value, and length filled in.
    static const char* GetCharacterRef( const char* p, char* value, int* length );
    static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );

};


/** XMLNode is a base class for every object that is in the
	XML Document Object Model (DOM), except XMLAttributes.
	Nodes have siblings, a parent, and children which can
	be navigated. A node is always in a XMLDocument.
	The type of a XMLNode can be queried, and it can
	be cast to its more defined type.

	A XMLDocument allocates memory for all its Nodes.
	When the XMLDocument gets deleted, all its Nodes
	will also be deleted.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration( leaf )

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributes (not on tree)
							Comment (leaf)
							Unknown (leaf)

	@endverbatim
*/
class XMLNode
{
    friend class document;
    friend class element;
public:

    /// Get the XMLDocument that owns this XMLNode.
    const document* GetDocument() const	{
        TIXMLASSERT( _document );
        return _document;
    }
    /// Get the XMLDocument that owns this XMLNode.
    document* GetDocument()				{
        TIXMLASSERT( _document );
        return _document;
    }

    /// Safely cast to an Element, or null.
    virtual element*		ToElement()		{
        return 0;
    }
    /// Safely cast to Text, or null.
    virtual XMLText*		ToText()		{
        return 0;
    }
    /// Safely cast to a Comment, or null.
    virtual XMLComment*		ToComment()		{
        return 0;
    }
    /// Safely cast to a Document, or null.
    virtual document*	ToDocument()	{
        return 0;
    }
    /// Safely cast to a Declaration, or null.
    virtual XMLDeclaration*	ToDeclaration()	{
        return 0;
    }
    virtual const element*		ToElement() const		{
        return 0;
    }
    virtual const XMLText*			ToText() const			{
        return 0;
    }
    virtual const XMLComment*		ToComment() const		{
        return 0;
    }
    virtual const document*		ToDocument() const		{
        return 0;
    }
    virtual const XMLDeclaration*	ToDeclaration() const	{
        return 0;
    }

    // ChildElementCount was originally suggested by msteiger on the sourceforge page for TinyXML and modified by KB1SPH for TinyXML-2.

    int ChildElementCount(const char *value) const;

    int ChildElementCount() const;

    /** The meaning of 'value' changes for the specific type.
    	@verbatim
    	Document:	empty (NULL is returned, not an empty string)
    	Element:	name of the element
    	Comment:	the comment text
    	Unknown:	the tag contents
    	Text:		the text string
    	@endverbatim
    */
    const char* Value() const;

    /** Set the Value of an XML node.
    	@sa Value()
    */
    void SetValue( const char* val, bool staticMem=false );

    /// Gets the line number the node is in, if the document was parsed from a file.
    int GetLineNum() const { return _parseLineNum; }

    /// Get the parent of this node on the DOM.
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

    /// Returns true if this node has no children.
    bool NoChildren() const					{
        return !_firstChild;
    }

    /// Get the first child node, or null if none exists.
    const XMLNode*  FirstChild() const		{
        return _firstChild;
    }

    XMLNode*		FirstChild()			{
        return _firstChild;
    }

    /** Get the first child element, or optionally the first child
        element with the specified name.
    */
    const element* begin_child_elem( const char* name = 0 ) const;

    element* begin_child_elem( const char* name = 0 )	{
        return const_cast<element*>(const_cast<const XMLNode*>(this)->begin_child_elem( name ));
    }

    /// Get the last child node, or null if none exists.
    const XMLNode*	LastChild() const						{
        return _lastChild;
    }

    XMLNode*		LastChild()								{
        return _lastChild;
    }

    /** Get the last child element or optionally the last child
        element with the specified name.
    */
    const element* LastChildElement( const char* name = 0 ) const;

    element* LastChildElement( const char* name = 0 )	{
        return const_cast<element*>(const_cast<const XMLNode*>(this)->LastChildElement(name) );
    }

    /// Get the previous (left) sibling node of this node.
    const XMLNode*	PreviousSibling() const					{
        return _prev;
    }

    XMLNode*	PreviousSibling()							{
        return _prev;
    }

    /// Get the previous (left) sibling element of this node, with an optionally supplied name.
    const element*	PreviousSiblingElement( const char* name = 0 ) const ;

    element*	PreviousSiblingElement( const char* name = 0 ) {
        return const_cast<element*>(const_cast<const XMLNode*>(this)->PreviousSiblingElement( name ) );
    }

    /// Get the next (right) sibling node of this node.
    const XMLNode*	NextSibling() const						{
        return _next;
    }

    XMLNode*	NextSibling()								{
        return _next;
    }

    /// Get the next (right) sibling element of this node, with an optionally supplied name.
    const element*	next_sibling_elem( const char* name = 0 ) const;

    element*	next_sibling_elem( const char* name = 0 )	{
        return const_cast<element*>(const_cast<const XMLNode*>(this)->next_sibling_elem( name ) );
    }

    /**
    	Add a child node as the last (right) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
    */
    XMLNode* insert_child_end( XMLNode* addThis );

    XMLNode* LinkEndChild( XMLNode* addThis )	{
        return insert_child_end( addThis );
    }
    /**
    	Add a child node as the first (left) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
    */
    XMLNode* InsertFirstChild( XMLNode* addThis );
    /**
    	Add a node after the specified child node.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the afterThis node
		is not a child of this node, or if the node does not
		belong to the same document.
    */
    XMLNode* InsertAfterChild( XMLNode* afterThis, XMLNode* addThis );

    /**
    	Delete all the children of this node.
    */
    void DeleteChildren();

    /**
    	Delete a child of this node.
    */
    void DeleteChild( XMLNode* node );

    /**
    	Make a copy of this node, but not its children.
    	You may pass in a Document pointer that will be
    	the owner of the new Node. If the 'document' is
    	null, then the node returned will be allocated
    	from the current Document. (this->GetDocument())

    	Note: if called on a XMLDocument, this will return null.
    */
    virtual XMLNode* ShallowClone( document* document ) const = 0;

	/**
		Make a copy of this node and all its children.

		If the 'target' is null, then the nodes will
		be allocated in the current document. If 'target'
        is specified, the memory will be allocated in the
        specified XMLDocument.

		NOTE: This is probably not the correct tool to
		copy a document, since XMLDocuments can have multiple
		top level XMLNodes. You probably want to use
        XMLDocument::DeepCopy()
	*/
	XMLNode* DeepClone( document* target ) const;

    /**
    	Test if 2 nodes are the same, but don't test children.
    	The 2 nodes do not need to be in the same Document.

    	Note: if called on a XMLDocument, this will return false.
    */
    virtual bool ShallowEqual( const XMLNode* compare ) const = 0;

    /** Accept a hierarchical visit of the nodes in the TinyXML-2 DOM. Every node in the
    	XML tree will be conditionally visited and the host will be called back
    	via the XMLVisitor interface.

    	This is essentially a SAX interface for TinyXML-2. (Note however it doesn't re-parse
    	the XML for the callbacks, so the performance of TinyXML-2 is unchanged by using this
    	interface versus any other.)

    	The interface has been based on ideas from:

    	- http://www.saxproject.org/
    	- http://c2.com/cgi/wiki?HierarchicalVisitorPattern

    	Which are both good references for "visiting".

    	An example of using Accept():
    	@verbatim
    	XMLPrinter printer;
    	tinyxmlDoc.Accept( &printer );
    	const char* xmlcstr = printer.CStr();
    	@endverbatim
    */
    virtual bool Accept( XMLVisitor* visitor ) const = 0;

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

    virtual char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr);

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

    XMLNode( const XMLNode& );	// not supported
    XMLNode& operator=( const XMLNode& );	// not supported
};


/** XML text.

	Note that a text node can have child element nodes, for example:
	@verbatim
	<root>This is <b>bold</b></root>
	@endverbatim

	A text node can have 2 ways to output the next. "normal" output
	and CDATA. It will default to the mode it was parsed from the XML file and
	you generally want to leave it alone, but you can change the output mode with
	SetCData() and query it with CData().
*/
class XMLText : public XMLNode
{
    friend class document;
public:
    virtual bool Accept( XMLVisitor* visitor ) const override;

    virtual XMLText* ToText() override		{
        return this;
    }
    virtual const XMLText* ToText() const override {
        return this;
    }

    /// Declare whether this should be CDATA or standard text.
    void SetCData( bool isCData )			{
        _isCData = isCData;
    }
    /// Returns true if this is a CDATA text element.
    bool CData() const						{
        return _isCData;
    }

    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

protected:
    explicit XMLText( document* doc )	: XMLNode( doc ), _isCData( false )	{}
    virtual ~XMLText()												{}

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr ) override;

private:
    bool _isCData;

    XMLText( const XMLText& );	// not supported
    XMLText& operator=( const XMLText& );	// not supported
};


/** An XML Comment. */
class XMLComment : public XMLNode
{
    friend class document;
public:
    virtual XMLComment*	ToComment() override		{
        return this;
    }
    virtual const XMLComment* ToComment() const override {
        return this;
    }

    virtual bool Accept( XMLVisitor* visitor ) const override;

    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

protected:
    explicit XMLComment( document* doc );
    virtual ~XMLComment();

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr) override;

private:
    XMLComment( const XMLComment& );	// not supported
    XMLComment& operator=( const XMLComment& );	// not supported
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXML-2 will happily read or write files without a declaration,
	however.

	The text of the declaration isn't interpreted. It is parsed
	and written as a string.
*/
class XMLDeclaration : public XMLNode
{
    friend class document;
public:
    virtual XMLDeclaration*	ToDeclaration() override		{
        return this;
    }
    virtual const XMLDeclaration* ToDeclaration() const override {
        return this;
    }

    virtual bool Accept( XMLVisitor* visitor ) const override;

    virtual XMLNode* ShallowClone( document* document ) const override;
    virtual bool ShallowEqual( const XMLNode* compare ) const override;

protected:
    explicit XMLDeclaration( document* doc );
    virtual ~XMLDeclaration();

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr ) override;

private:
    XMLDeclaration( const XMLDeclaration& );	// not supported
    XMLDeclaration& operator=( const XMLDeclaration& );	// not supported
};


/** Any tag that TinyXML-2 doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.

	DTD tags get thrown into XMLUnknowns.
*/
// class XMLUnknown : public XMLNode
// {
//     friend class document;
// public:
//     virtual XMLUnknown*	ToUnknown() override		{
//         return this;
//     }
//     virtual const XMLUnknown* ToUnknown() const override {
//         return this;
//     }
//
//     virtual bool Accept( XMLVisitor* visitor ) const override;
//
//     virtual XMLNode* ShallowClone( document* document ) const override;
//     virtual bool ShallowEqual( const XMLNode* compare ) const override;
//
// protected:
//     explicit XMLUnknown( document* doc );
//     virtual ~XMLUnknown();
//
//     char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr ) override;
//
// private:
//     XMLUnknown( const XMLUnknown& );	// not supported
//     XMLUnknown& operator=( const XMLUnknown& );	// not supported
// };



/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not XMLNodes. You may only query the
	Next() attribute in a list.
*/
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
    virtual ~XMLAttribute()	{}

    XMLAttribute( const XMLAttribute& );	// not supported
    void operator=( const XMLAttribute& );	// not supported
    void SetName( const char* name );

    char* ParseDeep( char* p, bool processEntities, int* curLineNumPtr );

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

    virtual element* ToElement() override	{
        return this;
    }
    virtual const element* ToElement() const override {
        return this;
    }
    virtual bool Accept( XMLVisitor* visitor ) const override;

    /** Given an attribute name, Attribute() returns the value
    	for the attribute of that name, or null if none
    	exists. For example:

    	@verbatim
    	const char* value = ele->Attribute( "foo" );
    	@endverbatim

    	The 'value' parameter is normally null. However, if specified,
    	the attribute will only be returned if the 'name' and 'value'
    	match. This allow you to write code:

    	@verbatim
    	if ( ele->Attribute( "foo", "bar" ) ) callFooIsBar();
    	@endverbatim

    	rather than:
    	@verbatim
    	if ( ele->Attribute( "foo" ) ) {
    		if ( strcmp( ele->Attribute( "foo" ), "bar" ) == 0 ) callFooIsBar();
    	}
    	@endverbatim
    */
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

    /** Convenience function for easy access to the text inside an element. Although easy
    	and concise, GetText() is limited compared to getting the XMLText child
    	and accessing it directly.

    	If the first child of 'this' is a XMLText, the GetText()
    	returns the character string of the Text node, else null is returned.

    	This is a convenient method for getting the text of simple contained text:
    	@verbatim
    	<foo>This is text</foo>
    		const char* str = fooElement->GetText();
    	@endverbatim

    	'str' will be a pointer to "This is text".

    	Note that this function can be misleading. If the element foo was created from
    	this XML:
    	@verbatim
    		<foo><b>This is text</b></foo>
    	@endverbatim

    	then the value of str would be null. The first child node isn't a text node, it is
    	another element. From this XML:
    	@verbatim
    		<foo>This is <b>text</b></foo>
    	@endverbatim
    	GetText() will return "This is ".
    */
    const char* GetText() const;

    /** Convenience function for easy access to the text inside an element. Although easy
    	and concise, SetText() is limited compared to creating an XMLText child
    	and mutating it directly.

    	If the first child of 'this' is a XMLText, SetText() sets its value to
		the given string, otherwise it will create a first child that is an XMLText.

    	This is a convenient method for setting the text of simple contained text:
    	@verbatim
    	<foo>This is text</foo>
    		fooElement->SetText( "Hullaballoo!" );
     	<foo>Hullaballoo!</foo>
		@endverbatim

    	Note that this function can be misleading. If the element foo was created from
    	this XML:
    	@verbatim
    		<foo><b>This is text</b></foo>
    	@endverbatim

    	then it will not change "This is text", but rather prefix it with a text element:
    	@verbatim
    		<foo>Hullaballoo!<b>This is text</b></foo>
    	@endverbatim

		For this XML:
    	@verbatim
    		<foo />
    	@endverbatim
    	SetText() will generate
    	@verbatim
    		<foo>Hullaballoo!</foo>
    	@endverbatim
    */
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

protected:
    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr ) override;

private:
    element( document* doc );
    virtual ~element();
    element( const element& );	// not supported
    void operator=( const element& );	// not supported

    XMLAttribute* FindOrCreateAttribute( const char* name );
    char* ParseAttributes( char* p, int* curLineNumPtr );
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

    virtual document* ToDocument() override		{
        TIXMLASSERT( this == _document );
        return this;
    }
    virtual const document* ToDocument() const override {
        TIXMLASSERT( this == _document );
        return this;
    }

    /**
    	Parse an XML file from a character string.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.

    	You may optionally pass in the 'nBytes', which is
    	the number of bytes which will be parsed. If not
    	specified, TinyXML-2 will assume 'xml' points to a
    	null terminated string.
    */
    void Parse( const char* xml, size_t nBytes=static_cast<size_t>(-1) );

    /**
    	Load an XML file from disk.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */

    /**
    	Save the XML file to disk.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
    void   SaveFile( const char* filename, bool compact = false );

    /**
    	Save the XML file to disk. You are responsible
    	for providing and closing the FILE*.

    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
    void   SaveFile( FILE* fp, bool compact = false );

    bool ProcessEntities() const		{
        return _processEntities;
    }
    Whitespace WhitespaceMode() const	{
        return _whitespaceMode;
    }

    /**
    	Returns true if this document has a leading Byte Order Mark of UTF8.
    */
    bool HasBOM() const {
        return _writeBOM;
    }
    /** Sets whether to write the BOM when writing the file.
    */
    void SetBOM( bool useBOM ) {
        _writeBOM = useBOM;
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

    /** Print the Document. If the Printer is not provided, it will
        print to stdout. If you provide Printer, this can print to a file:
    	@verbatim
    	XMLPrinter printer( fp );
    	doc.Print( &printer );
    	@endverbatim

    	Or you can use a printer to print to memory:
    	@verbatim
    	XMLPrinter printer;
    	doc.Print( &printer );
    	// printer.CStr() has a const char* to the XML
    	@endverbatim
    */
    void Print( XMLPrinter* streamer=0 ) const;
    virtual bool Accept( XMLVisitor* visitor ) const override;

    /**
    	Create a new Element associated with
    	this Document. The memory for the Element
    	is managed by the Document.
    */
    element* make_elem( const char* name );
    /**
    	Create a new Comment associated with
    	this Document. The memory for the Comment
    	is managed by the Document.
    */
    XMLComment* make_comm( const char* comment );
    /**
    	Create a new Text associated with
    	this Document. The memory for the Text
    	is managed by the Document.
    */
    XMLText* NewText( const char* text );
    /**
    	Create a new Declaration associated with
    	this Document. The memory for the object
    	is managed by the Document.

    	If the 'text' param is null, the standard
    	declaration is used.:
    	@verbatim
    		<?xml version="1.0" encoding="UTF-8"?>
    	@endverbatim
    */
    XMLDeclaration* make_decl( const char* text=0 );
    /**
    	Create a new Unknown associated with
    	this Document. The memory for the object
    	is managed by the Document.
    */

    /**
    	Delete a node associated with this document.
    	It will be unlinked from the DOM.
    */
    void DeleteNode( XMLNode* node );

    /// Clear the document, resetting it to the initial state.
    void Clear();

	/**
		Copies this document to a target document.
		The target will be completely cleared before the copy.
		If you want to copy a sub-tree, see XMLNode::DeepClone().

		NOTE: that the 'target' must be non-null.
	*/
	void DeepCopy(document* target) const;

	// internal
    char* Identify( char* p, XMLNode** node, bool first );

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

    bool			_writeBOM;
    bool			_processEntities;
    Whitespace		_whitespaceMode;
    char*			_charBuffer;
    int				_parseCurLineNum;
	int				_parsingDepth;
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

    void Parse();

	template <typename ... Args>
    constexpr void print_error_and_exit( error_t error, int lineNum, std::format_string<Args...> fmt, Args&& ... args) {
		std::cout << std::format("Error={:s} ErrorID={:d} Line number={:d} Error detail: ",
			_errorNames[static_cast<std::uint32_t>(error)], static_cast<std::uint32_t>(error), lineNum) <<
				std::format(fmt, std::forward<Args>(args)...) << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// Something of an obvious security hole, once it was discovered.
	// Either an ill-formed XML or an excessively deep one can overflow
	// the stack. Track stack depth, and error out if needed.
	class DepthTracker {
	public:
		explicit DepthTracker(document * document) {
			this->_document = document;
			document->PushDepth();
		}
		~DepthTracker() {
			_document->PopDepth();
		}
	private:
		document * _document;
	};
	void PushDepth();
	void PopDepth();

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

/**
	Printing functionality. The XMLPrinter gives you more
	options than the XMLDocument::Print() method.

	It can:
	-# Print to memory.
	-# Print to a file you provide.
	-# Print XML without a XMLDocument.

	Print to Memory

	@verbatim
	XMLPrinter printer;
	doc.Print( &printer );
	SomeFunction( printer.CStr() );
	@endverbatim

	Print to a File

	You provide the file pointer.
	@verbatim
	XMLPrinter printer( fp );
	doc.Print( &printer );
	@endverbatim

	Print without a XMLDocument

	When loading, an XML parser is very useful. However, sometimes
	when saving, it just gets in the way. The code is often set up
	for streaming, and constructing the DOM is just overhead.

	The Printer supports the streaming case. The following code
	prints out a trivially simple XML file without ever creating
	an XML document.

	@verbatim
	XMLPrinter printer( fp );
	printer.OpenElement( "foo" );
	printer.PushAttribute( "foo", "bar" );
	printer.CloseElement();
	@endverbatim
*/
class XMLPrinter : public XMLVisitor
{
public:
    enum EscapeAposCharsInAttributes {
        ESCAPE_APOS_CHARS_IN_ATTRIBUTES,
        DONT_ESCAPE_APOS_CHARS_IN_ATTRIBUTES
    };

    /** Construct the printer. If the FILE* is specified,
    	this will print to the FILE. Else it will print
    	to memory, and the result is available in CStr().
    	If 'compact' is set to true, then output is created
    	with only required whitespace and newlines.
    */
    XMLPrinter( FILE* file=0, bool compact = false, int depth = 0, EscapeAposCharsInAttributes aposInAttributes = DONT_ESCAPE_APOS_CHARS_IN_ATTRIBUTES );
    ~XMLPrinter() override = default;

    /** If streaming, write the BOM and declaration. */
    void PushHeader( bool writeBOM, bool writeDeclaration );
    /** If streaming, start writing an element.
        The element must be closed with CloseElement()
    */
    void OpenElement( const char* name, bool compactMode=false );
    /// If streaming, add an attribute to an open element.
    void PushAttribute( const char* name, const char* value );
    /// If streaming, close the Element.
    virtual void CloseElement( bool compactMode=false );

    /// Add a text node.
    void PushText( const char* text, bool cdata=false );

    /// Add a comment
    void PushComment( const char* comment );

    void PushDeclaration( const char* value );
    void PushUnknown( const char* value );

    virtual bool VisitEnter( const document& /*doc*/ ) override;
    virtual bool VisitExit( const document& /*doc*/ ) override	{
        return true;
    }

    virtual bool VisitEnter( const element& elem, const XMLAttribute* attribute ) override;
    virtual bool VisitExit( const element& element ) override;

    virtual bool Visit( const XMLText& text ) override;
    virtual bool Visit( const XMLComment& comment ) override;
    virtual bool Visit( const XMLDeclaration& declaration ) override;

    /**
    	If in print to memory mode, return a pointer to
    	the XML file in memory.
    */
    const char* CStr() const {
        return _buffer.Mem();
    }
    /**
    	If in print to memory mode, return the size
    	of the XML file in memory. (Note the size returned
    	includes the terminating null.)
    */
    size_t CStrSize() const {
        return _buffer.Size();
    }
    /**
    	If in print to memory mode, reset the buffer to the
    	beginning.
    */
    void ClearBuffer( bool resetToFirstElement = true ) {
        _buffer.Clear();
        _buffer.Push(0);
		_firstElement = resetToFirstElement;
    }

protected:
	bool CompactMode( const element& )	{ return _compactMode; }

	/** Prints out the space before an element. You may override to change
	    the space and tabs used. A PrintSpace() override should call Print().
	*/
    void PrintSpace( int depth );
    void Write( const char* data, size_t size );
    void Putc( char ch );

    inline void Write(const char* data) { Write(data, strlen(data)); }

    void SealElementIfJustOpened();
    bool _elementJustOpened;
    DynArray< const char*, 10 > _stack;

private:
    /**
       Prepares to write a new node. This includes sealing an element that was
       just opened, and writing any whitespace necessary if not in compact mode.
     */
    void PrepareForNewNode( bool compactMode );
    void PrintString( const char*, bool restrictedEntitySet );	// prints out, after detecting entities.

    bool _firstElement;
    FILE* _fp;
    int _depth;
    int _textDepth;
    bool _processEntities;
	bool _compactMode;

    enum {
        ENTITY_RANGE = 64,
        BUF_SIZE = 200
    };
    bool _entityFlag[ENTITY_RANGE];
    bool _restrictedEntityFlag[ENTITY_RANGE];

    DynArray< char, 20 > _buffer;
};


} // namespace tinyxml2

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif // TINYXML2_INCLUDED