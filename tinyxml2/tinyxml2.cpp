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

#include "tinyxml2.h"

#include <new>		// yes, this one new style header, is in the Android SDK.
#include <cstddef>

static constexpr char LINE_FEED				= static_cast<char>(0x0a);			// all line endings are normalized to LF
static constexpr char LF = LINE_FEED;
static constexpr char CARRIAGE_RETURN		= static_cast<char>(0x0d);			// CR gets filtered out
static constexpr char CR = CARRIAGE_RETURN;
static constexpr char SINGLE_QUOTE			= '\'';
static constexpr char DOUBLE_QUOTE			= '\"';

// Bunch of unicode info at:
//		http://www.unicode.org/faq/utf_bom.html
//	ef bb bf (Microsoft "lead bytes") - designates UTF-8

static constexpr unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static constexpr unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static constexpr unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

namespace msvc_xml
{

struct Entity {
    const char* pattern;
    int length;
    char value;
};

static const int NUM_ENTITIES = 5;
static const Entity entities[NUM_ENTITIES] = {
    { "quot", 4,	DOUBLE_QUOTE },
    { "amp", 3,		'&'  },
    { "apos", 4,	SINGLE_QUOTE },
    { "lt",	2, 		'<'	 },
    { "gt",	2,		'>'	 }
};


StrPair::~StrPair()
{
    Reset();
}


void StrPair::TransferTo( StrPair* other )
{
    if ( this == other ) {
        return;
    }
    // This in effect implements the assignment operator by "moving"
    // ownership (as in auto_ptr).

    TIXMLASSERT( other != 0 );
    TIXMLASSERT( other->_flags == 0 );
    TIXMLASSERT( other->_start == 0 );
    TIXMLASSERT( other->_end == 0 );

    other->Reset();

    other->_flags = _flags;
    other->_start = _start;
    other->_end = _end;

    _flags = 0;
    _start = 0;
    _end = 0;
}


void StrPair::Reset()
{
    if ( _flags & NEEDS_DELETE ) {
        delete [] _start;
    }
    _flags = 0;
    _start = 0;
    _end = 0;
}


void StrPair::SetStr( const char* str, int flags )
{
    TIXMLASSERT( str );
    Reset();
    size_t len = strlen( str );
    TIXMLASSERT( _start == 0 );
    _start = new char[ len+1 ];
    memcpy( _start, str, len+1 );
    _end = _start + len;
    _flags = flags | NEEDS_DELETE;
}

const char* StrPair::GetStr()
{
    TIXMLASSERT( _start );
    TIXMLASSERT( _end );
    if ( _flags & NEEDS_FLUSH ) {
        *_end = 0;
        _flags ^= NEEDS_FLUSH;

        if ( _flags ) {
            const char* p = _start;	// the read pointer
            char* q = _start;	// the write pointer

            while( p < _end ) {
                if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == CR ) {
                    // CR-LF pair becomes LF
                    // CR alone becomes LF
                    // LF-CR becomes LF
                    if ( *(p+1) == LF ) {
                        p += 2;
                    }
                    else {
                        ++p;
                    }
                    *q = LF;
                    ++q;
                }
                else if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == LF ) {
                    if ( *(p+1) == CR ) {
                        p += 2;
                    }
                    else {
                        ++p;
                    }
                    *q = LF;
                    ++q;
                }
                else if ( (_flags & NEEDS_ENTITY_PROCESSING) && *p == '&' ) {
                    // Entities handled by tinyXML2:
                    // - special entities in the entity table [in/out]
                    // - numeric character reference [in]
                    //   &#20013; or &#x4e2d;

                    if ( *(p+1) == '#' ) {
                        const int buflen = 10;
                        char buf[buflen] = { 0 };
                        int len = 0;
                        const char* adjusted = const_cast<char*>( XMLUtil::GetCharacterRef( p, buf, &len ) );
                        if ( adjusted == 0 ) {
                            *q = *p;
                            ++p;
                            ++q;
                        }
                        else {
                            TIXMLASSERT( 0 <= len && len <= buflen );
                            TIXMLASSERT( q + len <= adjusted );
                            p = adjusted;
                            memcpy( q, buf, len );
                            q += len;
                        }
                    }
                    else {
                        bool entityFound = false;
                        for( int i = 0; i < NUM_ENTITIES; ++i ) {
                            const Entity& entity = entities[i];
                            if ( strncmp( p + 1, entity.pattern, entity.length ) == 0
                                    && *( p + entity.length + 1 ) == ';' ) {
                                // Found an entity - convert.
                                *q = entity.value;
                                ++q;
                                p += entity.length + 2;
                                entityFound = true;
                                break;
                            }
                        }
                        if ( !entityFound ) {
                            // fixme: treat as error?
                            ++p;
                            ++q;
                        }
                    }
                }
                else {
                    *q = *p;
                    ++p;
                    ++q;
                }
            }
            *q = 0;
        }
        _flags = (_flags & NEEDS_DELETE);
    }
    TIXMLASSERT( _start );
    return _start;
}
    
// --------- XMLUtil ----------- //


void XMLUtil::ConvertUTF32ToUTF8( unsigned long input, char* output, int* length )
{
    const unsigned long BYTE_MASK = 0xBF;
    const unsigned long BYTE_MARK = 0x80;
    const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    if (input < 0x80) {
        *length = 1;
    }
    else if ( input < 0x800 ) {
        *length = 2;
    }
    else if ( input < 0x10000 ) {
        *length = 3;
    }
    else if ( input < 0x200000 ) {
        *length = 4;
    }
    else {
        *length = 0;    // This code won't convert this correctly anyway.
        return;
    }

    output += *length;

    // Scary scary fall throughs are annotated with carefully designed comments
    // to suppress compiler warnings such as -Wimplicit-fallthrough in gcc
    switch (*length) {
        case 4:
            --output;
            *output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
            //fall through
        case 3:
            --output;
            *output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
            //fall through
        case 2:
            --output;
            *output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
            //fall through
        case 1:
            --output;
            *output = static_cast<char>(input | FIRST_BYTE_MARK[*length]);
            break;
        default:
            TIXMLASSERT( false );
    }
}


const char* XMLUtil::GetCharacterRef(const char* p, char* value, int* length)
{
    // Assume an entity, and pull it out.
    *length = 0;

    static const uint32_t MAX_CODE_POINT = 0x10FFFF;

    if (*(p + 1) == '#' && *(p + 2)) {
        uint32_t ucs = 0;
        ptrdiff_t delta = 0;
        uint32_t mult = 1;
        static const char SEMICOLON = ';';

        bool hex = false;
        uint32_t radix = 10;
        const char* q = 0;
        char terminator = '#';

        if (*(p + 2) == 'x') {
            // Hexadecimal.
            hex = true;
            radix = 16;
            terminator = 'x';

            q = p + 3;
        }
        else {
            // Decimal.
            q = p + 2;
        }
        if (!(*q)) {
            return 0;
        }

        q = strchr(q, SEMICOLON);
        if (!q) {
            return 0;
        }
        TIXMLASSERT(*q == SEMICOLON);

        delta = q - p;
        --q;

        while (*q != terminator) {
            uint32_t digit = 0;

            if (*q >= '0' && *q <= '9') {
                digit = *q - '0';
            }
            else if (hex && (*q >= 'a' && *q <= 'f')) {
                digit = *q - 'a' + 10;
            }
            else if (hex && (*q >= 'A' && *q <= 'F')) {
                digit = *q - 'A' + 10;
            }
            else {
                return 0;
            }
            TIXMLASSERT(digit < radix);

            const unsigned int digitScaled = mult * digit;
            ucs += digitScaled;
            mult *= radix;       
            
            // Security check: could a value exist that is out of range?
            // Easily; limit to the MAX_CODE_POINT, which also allows for a
            // bunch of leading zeroes.
            if (mult > MAX_CODE_POINT) {
                mult = MAX_CODE_POINT;
            }
            --q;
        }
        // Out of range:
        if (ucs > MAX_CODE_POINT) {
            return 0;
        }
        // convert the UCS to UTF-8
        ConvertUTF32ToUTF8(ucs, value, length);
		if (length == 0) {
            // If length is 0, there was an error. (Security? Bad input?)
            // Fail safely.
			return 0;
		}
        return p + delta + 1;
    }
    return p + 1;
}

bool document::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    if ( visitor->VisitEnter( *this ) ) {
        for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() ) {
            if ( !node->Accept( visitor ) ) {
                break;
            }
        }
    }
    return visitor->VisitExit( *this );
}


// --------- XMLNode ----------- //

XMLNode::XMLNode( document* doc ) :
    _document( doc ),
    _parent( 0 ),
    _value(),
    _parseLineNum( 0 ),
    _firstChild( 0 ), _lastChild( 0 ),
    _prev( 0 ), _next( 0 ),
	_userData( 0 ),
    _memPool( 0 )
{
}


XMLNode::~XMLNode()
{
    DeleteChildren();
    if ( _parent ) {
        _parent->Unlink( this );
    }
}

// ChildElementCount was originally suggested by msteiger on the sourceforge page for TinyXML and modified by KB1SPH for TinyXML-2.

int XMLNode::ChildElementCount(const char *value) const {
	int count = 0;

	const element *e = begin_child_elem(value);

	while (e) {
		e = e->next_sibling_elem(value);
		count++;
	}

	return count;
}

int XMLNode::ChildElementCount() const {
	int count = 0;

	const element *e = begin_child_elem();

	while (e) {
		e = e->next_sibling_elem();
		count++;
	}

	return count;
}

const char* XMLNode::Value() const
{
    // Edge case: XMLDocuments don't have a Value. Return null.
    if (dynamic_cast<const document*>(this))
        return 0;
    return _value.GetStr();
}

void XMLNode::SetValue( const char* str, bool staticMem )
{
    if ( staticMem ) {
        _value.SetInternedStr( str );
    }
    else {
        _value.SetStr( str );
    }
}

XMLNode* XMLNode::DeepClone(document* target) const
{
	XMLNode* clone = this->ShallowClone(target);
	if (!clone) return 0;

	for (const XMLNode* child = this->FirstChild(); child; child = child->NextSibling()) {
		XMLNode* childClone = child->DeepClone(target);
		TIXMLASSERT(childClone);
		clone->insert_child_end(childClone);
	}
	return clone;
}

void XMLNode::DeleteChildren()
{
    while( _firstChild ) {
        TIXMLASSERT( _lastChild );
        DeleteChild( _firstChild );
    }
    _firstChild = _lastChild = 0;
}


void XMLNode::Unlink( XMLNode* child )
{
    TIXMLASSERT( child );
    TIXMLASSERT( child->_document == _document );
    TIXMLASSERT( child->_parent == this );
    if ( child == _firstChild ) {
        _firstChild = _firstChild->_next;
    }
    if ( child == _lastChild ) {
        _lastChild = _lastChild->_prev;
    }

    if ( child->_prev ) {
        child->_prev->_next = child->_next;
    }
    if ( child->_next ) {
        child->_next->_prev = child->_prev;
    }
	child->_next = 0;
	child->_prev = 0;
	child->_parent = 0;
}


void XMLNode::DeleteChild( XMLNode* node )
{
    TIXMLASSERT( node );
    TIXMLASSERT( node->_document == _document );
    TIXMLASSERT( node->_parent == this );
    Unlink( node );
	TIXMLASSERT(node->_prev == 0);
	TIXMLASSERT(node->_next == 0);
	TIXMLASSERT(node->_parent == 0);
    DeleteNode( node );
}


XMLNode* XMLNode::insert_child_end( XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _lastChild ) {
        TIXMLASSERT( _firstChild );
        TIXMLASSERT( _lastChild->_next == 0 );
        _lastChild->_next = addThis;
        addThis->_prev = _lastChild;
        _lastChild = addThis;

        addThis->_next = 0;
    }
    else {
        TIXMLASSERT( _firstChild == 0 );
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->_parent = this;
    return addThis;
}


XMLNode* XMLNode::InsertFirstChild( XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _firstChild ) {
        TIXMLASSERT( _lastChild );
        TIXMLASSERT( _firstChild->_prev == 0 );

        _firstChild->_prev = addThis;
        addThis->_next = _firstChild;
        _firstChild = addThis;

        addThis->_prev = 0;
    }
    else {
        TIXMLASSERT( _lastChild == 0 );
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->_parent = this;
    return addThis;
}


XMLNode* XMLNode::InsertAfterChild( XMLNode* afterThis, XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }

    TIXMLASSERT( afterThis );

    if ( afterThis->_parent != this ) {
        TIXMLASSERT( false );
        return 0;
    }
    if ( afterThis == addThis ) {
        // Current state: BeforeThis -> AddThis -> OneAfterAddThis
        // Now AddThis must disappear from it's location and then
        // reappear between BeforeThis and OneAfterAddThis.
        // So just leave it where it is.
        return addThis;
    }

    if ( afterThis->_next == 0 ) {
        // The last node or the only node.
        return insert_child_end( addThis );
    }
    InsertChildPreamble( addThis );
    addThis->_prev = afterThis;
    addThis->_next = afterThis->_next;
    afterThis->_next->_prev = addThis;
    afterThis->_next = addThis;
    addThis->_parent = this;
    return addThis;
}




const element* XMLNode::begin_child_elem( const char* name ) const
{
    for( const XMLNode* node = _firstChild; node; node = node->_next ) {
        const element* element = node->ToElementWithName( name );
        if ( element ) {
            return element;
        }
    }
    return 0;
}


const element* XMLNode::LastChildElement( const char* name ) const
{
    for( const XMLNode* node = _lastChild; node; node = node->_prev ) {
        const element* element = node->ToElementWithName( name );
        if ( element ) {
            return element;
        }
    }
    return 0;
}


const element* XMLNode::next_sibling_elem( const char* name ) const
{
    for( const XMLNode* node = _next; node; node = node->_next ) {
        const element* element = node->ToElementWithName( name );
        if ( element ) {
            return element;
        }
    }
    return 0;
}


const element* XMLNode::PreviousSiblingElement( const char* name ) const
{
    for( const XMLNode* node = _prev; node; node = node->_prev ) {
        const element* element = node->ToElementWithName( name );
        if ( element ) {
            return element;
        }
    }
    return 0;
}
    
/*static*/ void XMLNode::DeleteNode( XMLNode* node )
{
    if ( node == 0 ) {
        return;
    }
	TIXMLASSERT(node->_document);
	if (dynamic_cast<document*>(node)) {
		node->_document->MarkInUse(node);
	}

    MemPool* pool = node->_memPool;
    node->~XMLNode();
    pool->Free( node );
}

void XMLNode::InsertChildPreamble( XMLNode* insertThis ) const
{
    TIXMLASSERT( insertThis );
    TIXMLASSERT( insertThis->_document == _document );

	if (insertThis->_parent) {
        insertThis->_parent->Unlink( insertThis );
	}
	else {
		insertThis->_document->MarkInUse(insertThis);
        insertThis->_memPool->SetTracked();
	}
}

const element* XMLNode::ToElementWithName( const char* name ) const
{
    const element* elem = dynamic_cast<const element*>(this);
    if ( elem == 0 ) {
        return 0;
    }
    if ( name == 0 ) {
        return elem;
    }
    if ( XMLUtil::StringEqual( elem->Name(), name ) ) {
       return elem;
    }
    return 0;
}

// --------- XMLText ---------- //

XMLNode* XMLText::ShallowClone( document* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLText* text = doc->NewText( Value() );	// fixme: this will always allocate memory. Intern?
    return text;
}


bool XMLText::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    const XMLText* text = dynamic_cast<const XMLText*>(compare);
    return ( text && XMLUtil::StringEqual( text->Value(), Value() ) );
}


bool XMLText::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}


// --------- XMLComment ---------- //

XMLNode* XMLComment::ShallowClone( document* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLComment* comment = doc->make_comm( Value() );	// fixme: this will always allocate memory. Intern?
    return comment;
}


bool XMLComment::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    auto comment = dynamic_cast<const XMLComment*>(compare);
    return ( comment && XMLUtil::StringEqual( comment->Value(), Value() ));
}


bool XMLComment::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}


// --------- XMLDeclaration ---------- //

XMLNode* XMLDeclaration::ShallowClone( document* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLDeclaration* dec = doc->make_decl( Value() );	// fixme: this will always allocate memory. Intern?
    return dec;
}


bool XMLDeclaration::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    auto declaration = dynamic_cast<const XMLDeclaration*>(compare);
    return ( declaration && XMLUtil::StringEqual( declaration->Value(), Value() ));
}



bool XMLDeclaration::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}

// --------- XMLAttribute ---------- //

const char* XMLAttribute::Name() const
{
    return _name.GetStr();
}

const char* XMLAttribute::Value() const
{
    return _value.GetStr();
}

void XMLAttribute::SetName( const char* n )
{
    _name.SetStr( n );
}

void XMLAttribute::SetAttribute( const char* v )
{
    _value.SetStr( v );
}

// --------- XMLElement ---------- //
element::element( document* doc ) : XMLNode( doc ),
    _closingType( OPEN ),
    _rootAttribute( 0 )
{
}


element::~element()
{
    while( _rootAttribute ) {
        XMLAttribute* next = _rootAttribute->_next;
        DeleteAttribute( _rootAttribute );
        _rootAttribute = next;
    }
}


const XMLAttribute* element::find_attrib( const char* name ) const
{
    for( XMLAttribute* a = _rootAttribute; a; a = a->_next ) {
        if ( XMLUtil::StringEqual( a->Name(), name ) ) {
            return a;
        }
    }
    return 0;
}


const char* element::get_attrib( const char* name, const char* value ) const
{
    const XMLAttribute* a = find_attrib( name );
    if ( !a ) {
        return 0;
    }
    if ( !value || XMLUtil::StringEqual( a->Value(), value )) {
        return a->Value();
    }
    return 0;
}

const char* element::GetText() const
{
    /* skip comment node */
    const XMLNode* node = FirstChild();
    while (node) {
        if (dynamic_cast<const XMLComment*>(node)) {
            node = node->NextSibling();
            continue;
        }
        break;
    }

    if ( node && dynamic_cast<const XMLText*>(node)) {
        return node->Value();
    }
    return 0;
}


element*	element::set_txt( const char* inText )
{
	if ( FirstChild() && dynamic_cast<XMLText*>(FirstChild()))
		FirstChild()->SetValue( inText );
	else {
		XMLText*	theText = GetDocument()->NewText( inText );
		InsertFirstChild( theText );
	}
    return this;
}


XMLAttribute* element::FindOrCreateAttribute( const char* name )
{
    XMLAttribute* last = 0;
    XMLAttribute* attrib = 0;
    for( attrib = _rootAttribute;
            attrib;
            last = attrib, attrib = attrib->_next ) {
        if ( XMLUtil::StringEqual( attrib->Name(), name ) ) {
            break;
        }
    }
    if ( !attrib ) {
        attrib = CreateAttribute();
        TIXMLASSERT( attrib );
        if ( last ) {
            TIXMLASSERT( last->_next == 0 );
            last->_next = attrib;
        }
        else {
            TIXMLASSERT( _rootAttribute == 0 );
            _rootAttribute = attrib;
        }
        attrib->SetName( name );
    }
    return attrib;
}


void element::DeleteAttribute( const char* name )
{
    XMLAttribute* prev = 0;
    for( XMLAttribute* a=_rootAttribute; a; a=a->_next ) {
        if ( XMLUtil::StringEqual( name, a->Name() ) ) {
            if ( prev ) {
                prev->_next = a->_next;
            }
            else {
                _rootAttribute = a->_next;
            }
            DeleteAttribute( a );
            break;
        }
        prev = a;
    }
}

void element::DeleteAttribute( XMLAttribute* attribute )
{
    if ( attribute == 0 ) {
        return;
    }
    MemPool* pool = attribute->_memPool;
    attribute->~XMLAttribute();
    pool->Free( attribute );
}

XMLAttribute* element::CreateAttribute()
{
    TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
    XMLAttribute* attrib = new (_document->_attributePool.Alloc() ) XMLAttribute();
    TIXMLASSERT( attrib );
    attrib->_memPool = &_document->_attributePool;
    attrib->_memPool->SetTracked();
    return attrib;
}


element* element::insert_child_elem(const char* name)
{
    element* node = _document->make_elem(name);
    return insert_child_end(node) ? node : 0;
}

XMLComment* element::insert_comm(const char* comment)
{
    XMLComment* node = _document->make_comm(comment);
    return insert_child_end(node) ? node : 0;
}

XMLText* element::InsertNewText(const char* text)
{
    XMLText* node = _document->NewText(text);
    return insert_child_end(node) ? node : 0;
}

XMLDeclaration* element::InsertNewDeclaration(const char* text)
{
    XMLDeclaration* node = _document->make_decl(text);
    return insert_child_end(node) ? node : 0;
}

XMLNode* element::ShallowClone( document* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    element* element = doc->make_elem( Value() );					// fixme: this will always allocate memory. Intern?
    for( const XMLAttribute* a=FirstAttribute(); a; a=a->Next() ) {
        element->set_attrib( a->Name(), a->Value() );					// fixme: this will always allocate memory. Intern?
    }
    return element;
}


bool element::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    auto other = dynamic_cast<const element*>(compare);
    if ( other && XMLUtil::StringEqual( other->Name(), Name() )) {

        const XMLAttribute* a=FirstAttribute();
        const XMLAttribute* b=other->FirstAttribute();

        while ( a && b ) {
            if ( !XMLUtil::StringEqual( a->Value(), b->Value() ) ) {
                return false;
            }
            a = a->Next();
            b = b->Next();
        }
        if ( a || b ) {
            // different count
            return false;
        }
        return true;
    }
    return false;
}


bool element::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    if ( visitor->VisitEnter( *this, _rootAttribute ) ) {
        for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() ) {
            if ( !node->Accept( visitor ) ) {
                break;
            }
        }
    }
    return visitor->VisitExit( *this );
}


// --------- XMLDocument ----------- //
    
document::document( bool processEntities, Whitespace whitespaceMode ) :
    XMLNode( 0 ),
    _processEntities( processEntities ),
    _whitespaceMode( whitespaceMode ),
    _charBuffer( 0 ) {
    _document = this;
}


document::~document()
{
    Clear();
}


void document::MarkInUse(const XMLNode* const node)
{
	TIXMLASSERT(node);
	TIXMLASSERT(node->_parent == 0);

	for (size_t i = 0; i < _unlinked.Size(); ++i) {
		if (node == _unlinked[i]) {
			_unlinked.SwapRemove(i);
			break;
		}
	}
}

void document::Clear()
{
    DeleteChildren();
	while( _unlinked.Size()) {
		DeleteNode(_unlinked[0]);	// Will remove from _unlinked as part of delete.
	}
    
    delete [] _charBuffer;
    _charBuffer = 0;
}


void document::DeepCopy(document* target) const
{
	TIXMLASSERT(target);
    if (target == this) {
        return; // technically success - a no-op.
    }

	target->Clear();
	for (const XMLNode* node = this->FirstChild(); node; node = node->NextSibling()) {
		target->insert_child_end(node->DeepClone(target));
	}
}

element* document::make_elem( const char* name )
{
    element* ele = CreateUnlinkedNode<element>( _elementPool );
    ele->SetName( name );
    return ele;
}


XMLComment* document::make_comm( const char* str )
{
    XMLComment* comment = CreateUnlinkedNode<XMLComment>( _commentPool );
    comment->SetValue( str );
    return comment;
}


XMLText* document::NewText( const char* str )
{
    XMLText* text = CreateUnlinkedNode<XMLText>( _textPool );
    text->SetValue( str );
    return text;
}


XMLDeclaration* document::make_decl( const char* str )
{
    XMLDeclaration* dec = CreateUnlinkedNode<XMLDeclaration>( _commentPool );
    dec->SetValue( str ? str : "xml version=\"1.0\" encoding=\"UTF-8\"" );
    return dec;
}
    
static FILE* callfopen( const char* filepath, const char* mode )
{
    TIXMLASSERT( filepath );
    TIXMLASSERT( mode );
#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
    FILE* fp = 0;
    const errno_t err = fopen_s( &fp, filepath, mode );
    if ( err ) {
        return 0;
    }
#else
    FILE* fp = fopen( filepath, mode );
#endif
    return fp;
}

void document::DeleteNode( XMLNode* node )	{
    TIXMLASSERT( node );
    TIXMLASSERT(node->_document == this );
    if (node->_parent) {
        node->_parent->DeleteChild( node );
    }
    else {
        // Isn't in the tree.
        // Use the parent delete.
        // Also, we need to mark it tracked: we 'know'
        // it was never used.
        node->_memPool->SetTracked();
        // Call the static XMLNode version:
        XMLNode::DeleteNode(node);
    }
}
    

void document::SaveFile( const char* filename, bool compact ) {
    std::ofstream file(filename);
    if (file.good() ) {
        SaveFile(file, compact);
        file.close();
        return;
    }
    std::cout.rdbuf()->sputn("File coult not be opened at line 0\n", 36);
}


void document::SaveFile(std::ofstream& f, bool compact ) {
    // Clear any error from the last save, otherwise it will get reported
    // for *this* call.
    XMLPrinter stream( f, compact );
    Print( &stream );
}

void document::Print( XMLPrinter* streamer ) const {
    Accept( streamer );
}

XMLPrinter::XMLPrinter(std::ostream& f, bool compact, int depth) :
    _elementJustOpened( false ),
    _stack(),
    _firstElement( true ),
    _fp( &f ),
    _depth( depth ),
    _textDepth( -1 ),
    _processEntities( true ),
    _compactMode( compact )
{
    for( int i=0; i<ENTITY_RANGE; ++i ) {
        _entityFlag[i] = false;
        _restrictedEntityFlag[i] = false;
    }
    for( int i=0; i<NUM_ENTITIES; ++i ) {
        const char entityValue = entities[i].value;
        if (entityValue != SINGLE_QUOTE) {
            const unsigned char flagIndex = static_cast<unsigned char>(entityValue);
            TIXMLASSERT( flagIndex < ENTITY_RANGE );
            _entityFlag[flagIndex] = true;
        }
    }
    _restrictedEntityFlag[static_cast<unsigned char>('&')] = true;
    _restrictedEntityFlag[static_cast<unsigned char>('<')] = true;
    _restrictedEntityFlag[static_cast<unsigned char>('>')] = true;	// not required, but consistency is nice
}

void XMLPrinter::PrintString( const char* p, bool restricted )
{
    // Look for runs of bytes between entities to print.
    const char* q = p;

    if ( _processEntities ) {
        const bool* flag = restricted ? _restrictedEntityFlag : _entityFlag;
        while ( *q ) {
            TIXMLASSERT( p <= q );
            // Remember, char is sometimes signed. (How many times has that bitten me?)
            if ( *q > 0 && *q < ENTITY_RANGE ) {
                // Check for entities. If one is found, flush
                // the stream up until the entity, write the
                // entity, and keep looking.
                if ( flag[static_cast<unsigned char>(*q)] ) {
                    while ( p < q ) {
                        const size_t delta = q - p;
                        const int toPrint = ( INT_MAX < delta ) ? INT_MAX : static_cast<int>(delta);
                        Write( p, toPrint );
                        p += toPrint;
                    }
                    bool entityPatternPrinted = false;
                    for( int i=0; i<NUM_ENTITIES; ++i ) {
                        if ( entities[i].value == *q ) {
                            Putc( '&' );
                            Write( entities[i].pattern, entities[i].length );
                            Putc( ';' );
                            entityPatternPrinted = true;
                            break;
                        }
                    }
                    if ( !entityPatternPrinted ) {
                        // TIXMLASSERT( entityPatternPrinted ) causes gcc -Wunused-but-set-variable in release
                        TIXMLASSERT( false );
                    }
                    ++p;
                }
            }
            ++q;
            TIXMLASSERT( p <= q );
        }
        // Flush the remaining string. This will be the entire
        // string if an entity wasn't found.
        if ( p < q ) {
            const size_t delta = q - p;
            const int toPrint = ( INT_MAX < delta ) ? INT_MAX : static_cast<int>(delta);
            Write( p, toPrint );
        }
    }
    else {
        Write( p );
    }
}

void XMLPrinter::PrepareForNewNode( bool compactMode )
{
    SealElementIfJustOpened();

    if ( compactMode ) {
        return;
    }

    if ( _firstElement ) {
        PrintSpace (_depth);
    } else if ( _textDepth < 0) {
        Putc( '\n' );
        PrintSpace( _depth );
    }

    _firstElement = false;
}

void XMLPrinter::OpenElement( const char* name, bool compactMode )
{
    PrepareForNewNode( compactMode );
    _stack.Push( name );

    Write ( "<" );
    Write ( name );

    _elementJustOpened = true;
    ++_depth;
}


void XMLPrinter::PushAttribute( const char* name, const char* value )
{
    TIXMLASSERT( _elementJustOpened );
    Putc ( ' ' );
    Write( name );
    Write( "=\"" );
    PrintString( value, false );
    Putc ( '\"' );
}

void XMLPrinter::CloseElement( bool compactMode )
{
    --_depth;
    const char* name = _stack.Pop();

    if ( _elementJustOpened ) {
        Write( "/>" );
    }
    else {
        if ( _textDepth < 0 && !compactMode) {
            Putc( '\n' );
            PrintSpace( _depth );
        }
        Write ( "</" );
        Write ( name );
        Write ( ">" );
    }

    if ( _textDepth == _depth ) {
        _textDepth = -1;
    }
    if ( _depth == 0 && !compactMode) {
        Putc( '\n' );
    }
    _elementJustOpened = false;
}


void XMLPrinter::SealElementIfJustOpened()
{
    if ( !_elementJustOpened ) {
        return;
    }
    _elementJustOpened = false;
    Putc( '>' );
}


void XMLPrinter::PushText( const char* text)
{
    _textDepth = _depth-1;

    SealElementIfJustOpened();
    PrintString( text, true );
}

void XMLPrinter::PushComment( const char* comment )
{
    PrepareForNewNode( _compactMode );

    Write( "<!--" );
    Write( comment );
    Write( "-->" );
}


void XMLPrinter::PushDeclaration( const char* value )
{
    PrepareForNewNode( _compactMode );

    Write( "<?" );
    Write( value );
    Write( "?>" );
}

bool XMLPrinter::VisitEnter( const document& doc )
{
    _processEntities = doc.ProcessEntities();
    return true;
}


bool XMLPrinter::VisitEnter( const element& elem, const XMLAttribute* attribute )
{
    const element* parentElem = 0;
    if ( elem.Parent() ) {
        parentElem = dynamic_cast<const element*>(elem.Parent());
    }
    const bool compactMode = parentElem ? CompactMode( *parentElem ) : _compactMode;
    OpenElement( elem.Name(), compactMode );
    while ( attribute ) {
        PushAttribute( attribute->Name(), attribute->Value() );
        attribute = attribute->Next();
    }
    return true;
}


bool XMLPrinter::VisitExit( const element& element )
{
    CloseElement( CompactMode(element) );
    return true;
}


bool XMLPrinter::Visit( const XMLText& text )
{
    PushText( text.Value());
    return true;
}


bool XMLPrinter::Visit( const XMLComment& comment )
{
    PushComment( comment.Value() );
    return true;
}

bool XMLPrinter::Visit( const XMLDeclaration& declaration )
{
    PushDeclaration( declaration.Value() );
    return true;
}

}   // namespace tinyxml2
