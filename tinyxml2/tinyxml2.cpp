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

namespace msvc_xml
{
    

StrPair::~StrPair()
{
    Reset();
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
    Reset();
    size_t len = strlen( str );
    _start = new char[ len+1 ];
    memcpy( _start, str, len+1 );
    _end = _start + len;
    _flags = flags | NEEDS_DELETE;
}

const char* StrPair::GetStr()
{
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
    return _start;
}
    
// --------- XMLUtil ----------- //

bool document::Accept( printer* visitor ) const {
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

void XMLNode::SetValue( const char* str) {
    _value.SetStr( str );
}

XMLNode* XMLNode::DeepClone(document* target) const
{
	XMLNode* clone = this->ShallowClone(target);
	if (!clone) return 0;

	for (const XMLNode* child = this->FirstChild(); child; child = child->NextSibling()) {
		XMLNode* childClone = child->DeepClone(target);
		clone->insert_child_end(childClone);
	}
	return clone;
}

void XMLNode::DeleteChildren()
{
    while( _firstChild ) {
        DeleteChild( _firstChild );
    }
    _firstChild = _lastChild = 0;
}


void XMLNode::Unlink( XMLNode* child )
{
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
    Unlink( node );
    DeleteNode( node );
}


XMLNode* XMLNode::insert_child_end( XMLNode* addThis )
{
    if ( addThis->_document != _document ) {
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _lastChild ) {
        _lastChild->_next = addThis;
        addThis->_prev = _lastChild;
        _lastChild = addThis;

        addThis->_next = 0;
    }
    else {
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->_parent = this;
    return addThis;
}


XMLNode* XMLNode::InsertFirstChild( XMLNode* addThis )
{
    if ( addThis->_document != _document ) {
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _firstChild ) {

        _firstChild->_prev = addThis;
        addThis->_next = _firstChild;
        _firstChild = addThis;

        addThis->_prev = 0;
    }
    else {
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->_parent = this;
    return addThis;
}


XMLNode* XMLNode::InsertAfterChild( XMLNode* afterThis, XMLNode* addThis )
{
    if ( addThis->_document != _document ) {
        return 0;
    }


    if ( afterThis->_parent != this ) {
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
	if (dynamic_cast<document*>(node)) {
		node->_document->MarkInUse(node);
	}

    MemPool* pool = node->_memPool;
    node->~XMLNode();
    pool->Free( node );
}

void XMLNode::InsertChildPreamble( XMLNode* insertThis ) const
{
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
    const XMLText* text = dynamic_cast<const XMLText*>(compare);
    return ( text && XMLUtil::StringEqual( text->Value(), Value() ) );
}


bool XMLText::Accept( printer* visitor ) const {
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
    auto comment = dynamic_cast<const XMLComment*>(compare);
    return ( comment && XMLUtil::StringEqual( comment->Value(), Value() ));
}


bool XMLComment::Accept( printer* visitor ) const {
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
    auto declaration = dynamic_cast<const XMLDeclaration*>(compare);
    return ( declaration && XMLUtil::StringEqual( declaration->Value(), Value() ));
}



bool XMLDeclaration::Accept( printer* visitor ) const {
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
        if ( last ) {
            last->_next = attrib;
        }
        else {
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
    XMLAttribute* attrib = new (_document->_attributePool.Alloc() ) XMLAttribute();
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


bool element::Accept( printer* visitor ) const {
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
    
document::document() :
    XMLNode( nullptr ),
    _charBuffer( nullptr ) {
    _document = this;
}


document::~document() {
    Clear();
}


void document::MarkInUse(const XMLNode* const node)
{
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


XMLDeclaration* document::make_decl( const char* str ) {
    XMLDeclaration* dec = CreateUnlinkedNode<XMLDeclaration>( _commentPool );
    dec->SetValue( str ? str : "xml version=\"1.0\" encoding=\"UTF-8\"" );
    return dec;
}

void document::DeleteNode( XMLNode* node )	{
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
    std::abort();
}


void document::SaveFile(std::ofstream& f, bool compact ) {
    // Clear any error from the last save, otherwise it will get reported
    // for *this* call.
    printer stream( f, compact );
    Print( &stream );
}

void document::Print( printer* streamer ) const {
    Accept( streamer );
}

printer::printer(std::ostream& f, bool compact, int depth) :
    _elementJustOpened( false ),
    _stack(),
    _firstElement( true ),
    _fp( &f ),
    _depth( depth ),
    _textDepth( -1 ),
    _compactMode( compact ) {
}

void printer::PrepareForNewNode( bool compactMode ) {
    SealElementIfJustOpened();
    if ( compactMode ) {
        return;
    }
    std::string space(_depth << 1, ' ');
    if ( _firstElement ) {
        makexx::tiny_print(*_fp, space);
    } else if ( _textDepth < 0) {
        makexx::tiny_print(*_fp, "\n{:s}", space);
    }
    _firstElement = false;
}

void printer::OpenElement( const char* name, bool compactMode )
{
    PrepareForNewNode( compactMode );
    _stack.Push( name );
    makexx::tiny_print(*_fp, "<{:s}", name);
    _elementJustOpened = true;
    ++_depth;
}


void printer::PushAttribute( const char* name, const char* value ) {
    makexx::tiny_print(*_fp, " {:s}=\"{:s}\"", name, value);
}

void printer::CloseElement( bool compactMode )
{
    --_depth;
    const char* name = _stack.Pop();

    if ( _elementJustOpened ) {
        _fp->write("/>", 2);
    }
    else {
        if ( _textDepth < 0 && !compactMode) {

            std::string space(_depth << 1, ' ');
            makexx::tiny_print(*_fp, "\n{:s}", space);
        }
        makexx::tiny_print(*_fp, "</{:s}>", name);
    }

    if ( _textDepth == _depth ) {
        _textDepth = -1;
    }
    if ( _depth == 0 && !compactMode) {
        _fp->write("\n", 1);
    }
    _elementJustOpened = false;
}


void printer::SealElementIfJustOpened() {
    if ( !_elementJustOpened ) {
        return;
    }
    _elementJustOpened = false;
    _fp->write(">", 1);
}


void printer::PushText( const char* text)
{
    _textDepth = _depth-1;

    SealElementIfJustOpened();
    _fp->write(text, std::strlen(text));
}

void printer::PushComment( const char* comment ) {
    PrepareForNewNode( _compactMode );
    makexx::tiny_print(*_fp, "<!--{:s}-->", comment);
}


void printer::PushDeclaration( const char* value ) {
    PrepareForNewNode( _compactMode );
    makexx::tiny_print(*_fp, "<?{:s}?>", value);
}

bool printer::VisitEnter( const document& doc ) {
    return true;
}


bool printer::VisitEnter( const element& elem, const XMLAttribute* attribute ) {
    auto parentElem = elem.parent_elem();
    const bool compactMode = parentElem ? CompactMode( *parentElem ) : _compactMode;
    OpenElement( elem.Name(), compactMode );
    while ( attribute ) {
        PushAttribute( attribute->Name(), attribute->Value() );
        attribute = attribute->Next();
    }
    return true;
}


bool printer::VisitExit( const element& element ) {
    CloseElement( CompactMode(element) );
    return true;
}


bool printer::Visit( const XMLText& text ) {
    PushText( text.Value());
    return true;
}


bool printer::Visit( const XMLComment& comment ) {
    PushComment( comment.Value() );
    return true;
}

bool printer::Visit( const XMLDeclaration& declaration ) {
    PushDeclaration( declaration.Value() );
    return true;
}

}   // namespace tinyxml2
