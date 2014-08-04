#include <new>
#include <cassert>

#include "html_node.hpp"
#include "html_attribute.hpp"
#include "pugiutil.hpp"
#include "html_document.hpp"
#include "common.hpp"


using namespace pugihtml;


enum chartypex_t {
	ctx_special_pcdata = 1,   // Any symbol >= 0 and < 32 (except \t, \r, \n), &, <, >
	ctx_special_attr = 2,	  // Any symbol >= 0 and < 32 (except \t), &, <, >, "
	ctx_start_symbol = 4,	  // Any symbol > 127, a-z, A-Z, _
	ctx_digit = 8,			  // 0-9
	ctx_symbol = 16			  // Any symbol > 127, a-z, A-Z, 0-9, _, -, .
};


const unsigned char chartypex_table[256] = {
	3,	3,	3,	3,	3,	3,	3,	3,	   3,  0,  2,  3,  3,  2,  3,  3,	  // 0-15
	3,	3,	3,	3,	3,	3,	3,	3,	   3,  3,  3,  3,  3,  3,  3,  3,	  // 16-31
	0,	0,	2,	0,	0,	0,	3,	0,	   0,  0,  0,  0,  0, 16, 16,  0,	  // 32-47
	24, 24, 24, 24, 24, 24, 24, 24,    24, 24, 0,  0,  3,  0,  3,  0,	  // 48-63

	0,	20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 64-79
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  20,	  // 80-95
	0,	20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 96-111
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  0,	  // 112-127

	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 128+
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20
};

#ifdef PUGIHTML_WCHAR_MODE
#define IS_CHARTYPE_IMPL(c, ct, table) ((static_cast<unsigned int>(c) < 128 ? table[static_cast<unsigned int>(c)] : table[128]) & (ct))
#else
#define IS_CHARTYPE_IMPL(c, ct, table) (table[static_cast<unsigned char>(c)] & (ct))
#endif

#define IS_CHARTYPEX(c, ct) IS_CHARTYPE_IMPL(c, ct, chartypex_table)


static inline html_allocator&
get_allocator(const html_node_struct* node)
{
	assert(node);

	return *reinterpret_cast<html_memory_page*>(node->header
		& html_memory_page_pointer_mask)->allocator;
}


inline html_attribute_struct*
allocate_attribute(html_allocator& alloc)
{
	html_memory_page* page;
	void* memory = alloc.allocate_memory(sizeof(html_attribute_struct), page);

	return new (memory) html_attribute_struct(page);
}


inline void
destroy_attribute(html_attribute_struct* a, html_allocator& alloc)
{
	uintptr_t header = a->header;

	if (header & html_memory_page_name_allocated_mask) alloc.deallocate_string(a->name);
	if (header & html_memory_page_value_allocated_mask) alloc.deallocate_string(a->value);

	alloc.deallocate_memory(a, sizeof(html_attribute_struct),
		reinterpret_cast<html_memory_page*>(header
		& html_memory_page_pointer_mask));
}


inline void
destroy_node(html_node_struct* n, html_allocator& alloc)
{
	uintptr_t header = n->header;

	if (header & html_memory_page_name_allocated_mask) alloc.deallocate_string(n->name);
	if (header & html_memory_page_value_allocated_mask) alloc.deallocate_string(n->value);

	for (html_attribute_struct* attr = n->first_attribute; attr; ) {
		html_attribute_struct* next = attr->next_attribute;

		destroy_attribute(attr, alloc);

		attr = next;
	}

	for (html_node_struct* child = n->first_child; child; ) {
		html_node_struct* next = child->next_sibling;

		destroy_node(child, alloc);

		child = next;
	}

	alloc.deallocate_memory(n, sizeof(html_node_struct),
		reinterpret_cast<html_memory_page*>(header
		& html_memory_page_pointer_mask));
}


inline bool
allow_insert_child(html_node_type parent, html_node_type child)
{
	if (parent != node_document && parent != node_element) return false;
	if (child == node_document || child == node_null) return false;
	if (parent != node_document && (child == node_declaration
		|| child == node_doctype)) return false;

	return true;
}


html_node::html_node(): _root(0)
{
}

html_node::html_node(html_node_struct* p): _root(p)
{
}

html_node::operator html_node::unspecified_bool_type() const
{
	return _root ? &html_node::_root : 0;
}

bool html_node::operator!() const
{
	return !_root;
}

html_node::iterator html_node::begin() const
{
	return iterator(_root ? _root->first_child : 0, _root);
}

html_node::iterator html_node::end() const
{
	return iterator(0, _root);
}

html_node::attribute_iterator html_node::attributes_begin() const
{
	return attribute_iterator(_root ? _root->first_attribute : 0, _root);
}

html_node::attribute_iterator html_node::attributes_end() const
{
	return attribute_iterator(0, _root);
}

bool html_node::operator==(const html_node& r) const
{
	return (_root == r._root);
}

bool html_node::operator!=(const html_node& r) const
{
	return (_root != r._root);
}

bool html_node::operator<(const html_node& r) const
{
	return (_root < r._root);
}

bool html_node::operator>(const html_node& r) const
{
	return (_root > r._root);
}

bool html_node::operator<=(const html_node& r) const
{
	return (_root <= r._root);
}

bool html_node::operator>=(const html_node& r) const
{
	return (_root >= r._root);
}

bool html_node::empty() const
{
	return !_root;
}

const char_t* html_node::name() const
{
	return (_root && _root->name) ? _root->name : PUGIHTML_TEXT("");
}

html_node_type html_node::type() const
{
	return _root ? static_cast<html_node_type>((_root->header & html_memory_page_type_mask) + 1) : node_null;
}

const char_t* html_node::value() const
{
	return (_root && _root->value) ? _root->value : PUGIHTML_TEXT("");
}

html_node html_node::child(const char_t* name) const
{
	if (!_root) return html_node();

	for (html_node_struct* i = _root->first_child; i; i = i->next_sibling)
		if (i->name && strequal(name, i->name)) return html_node(i);

	return html_node();
}

html_attribute html_node::attribute(const char_t* name) const
{
	if (!_root) return html_attribute();

	for (html_attribute_struct* i = _root->first_attribute; i; i = i->next_attribute)
		if (i->name && strequal(name, i->name))
			return html_attribute(i);

	return html_attribute();
}

html_node html_node::next_sibling(const char_t* name) const
{
	if (!_root) return html_node();

	for (html_node_struct* i = _root->next_sibling; i; i = i->next_sibling)
		if (i->name && strequal(name, i->name)) return html_node(i);

	return html_node();
}

html_node html_node::next_sibling() const
{
	if (!_root) return html_node();

	if (_root->next_sibling) return html_node(_root->next_sibling);
	else return html_node();
}

html_node html_node::previous_sibling(const char_t* name) const
{
	if (!_root) return html_node();

	for (html_node_struct* i = _root->prev_sibling_c; i->next_sibling; i = i->prev_sibling_c)
		if (i->name && strequal(name, i->name)) return html_node(i);

	return html_node();
}

html_node html_node::previous_sibling() const
{
	if (!_root) return html_node();

	if (_root->prev_sibling_c->next_sibling) return html_node(_root->prev_sibling_c);
	else return html_node();
}

html_node html_node::parent() const
{
	return _root ? html_node(_root->parent) : html_node();
}

html_node html_node::root() const
{
	if (!_root) return html_node();

	html_memory_page* page = reinterpret_cast<html_memory_page*>(_root->header & html_memory_page_pointer_mask);

	return html_node(static_cast<html_document_struct*>(page->allocator));
}

const char_t* html_node::child_value() const
{
	if (!_root) return PUGIHTML_TEXT("");

	for (html_node_struct* i = _root->first_child; i; i = i->next_sibling)
	{
		html_node_type type = static_cast<html_node_type>((i->header & html_memory_page_type_mask) + 1);

		if (i->value && (type == node_pcdata || type == node_cdata))
			return i->value;
	}

	return PUGIHTML_TEXT("");
}

const char_t* html_node::child_value(const char_t* name) const
{
	return child(name).child_value();
}

html_attribute html_node::first_attribute() const
{
	return _root ? html_attribute(_root->first_attribute) : html_attribute();
}

html_attribute html_node::last_attribute() const
{
	return _root && _root->first_attribute ? html_attribute(_root->first_attribute->prev_attribute_c) : html_attribute();
}

html_node html_node::first_child() const
{
	return _root ? html_node(_root->first_child) : html_node();
}

html_node html_node::last_child() const
{
	return _root && _root->first_child ? html_node(_root->first_child->prev_sibling_c) : html_node();
}

bool html_node::set_name(const char_t* rhs)
{
	switch (type())
	{
	case node_pi:
	case node_declaration:
	case node_element:
		return strcpy_insitu(_root->name, _root->header, html_memory_page_name_allocated_mask, rhs);

	default:
		return false;
	}
}

bool html_node::set_value(const char_t* rhs)
{
	switch (type())
	{
	case node_pi:
	case node_cdata:
	case node_pcdata:
	case node_comment:
	case node_doctype:
		return strcpy_insitu(_root->value, _root->header, html_memory_page_value_allocated_mask, rhs);

	default:
		return false;
	}
}

html_attribute html_node::append_attribute(const char_t* name)
{
	if (type() != node_element && type() != node_declaration) return html_attribute();

	html_attribute a(append_attribute_ll(_root, get_allocator(_root)));
	a.set_name(name);

	return a;
}

html_attribute html_node::prepend_attribute(const char_t* name)
{
	if (type() != node_element && type() != node_declaration) return html_attribute();

	html_attribute a(allocate_attribute(get_allocator(_root)));
	if (!a) return html_attribute();

	a.set_name(name);

	html_attribute_struct* head = _root->first_attribute;

	if (head)
	{
		a._attr->prev_attribute_c = head->prev_attribute_c;
		head->prev_attribute_c = a._attr;
	}
	else
		a._attr->prev_attribute_c = a._attr;

	a._attr->next_attribute = head;
	_root->first_attribute = a._attr;

	return a;
}

html_attribute html_node::insert_attribute_before(const char_t* name, const html_attribute& attr)
{
	if ((type() != node_element && type() != node_declaration) || attr.empty()) return html_attribute();

	// check that attribute belongs to *this
	html_attribute_struct* cur = attr._attr;

	while (cur->prev_attribute_c->next_attribute) cur = cur->prev_attribute_c;

	if (cur != _root->first_attribute) return html_attribute();

	html_attribute a(allocate_attribute(get_allocator(_root)));
	if (!a) return html_attribute();

	a.set_name(name);

	if (attr._attr->prev_attribute_c->next_attribute)
		attr._attr->prev_attribute_c->next_attribute = a._attr;
	else
		_root->first_attribute = a._attr;

	a._attr->prev_attribute_c = attr._attr->prev_attribute_c;
	a._attr->next_attribute = attr._attr;
	attr._attr->prev_attribute_c = a._attr;

	return a;
}

html_attribute html_node::insert_attribute_after(const char_t* name, const html_attribute& attr)
{
	if ((type() != node_element && type() != node_declaration) || attr.empty()) return html_attribute();

	// check that attribute belongs to *this
	html_attribute_struct* cur = attr._attr;

	while (cur->prev_attribute_c->next_attribute) cur = cur->prev_attribute_c;

	if (cur != _root->first_attribute) return html_attribute();

	html_attribute a(allocate_attribute(get_allocator(_root)));
	if (!a) return html_attribute();

	a.set_name(name);

	if (attr._attr->next_attribute)
		attr._attr->next_attribute->prev_attribute_c = a._attr;
	else
		_root->first_attribute->prev_attribute_c = a._attr;

	a._attr->next_attribute = attr._attr->next_attribute;
	a._attr->prev_attribute_c = attr._attr;
	attr._attr->next_attribute = a._attr;

	return a;
}

html_attribute html_node::append_copy(const html_attribute& proto)
{
	if (!proto) return html_attribute();

	html_attribute result = append_attribute(proto.name());
	result.set_value(proto.value());

	return result;
}

html_attribute html_node::prepend_copy(const html_attribute& proto)
{
	if (!proto) return html_attribute();

	html_attribute result = prepend_attribute(proto.name());
	result.set_value(proto.value());

	return result;
}

html_attribute html_node::insert_copy_after(const html_attribute& proto, const html_attribute& attr)
{
	if (!proto) return html_attribute();

	html_attribute result = insert_attribute_after(proto.name(), attr);
	result.set_value(proto.value());

	return result;
}

html_attribute html_node::insert_copy_before(const html_attribute& proto, const html_attribute& attr)
{
	if (!proto) return html_attribute();

	html_attribute result = insert_attribute_before(proto.name(), attr);
	result.set_value(proto.value());

	return result;
}

html_node html_node::append_child(html_node_type type)
{
	if (!allow_insert_child(this->type(), type)) return html_node();

	html_node n(append_node(_root, get_allocator(_root), type));

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}

html_node html_node::prepend_child(html_node_type type)
{
	if (!allow_insert_child(this->type(), type)) return html_node();

	html_node n(allocate_node(get_allocator(_root), type));
	if (!n) return html_node();

	n._root->parent = _root;

	html_node_struct* head = _root->first_child;

	if (head)
	{
		n._root->prev_sibling_c = head->prev_sibling_c;
		head->prev_sibling_c = n._root;
	}
	else
		n._root->prev_sibling_c = n._root;

	n._root->next_sibling = head;
	_root->first_child = n._root;

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}

html_node html_node::insert_child_before(html_node_type type, const html_node& node)
{
	if (!allow_insert_child(this->type(), type)) return html_node();
	if (!node._root || node._root->parent != _root) return html_node();

	html_node n(allocate_node(get_allocator(_root), type));
	if (!n) return html_node();

	n._root->parent = _root;

	if (node._root->prev_sibling_c->next_sibling)
		node._root->prev_sibling_c->next_sibling = n._root;
	else
		_root->first_child = n._root;

	n._root->prev_sibling_c = node._root->prev_sibling_c;
	n._root->next_sibling = node._root;
	node._root->prev_sibling_c = n._root;

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}

html_node html_node::insert_child_after(html_node_type type, const html_node& node)
{
	if (!allow_insert_child(this->type(), type)) return html_node();
	if (!node._root || node._root->parent != _root) return html_node();

	html_node n(allocate_node(get_allocator(_root), type));
	if (!n) return html_node();

	n._root->parent = _root;

	if (node._root->next_sibling)
		node._root->next_sibling->prev_sibling_c = n._root;
	else
		_root->first_child->prev_sibling_c = n._root;

	n._root->next_sibling = node._root->next_sibling;
	n._root->prev_sibling_c = node._root;
	node._root->next_sibling = n._root;

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}

html_node html_node::append_child(const char_t* name)
{
	html_node result = append_child(node_element);

	result.set_name(name);

	return result;
}

html_node html_node::prepend_child(const char_t* name)
{
	html_node result = prepend_child(node_element);

	result.set_name(name);

	return result;
}

html_node html_node::insert_child_after(const char_t* name, const html_node& node)
{
	html_node result = insert_child_after(node_element, node);

	result.set_name(name);

	return result;
}

html_node html_node::insert_child_before(const char_t* name, const html_node& node)
{
	html_node result = insert_child_before(node_element, node);

	result.set_name(name);

	return result;
}


void recursive_copy_skip(html_node& dest, const html_node& source, const html_node& skip)
{
	assert(dest.type() == source.type());

	switch (source.type())
	{
	case node_element:
	{
		dest.set_name(source.name());

		for (html_attribute a = source.first_attribute(); a; a = a.next_attribute())
			dest.append_attribute(a.name()).set_value(a.value());

		for (html_node c = source.first_child(); c; c = c.next_sibling())
		{
			if (c == skip) continue;

			html_node cc = dest.append_child(c.type());
			assert(cc);

			recursive_copy_skip(cc, c, skip);
		}

		break;
	}

	case node_pcdata:
	case node_cdata:
	case node_comment:
	case node_doctype:
		dest.set_value(source.value());
		break;

	case node_pi:
		dest.set_name(source.name());
		dest.set_value(source.value());
		break;

	case node_declaration:
	{
		dest.set_name(source.name());

		for (html_attribute a = source.first_attribute(); a; a = a.next_attribute())
			dest.append_attribute(a.name()).set_value(a.value());

		break;
	}

	default:
		assert(!"Invalid node type");
	}
}


html_node html_node::append_copy(const html_node& proto)
{
	html_node result = append_child(proto.type());

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}

html_node html_node::prepend_copy(const html_node& proto)
{
	html_node result = prepend_child(proto.type());

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}

html_node html_node::insert_copy_after(const html_node& proto, const html_node& node)
{
	html_node result = insert_child_after(proto.type(), node);

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}

html_node html_node::insert_copy_before(const html_node& proto, const html_node& node)
{
	html_node result = insert_child_before(proto.type(), node);

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}

bool html_node::remove_attribute(const char_t* name)
{
	return remove_attribute(attribute(name));
}

bool html_node::remove_attribute(const html_attribute& a)
{
	if (!_root || !a._attr) return false;

	// check that attribute belongs to *this
	html_attribute_struct* attr = a._attr;

	while (attr->prev_attribute_c->next_attribute) attr = attr->prev_attribute_c;

	if (attr != _root->first_attribute) return false;

	if (a._attr->next_attribute) a._attr->next_attribute->prev_attribute_c = a._attr->prev_attribute_c;
	else if (_root->first_attribute) _root->first_attribute->prev_attribute_c = a._attr->prev_attribute_c;

	if (a._attr->prev_attribute_c->next_attribute) a._attr->prev_attribute_c->next_attribute = a._attr->next_attribute;
	else _root->first_attribute = a._attr->next_attribute;

	destroy_attribute(a._attr, get_allocator(_root));

	return true;
}

bool html_node::remove_child(const char_t* name)
{
	return remove_child(child(name));
}

bool html_node::remove_child(const html_node& n)
{
	if (!_root || !n._root || n._root->parent != _root) return false;

	if (n._root->next_sibling) n._root->next_sibling->prev_sibling_c = n._root->prev_sibling_c;
	else if (_root->first_child) _root->first_child->prev_sibling_c = n._root->prev_sibling_c;

	if (n._root->prev_sibling_c->next_sibling) n._root->prev_sibling_c->next_sibling = n._root->next_sibling;
	else _root->first_child = n._root->next_sibling;

	destroy_node(n._root, get_allocator(_root));

	return true;
}

html_node html_node::find_child_by_attribute(const char_t* name, const char_t* attr_name, const char_t* attr_value) const
{
	if (!_root) return html_node();

	for (html_node_struct* i = _root->first_child; i; i = i->next_sibling)
		if (i->name && strequal(name, i->name))
		{
			for (html_attribute_struct* a = i->first_attribute; a; a = a->next_attribute)
				if (strequal(attr_name, a->name) && strequal(attr_value, a->value))
					return html_node(i);
		}

	return html_node();
}

html_node html_node::find_child_by_attribute(const char_t* attr_name, const char_t* attr_value) const
{
	if (!_root) return html_node();

	for (html_node_struct* i = _root->first_child; i; i = i->next_sibling)
		for (html_attribute_struct* a = i->first_attribute; a; a = a->next_attribute)
			if (strequal(attr_name, a->name) && strequal(attr_value, a->value))
				return html_node(i);

	return html_node();
}

#ifndef PUGIHTML_NO_STL
string_t
html_node::path(char_t delimiter) const
{
	string_t path;

	html_node cursor = *this; // Make a copy.

	path = cursor.name();

	while (cursor.parent()) {
		cursor = cursor.parent();

		string_t temp = cursor.name();
		temp += delimiter;
		temp += path;
		path.swap(temp);
	}

	return path;
}
#endif

html_node html_node::first_element_by_path(const char_t* path, char_t delimiter) const
{
	html_node found = *this; // Current search context.

	if (!_root || !path || !path[0]) return found;

	if (path[0] == delimiter)
	{
		// Absolute path; e.g. '/foo/bar'
		found = found.root();
		++path;
	}

	const char_t* path_segment = path;

	while (*path_segment == delimiter) ++path_segment;

	const char_t* path_segment_end = path_segment;

	while (*path_segment_end && *path_segment_end != delimiter) ++path_segment_end;

	if (path_segment == path_segment_end) return found;

	const char_t* next_segment = path_segment_end;

	while (*next_segment == delimiter) ++next_segment;

	if (*path_segment == '.' && path_segment + 1 == path_segment_end)
		return found.first_element_by_path(next_segment, delimiter);
	else if (*path_segment == '.' && *(path_segment+1) == '.' && path_segment + 2 == path_segment_end)
		return found.parent().first_element_by_path(next_segment, delimiter);
	else
	{
		for (html_node_struct* j = found._root->first_child; j; j = j->next_sibling)
		{
			if (j->name && strequalrange(j->name, path_segment, static_cast<size_t>(path_segment_end - path_segment)))
			{
				html_node subsearch = html_node(j).first_element_by_path(next_segment, delimiter);

				if (subsearch) return subsearch;
			}
		}

		return html_node();
	}
}

bool
html_node::traverse(html_tree_walker& walker)
{
	walker._depth = -1;

	html_node arg_begin = *this;
	if (!walker.begin(arg_begin)) return false;

	html_node cur = first_child();

	if (cur) {
		++walker._depth;

		do {
			html_node arg_for_each = cur;
			if (!walker.for_each(arg_for_each))
				return false;

			if (cur.first_child()) {
				++walker._depth;
				cur = cur.first_child();
			}
			else if (cur.next_sibling())
				cur = cur.next_sibling();
			else {
				// Borland C++ workaround
				while (!cur.next_sibling() && cur != *this && (bool)cur.parent())
				{
					--walker._depth;
					cur = cur.parent();
				}

				if (cur != *this)
					cur = cur.next_sibling();
			}
		}
		while (cur && cur != *this);
	}

	assert(walker._depth == -1);

	html_node arg_end = *this;
	return walker.end(arg_end);
}

size_t html_node::hash_value() const
{
	return static_cast<size_t>(reinterpret_cast<uintptr_t>(_root) / sizeof(html_node_struct));
}

html_node_struct* html_node::internal_object() const
{
	return _root;
}

void
html_node::print(html_writer& writer, const char_t* indent, unsigned int flags,
	html_encoding encoding, unsigned int depth) const
{
	if (!_root) return;

	html_buffered_writer buffered_writer(writer, encoding);

	node_output(buffered_writer, *this, indent, flags, depth);
}

#ifndef PUGIHTML_NO_STL
void html_node::print(std::basic_ostream<char, std::char_traits<char> >& stream, const char_t* indent, unsigned int flags, html_encoding encoding, unsigned int depth) const
{
	html_writer_stream writer(stream);

	print(writer, indent, flags, encoding, depth);
}

void html_node::print(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream, const char_t* indent, unsigned int flags, unsigned int depth) const
{
	html_writer_stream writer(stream);

	print(writer, indent, flags, encoding_wchar, depth);
}
#endif

ptrdiff_t html_node::offset_debug() const
{
	html_node_struct* r = root()._root;

	if (!r) return -1;

	const char_t* buffer = static_cast<html_document_struct*>(r)->buffer;

	if (!buffer) return -1;

	switch (type())
	{
	case node_document:
		return 0;

	case node_element:
	case node_declaration:
	case node_pi:
		return (_root->header & html_memory_page_name_allocated_mask) ? -1 : _root->name - buffer;

	case node_pcdata:
	case node_cdata:
	case node_comment:
	case node_doctype:
		return (_root->header & html_memory_page_value_allocated_mask) ? -1 : _root->value - buffer;

	default:
		return -1;
	}
}

#ifdef __BORLANDC__
bool operator&&(const html_node& lhs, bool rhs)
{
	return (bool)lhs && rhs;
}

bool operator||(const html_node& lhs, bool rhs)
{
	return (bool)lhs || rhs;
}
#endif


html_node_iterator::html_node_iterator()
{
}

html_node_iterator::html_node_iterator(const html_node& node): _wrap(node), _parent(node.parent())
{
}

html_node_iterator::html_node_iterator(html_node_struct* ref, html_node_struct* parent): _wrap(ref), _parent(parent)
{
}

bool html_node_iterator::operator==(const html_node_iterator& rhs) const
{
	return _wrap._root == rhs._wrap._root && _parent._root == rhs._parent._root;
}

bool html_node_iterator::operator!=(const html_node_iterator& rhs) const
{
	return _wrap._root != rhs._wrap._root || _parent._root != rhs._parent._root;
}

html_node& html_node_iterator::operator*()
{
	assert(_wrap._root);
	return _wrap;
}

html_node* html_node_iterator::operator->()
{
	assert(_wrap._root);
	return &_wrap;
}

const html_node_iterator& html_node_iterator::operator++()
{
	assert(_wrap._root);
	_wrap._root = _wrap._root->next_sibling;
	return *this;
}

html_node_iterator html_node_iterator::operator++(int)
{
	html_node_iterator temp = *this;
	++*this;
	return temp;
}

const html_node_iterator& html_node_iterator::operator--()
{
	_wrap = _wrap._root ? _wrap.previous_sibling() : _parent.last_child();
	return *this;
}

html_node_iterator html_node_iterator::operator--(int)
{
	html_node_iterator temp = *this;
	--*this;
	return temp;
}


html_tree_walker::html_tree_walker(): _depth(0)
{
}


html_tree_walker::~html_tree_walker()
{
}


int
html_tree_walker::depth() const
{
	return _depth;
}


bool
html_tree_walker::begin(html_node&)
{
	return true;
}


bool
html_tree_walker::end(html_node&)
{
	return true;
}


html_node_struct*
pugihtml::allocate_node(html_allocator& alloc, html_node_type type)
{
	html_memory_page* page;
	void* memory = alloc.allocate_memory(sizeof(html_node_struct), page);

	return new (memory) html_node_struct(page, type);
}


html_node_struct*
pugihtml::append_node(html_node_struct* node, html_allocator& alloc,
	html_node_type type)
{
	html_node_struct* child = allocate_node(alloc, type);
	if (!child) {
		return nullptr;
	}

	child->parent = node;

	html_node_struct* first_child = node->first_child;

	if (first_child) {
		html_node_struct* last_child = first_child->prev_sibling_c;

		last_child->next_sibling = child;
		child->prev_sibling_c = last_child;
		first_child->prev_sibling_c = child;
	}
	else {
		node->first_child = child;
		child->prev_sibling_c = child;
	}

	return child;
}


html_attribute_struct*
pugihtml::append_attribute_ll(html_node_struct* node, html_allocator& alloc)
{
	html_attribute_struct* a = allocate_attribute(alloc);
	if (!a) return 0;

	html_attribute_struct* first_attribute = node->first_attribute;

	if (first_attribute)
	{
		html_attribute_struct* last_attribute =
			first_attribute->prev_attribute_c;

		last_attribute->next_attribute = a;
		a->prev_attribute_c = last_attribute;
		first_attribute->prev_attribute_c = a;
	}
	else
	{
		node->first_attribute = a;
		a->prev_attribute_c = a;
	}

	return a;
}


void
text_output_escaped(html_buffered_writer& writer, const char_t* s, chartypex_t type)
{
	while (*s) {
		const char_t* prev = s;

		// While *s is a usual symbol
		while (!IS_CHARTYPEX(*s, type)) ++s;

		writer.write(prev, static_cast<size_t>(s - prev));

		switch (*s)
		{
			case 0: break;
			case '&':
				writer.write('&', 'a', 'm', 'p', ';');
				++s;
				break;
			case '<':
				writer.write('&', 'l', 't', ';');
				++s;
				break;
			case '>':
				writer.write('&', 'g', 't', ';');
				++s;
				break;
			case '"':
				writer.write('&', 'q', 'u', 'o', 't', ';');
				++s;
				break;
			default: // s is not a usual symbol
			{
				unsigned int ch = static_cast<unsigned int>(*s++);
				assert(ch < 32);

				writer.write('&', '#',
					static_cast<char_t>((ch / 10) + '0'),
					static_cast<char_t>((ch % 10) + '0'),
					';');
			}
		}
	}
}


void
node_output_attributes(html_buffered_writer& writer, const html_node& node)
{
	const char_t* default_name = PUGIHTML_TEXT(":anonymous");

	for (html_attribute a = node.first_attribute(); a; a = a.next_attribute()) {
		writer.write(' ');
		writer.write(a.name()[0] ? a.name() : default_name);
		writer.write('=', '"');

		text_output_escaped(writer, a.value(), ctx_special_attr);

		writer.write('"');
	}
}


void
text_output_cdata(html_buffered_writer& writer, const char_t* s)
{
	do
	{
		writer.write('<', '!', '[', 'C', 'D');
		writer.write('A', 'T', 'A', '[');

		const char_t* prev = s;

		// look for ]]> sequence - we can't output it as is since it terminates CDATA
		while (*s && !(s[0] == ']' && s[1] == ']' && s[2] == '>')) ++s;

		// skip ]] if we stopped at ]]>, > will go to the next CDATA section
		if (*s) s += 2;

		writer.write(prev, static_cast<size_t>(s - prev));

		writer.write(']', ']', '>');
	}
	while (*s);
}


void
pugihtml::node_output(html_buffered_writer& writer, const html_node& node,
	const char_t* indent, unsigned int flags, unsigned int depth)
{
	const char_t* default_name = PUGIHTML_TEXT(":anonymous");

	if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
		for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

	switch (node.type())
	{
	case node_document:
	{
		for (html_node n = node.first_child(); n; n = n.next_sibling())
			node_output(writer, n, indent, flags, depth);
		break;
	}

	case node_element:
	{
		const char_t* name = node.name()[0] ? node.name() : default_name;

		writer.write('<');
		writer.write(name);

		node_output_attributes(writer, node);

		if (flags & format_raw)
		{
			if (!node.first_child())
				writer.write(' ', '/', '>');
			else
			{
				writer.write('>');

				for (html_node n = node.first_child(); n; n = n.next_sibling())
					node_output(writer, n, indent, flags, depth + 1);

				writer.write('<', '/');
				writer.write(name);
				writer.write('>');
			}
		}
		else if (!node.first_child())
			writer.write(' ', '/', '>', '\n');
		else if (node.first_child() == node.last_child() && (node.first_child().type() == node_pcdata || node.first_child().type() == node_cdata))
		{
			writer.write('>');

			if (node.first_child().type() == node_pcdata)
				text_output_escaped(writer, node.first_child().value(), ctx_special_pcdata);
			else
				text_output_cdata(writer, node.first_child().value());

			writer.write('<', '/');
			writer.write(name);
			writer.write('>', '\n');
		}
		else
		{
			writer.write('>', '\n');

			for (html_node n = node.first_child(); n; n = n.next_sibling())
				node_output(writer, n, indent, flags, depth + 1);

			if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
				for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

			writer.write('<', '/');
			writer.write(name);
			writer.write('>', '\n');
		}

		break;
	}

	case node_pcdata:
		text_output_escaped(writer, node.value(), ctx_special_pcdata);
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_cdata:
		text_output_cdata(writer, node.value());
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_comment:
		writer.write('<', '!', '-', '-');
		writer.write(node.value());
		writer.write('-', '-', '>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_pi:
	case node_declaration:
		writer.write('<', '?');
		writer.write(node.name()[0] ? node.name() : default_name);

		if (node.type() == node_declaration)
		{
			node_output_attributes(writer, node);
		}
		else if (node.value()[0])
		{
			writer.write(' ');
			writer.write(node.value());
		}

		writer.write('?', '>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_doctype:
		writer.write('<', '!', 'D', 'O', 'C');
		writer.write('T', 'Y', 'P', 'E');

		if (node.value()[0])
		{
			writer.write(' ');
			writer.write(node.value());
		}

		writer.write('>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	default:
		assert(!"Invalid node type");
	}
}
