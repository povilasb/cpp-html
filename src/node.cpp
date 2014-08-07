#include <new>
#include <cassert>

#include <pugihtml/node.hpp>
#include <pugihtml/document.hpp>
#include <pugihtml/attribute.hpp>

#include "pugiutil.hpp"
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
get_allocator(const node_struct* node)
{
	assert(node);

	return *reinterpret_cast<html_memory_page*>(node->header
		& html_memory_page_pointer_mask)->allocator;
}


inline attribute_struct*
allocate_attribute(html_allocator& alloc)
{
	html_memory_page* page;
	void* memory = alloc.allocate_memory(sizeof(attribute_struct), page);

	return new (memory) attribute_struct(page);
}


inline void
destroy_attribute(attribute_struct* a, html_allocator& alloc)
{
	uintptr_t header = a->header;

	if (header & html_memory_page_name_allocated_mask) alloc.deallocate_string(a->name);
	if (header & html_memory_page_value_allocated_mask) alloc.deallocate_string(a->value);

	alloc.deallocate_memory(a, sizeof(attribute_struct),
		reinterpret_cast<html_memory_page*>(header
		& html_memory_page_pointer_mask));
}


inline void
destroy_node(node_struct* n, html_allocator& alloc)
{
	uintptr_t header = n->header;

	if (header & html_memory_page_name_allocated_mask) alloc.deallocate_string(n->name);
	if (header & html_memory_page_value_allocated_mask) alloc.deallocate_string(n->value);

	for (attribute_struct* attr = n->first_attribute; attr; ) {
		attribute_struct* next = attr->next_attribute;

		destroy_attribute(attr, alloc);

		attr = next;
	}

	for (node_struct* child = n->first_child; child; ) {
		node_struct* next = child->next_sibling;

		destroy_node(child, alloc);

		child = next;
	}

	alloc.deallocate_memory(n, sizeof(node_struct),
		reinterpret_cast<html_memory_page*>(header
		& html_memory_page_pointer_mask));
}


inline bool
allow_insert_child(node_type parent, node_type child)
{
	if (parent != node_document && parent != node_element) return false;
	if (child == node_document || child == node_null) return false;
	if (parent != node_document && (child == node_declaration
		|| child == node_doctype)) return false;

	return true;
}


node::node(): _root(0)
{
}

node::node(node_struct* p): _root(p)
{
}

node::operator node::unspecified_bool_type() const
{
	return _root ? &node::_root : 0;
}

bool node::operator!() const
{
	return !_root;
}

node::iterator node::begin() const
{
	return iterator(_root ? _root->first_child : 0, _root);
}

node::iterator node::end() const
{
	return iterator(0, _root);
}

attribute_iterator node::attributes_begin() const
{
	return attribute_iterator(_root ? _root->first_attribute : 0, _root);
}

attribute_iterator node::attributes_end() const
{
	return attribute_iterator(0, _root);
}

bool node::operator==(const node& r) const
{
	return (_root == r._root);
}

bool node::operator!=(const node& r) const
{
	return (_root != r._root);
}

bool node::operator<(const node& r) const
{
	return (_root < r._root);
}

bool node::operator>(const node& r) const
{
	return (_root > r._root);
}

bool node::operator<=(const node& r) const
{
	return (_root <= r._root);
}

bool
node::operator>=(const node& r) const
{
	return (_root >= r._root);
}


bool
node::empty() const
{
	return !_root;
}


const char_t* node::name() const
{
	return (_root && _root->name) ? _root->name : PUGIHTML_TEXT("");
}

node_type node::type() const
{
	return _root ? static_cast<node_type>((_root->header & html_memory_page_type_mask) + 1) : node_null;
}

const char_t* node::value() const
{
	return (_root && _root->value) ? _root->value : PUGIHTML_TEXT("");
}

node node::child(const char_t* name) const
{
	if (!_root) return node();

	for (node_struct* i = _root->first_child; i; i = i->next_sibling)
		if (i->name && strequal(name, i->name)) return node(i);

	return node();
}

attribute
node::get_attribute(const char_t* name) const
{
	if (!_root) return attribute();

	for (attribute_struct* i = _root->first_attribute; i; i = i->next_attribute)
		if (i->name && strequal(name, i->name))
			return attribute(i);

	return attribute();
}

node node::next_sibling(const char_t* name) const
{
	if (!_root) return node();

	for (node_struct* i = _root->next_sibling; i; i = i->next_sibling)
		if (i->name && strequal(name, i->name)) return node(i);

	return node();
}

node node::next_sibling() const
{
	if (!_root) return node();

	if (_root->next_sibling) return node(_root->next_sibling);
	else return node();
}

node node::previous_sibling(const char_t* name) const
{
	if (!_root) return node();

	for (node_struct* i = _root->prev_sibling_c; i->next_sibling; i = i->prev_sibling_c)
		if (i->name && strequal(name, i->name)) return node(i);

	return node();
}

node node::previous_sibling() const
{
	if (!_root) return node();

	if (_root->prev_sibling_c->next_sibling) return node(_root->prev_sibling_c);
	else return node();
}


node
node::parent() const
{
	return _root ? node(_root->parent) : node();
}


node
node::root() const
{
	if (!_root) return node();

	html_memory_page* page = reinterpret_cast<html_memory_page*>(_root->header & html_memory_page_pointer_mask);

	return node(static_cast<document_struct*>(page->allocator));
}

const char_t* node::child_value() const
{
	if (!_root) return PUGIHTML_TEXT("");

	for (node_struct* i = _root->first_child; i; i = i->next_sibling)
	{
		node_type type = static_cast<node_type>((i->header & html_memory_page_type_mask) + 1);

		if (i->value && (type == node_pcdata || type == node_cdata))
			return i->value;
	}

	return PUGIHTML_TEXT("");
}

const char_t* node::child_value(const char_t* name) const
{
	return child(name).child_value();
}

attribute node::first_attribute() const
{
	return _root ? attribute(_root->first_attribute) : attribute();
}

attribute node::last_attribute() const
{
	return _root && _root->first_attribute ? attribute(_root->first_attribute->prev_attribute_c) : attribute();
}

node node::first_child() const
{
	return _root ? node(_root->first_child) : node();
}

node node::last_child() const
{
	return _root && _root->first_child ? node(_root->first_child->prev_sibling_c) : node();
}

bool node::set_name(const char_t* rhs)
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

bool node::set_value(const char_t* rhs)
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

attribute node::append_attribute(const char_t* name)
{
	if (type() != node_element && type() != node_declaration) return attribute();

	attribute a(append_attribute_ll(_root, get_allocator(_root)));
	a.set_name(name);

	return a;
}

attribute node::prepend_attribute(const char_t* name)
{
	if (type() != node_element && type() != node_declaration) return attribute();

	attribute a(allocate_attribute(get_allocator(_root)));
	if (!a) return attribute();

	a.set_name(name);

	attribute_struct* head = _root->first_attribute;

	if (head)
	{
		a.attr_->prev_attribute_c = head->prev_attribute_c;
		head->prev_attribute_c = a.attr_;
	}
	else
		a.attr_->prev_attribute_c = a.attr_;

	a.attr_->next_attribute = head;
	_root->first_attribute = a.attr_;

	return a;
}

attribute node::insert_attribute_before(const char_t* name, const attribute& attr)
{
	if ((type() != node_element && type() != node_declaration) || attr.empty()) return attribute();

	// check that attribute belongs to *this
	attribute_struct* cur = attr.attr_;

	while (cur->prev_attribute_c->next_attribute) cur = cur->prev_attribute_c;

	if (cur != _root->first_attribute) return attribute();

	attribute a(allocate_attribute(get_allocator(_root)));
	if (!a) return attribute();

	a.set_name(name);

	if (attr.attr_->prev_attribute_c->next_attribute)
		attr.attr_->prev_attribute_c->next_attribute = a.attr_;
	else
		_root->first_attribute = a.attr_;

	a.attr_->prev_attribute_c = attr.attr_->prev_attribute_c;
	a.attr_->next_attribute = attr.attr_;
	attr.attr_->prev_attribute_c = a.attr_;

	return a;
}

attribute node::insert_attribute_after(const char_t* name, const attribute& attr)
{
	if ((type() != node_element && type() != node_declaration) || attr.empty()) return attribute();

	// check that attribute belongs to *this
	attribute_struct* cur = attr.attr_;

	while (cur->prev_attribute_c->next_attribute) cur = cur->prev_attribute_c;

	if (cur != _root->first_attribute) return attribute();

	attribute a(allocate_attribute(get_allocator(_root)));
	if (!a) return attribute();

	a.set_name(name);

	if (attr.attr_->next_attribute)
		attr.attr_->next_attribute->prev_attribute_c = a.attr_;
	else
		_root->first_attribute->prev_attribute_c = a.attr_;

	a.attr_->next_attribute = attr.attr_->next_attribute;
	a.attr_->prev_attribute_c = attr.attr_;
	attr.attr_->next_attribute = a.attr_;

	return a;
}

attribute node::append_copy(const attribute& proto)
{
	if (!proto) return attribute();

	attribute result = append_attribute(proto.name());
	result.set_value(proto.value());

	return result;
}

attribute node::prepend_copy(const attribute& proto)
{
	if (!proto) return attribute();

	attribute result = prepend_attribute(proto.name());
	result.set_value(proto.value());

	return result;
}

attribute node::insert_copy_after(const attribute& proto, const attribute& attr)
{
	if (!proto) return attribute();

	attribute result = insert_attribute_after(proto.name(), attr);
	result.set_value(proto.value());

	return result;
}

attribute node::insert_copy_before(const attribute& proto, const attribute& attr)
{
	if (!proto) return attribute();

	attribute result = insert_attribute_before(proto.name(), attr);
	result.set_value(proto.value());

	return result;
}

node node::append_child(node_type type)
{
	if (!allow_insert_child(this->type(), type)) return node();

	node n(append_node(_root, get_allocator(_root), type));

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}

node node::prepend_child(node_type type)
{
	if (!allow_insert_child(this->type(), type)) return node();

	node n(allocate_node(get_allocator(_root), type));
	if (!n) return node();

	n._root->parent = _root;

	node_struct* head = _root->first_child;

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

node
node::insert_child_before(node_type type, const node& _node)
{
	if (!allow_insert_child(this->type(), type)) return node();
	if (!_node._root || _node._root->parent != _root) return node();

	node n(allocate_node(get_allocator(_root), type));
	if (!n) return node();

	n._root->parent = _root;

	if (_node._root->prev_sibling_c->next_sibling)
		_node._root->prev_sibling_c->next_sibling = n._root;
	else
		_root->first_child = n._root;

	n._root->prev_sibling_c = _node._root->prev_sibling_c;
	n._root->next_sibling = _node._root;
	_node._root->prev_sibling_c = n._root;

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}


node
node::insert_child_after(node_type type, const node& _node)
{
	if (!allow_insert_child(this->type(), type)) return node();
	if (!_node._root || _node._root->parent != _root) return node();

	node n(allocate_node(get_allocator(_root), type));
	if (!n) return node();

	n._root->parent = _root;

	if (_node._root->next_sibling)
		_node._root->next_sibling->prev_sibling_c = n._root;
	else
		_root->first_child->prev_sibling_c = n._root;

	n._root->next_sibling = _node._root->next_sibling;
	n._root->prev_sibling_c = _node._root;
	_node._root->next_sibling = n._root;

	if (type == node_declaration) n.set_name(PUGIHTML_TEXT("html"));

	return n;
}


node
node::append_child(const char_t* name)
{
	node result = append_child(node_element);

	result.set_name(name);

	return result;
}

node node::prepend_child(const char_t* name)
{
	node result = prepend_child(node_element);

	result.set_name(name);

	return result;
}


node
node::insert_child_after(const char_t* name, const node& _node)
{
	node result = insert_child_after(node_element, _node);

	result.set_name(name);

	return result;
}


node
node::insert_child_before(const char_t* name, const node& _node)
{
	node result = insert_child_before(node_element, _node);
	result.set_name(name);

	return result;
}


void recursive_copy_skip(node& dest, const node& source, const node& skip)
{
	assert(dest.type() == source.type());

	switch (source.type())
	{
	case node_element:
	{
		dest.set_name(source.name());

		for (attribute a = source.first_attribute(); a; a = a.next_attribute())
			dest.append_attribute(a.name()).set_value(a.value());

		for (node c = source.first_child(); c; c = c.next_sibling())
		{
			if (c == skip) continue;

			node cc = dest.append_child(c.type());
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

		for (attribute a = source.first_attribute(); a; a = a.next_attribute())
			dest.append_attribute(a.name()).set_value(a.value());

		break;
	}

	default:
		assert(!"Invalid node type");
	}
}


node node::append_copy(const node& proto)
{
	node result = append_child(proto.type());

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}

node node::prepend_copy(const node& proto)
{
	node result = prepend_child(proto.type());

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}

node
node::insert_copy_after(const node& proto, const node& _node)
{
	node result = insert_child_after(proto.type(), _node);

	if (result) {
		recursive_copy_skip(result, proto, result);
	}

	return result;
}


node
node::insert_copy_before(const node& proto, const node& _node)
{
	node result = insert_child_before(proto.type(), _node);

	if (result) recursive_copy_skip(result, proto, result);

	return result;
}


bool
node::remove_attribute(const char_t* name)
{
	return remove_attribute(this->get_attribute(name));
}

bool node::remove_attribute(const attribute& a)
{
	if (!_root || !a.attr_) return false;

	// check that attribute belongs to *this
	attribute_struct* attr = a.attr_;

	while (attr->prev_attribute_c->next_attribute) attr = attr->prev_attribute_c;

	if (attr != _root->first_attribute) return false;

	if (a.attr_->next_attribute) a.attr_->next_attribute->prev_attribute_c = a.attr_->prev_attribute_c;
	else if (_root->first_attribute) _root->first_attribute->prev_attribute_c = a.attr_->prev_attribute_c;

	if (a.attr_->prev_attribute_c->next_attribute) a.attr_->prev_attribute_c->next_attribute = a.attr_->next_attribute;
	else _root->first_attribute = a.attr_->next_attribute;

	destroy_attribute(a.attr_, get_allocator(_root));

	return true;
}

bool node::remove_child(const char_t* name)
{
	return remove_child(child(name));
}

bool node::remove_child(const node& n)
{
	if (!_root || !n._root || n._root->parent != _root) return false;

	if (n._root->next_sibling) n._root->next_sibling->prev_sibling_c = n._root->prev_sibling_c;
	else if (_root->first_child) _root->first_child->prev_sibling_c = n._root->prev_sibling_c;

	if (n._root->prev_sibling_c->next_sibling) n._root->prev_sibling_c->next_sibling = n._root->next_sibling;
	else _root->first_child = n._root->next_sibling;

	destroy_node(n._root, get_allocator(_root));

	return true;
}


node
node::find_child_by_attribute(const char_t* name, const char_t* attr_name,
	const char_t* attr_value) const
{
	if (this->empty()) {
		return node();
	}

	for (node_struct* curr_node = this->_root->first_child; curr_node;
		curr_node = curr_node->next_sibling) {
		if (curr_node->name && strequal(name, curr_node->name)) {
			for (attribute_struct* a = curr_node->first_attribute;
				a; a = a->next_attribute)
				if (strequal(attr_name, a->name) &&
					strequal(attr_value, a->value)) {
					return node(curr_node);
				}
		}
	}

	return node();
}


node
node::find_child_by_attribute(const char_t* attr_name,
	const char_t* attr_value) const
{
	if (this->empty()) {
		return node();
	}

	for (node_struct* curr_node = _root->first_child; curr_node;
		curr_node = curr_node->next_sibling) {
		for (attribute_struct* a = curr_node->first_attribute; a;
			a = a->next_attribute) {
			if (strequal(attr_name, a->name) &&
				strequal(attr_value, a->value))
				return node(curr_node);
		}
	}

	return node();
}

#ifndef PUGIHTML_NO_STL
string_t
node::path(char_t delimiter) const
{
	string_t path;

	node cursor = *this; // Make a copy.

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

node node::first_element_by_path(const char_t* path, char_t delimiter) const
{
	node found = *this; // Current search context.

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
		for (node_struct* j = found._root->first_child; j; j = j->next_sibling)
		{
			if (j->name && strequalrange(j->name, path_segment, static_cast<size_t>(path_segment_end - path_segment)))
			{
				node subsearch = node(j).first_element_by_path(next_segment, delimiter);

				if (subsearch) return subsearch;
			}
		}

		return node();
	}
}

bool
node::traverse(html_tree_walker& walker)
{
	walker._depth = -1;

	node arg_begin = *this;
	if (!walker.begin(arg_begin)) return false;

	node cur = first_child();

	if (cur) {
		++walker._depth;

		do {
			node arg_for_each = cur;
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

	node arg_end = *this;
	return walker.end(arg_end);
}

size_t node::hash_value() const
{
	return static_cast<size_t>(reinterpret_cast<uintptr_t>(_root) / sizeof(node_struct));
}

node_struct* node::internal_object() const
{
	return _root;
}

void
node::print(html_writer& writer, const char_t* indent, unsigned int flags,
	html_encoding encoding, unsigned int depth) const
{
	if (!_root) return;

	html_buffered_writer buffered_writer(writer, encoding);

	node_output(buffered_writer, *this, indent, flags, depth);
}

#ifndef PUGIHTML_NO_STL
void node::print(std::basic_ostream<char, std::char_traits<char> >& stream, const char_t* indent, unsigned int flags, html_encoding encoding, unsigned int depth) const
{
	html_writer_stream writer(stream);

	print(writer, indent, flags, encoding, depth);
}

void node::print(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream, const char_t* indent, unsigned int flags, unsigned int depth) const
{
	html_writer_stream writer(stream);

	print(writer, indent, flags, encoding_wchar, depth);
}
#endif

ptrdiff_t node::offset_debug() const
{
	node_struct* r = root()._root;

	if (!r) return -1;

	const char_t* buffer = static_cast<document_struct*>(r)->buffer;

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
bool operator&&(const node& lhs, bool rhs)
{
	return (bool)lhs && rhs;
}

bool operator||(const node& lhs, bool rhs)
{
	return (bool)lhs || rhs;
}
#endif


node_iterator::node_iterator()
{
}

node_iterator::node_iterator(const node& node): _wrap(node), _parent(node.parent())
{
}

node_iterator::node_iterator(node_struct* ref, node_struct* parent): _wrap(ref), _parent(parent)
{
}

bool node_iterator::operator==(const node_iterator& rhs) const
{
	return _wrap._root == rhs._wrap._root && _parent._root == rhs._parent._root;
}

bool node_iterator::operator!=(const node_iterator& rhs) const
{
	return _wrap._root != rhs._wrap._root || _parent._root != rhs._parent._root;
}

node& node_iterator::operator*()
{
	assert(_wrap._root);
	return _wrap;
}

node* node_iterator::operator->()
{
	assert(_wrap._root);
	return &_wrap;
}

const node_iterator& node_iterator::operator++()
{
	assert(_wrap._root);
	_wrap._root = _wrap._root->next_sibling;
	return *this;
}

node_iterator node_iterator::operator++(int)
{
	node_iterator temp = *this;
	++*this;
	return temp;
}

const node_iterator& node_iterator::operator--()
{
	_wrap = _wrap._root ? _wrap.previous_sibling() : _parent.last_child();
	return *this;
}

node_iterator node_iterator::operator--(int)
{
	node_iterator temp = *this;
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
html_tree_walker::begin(node&)
{
	return true;
}


bool
html_tree_walker::end(node&)
{
	return true;
}


node_struct*
pugihtml::allocate_node(html_allocator& alloc, node_type type)
{
	html_memory_page* page;
	void* memory = alloc.allocate_memory(sizeof(node_struct), page);

	return new (memory) node_struct(page, type);
}


node_struct*
pugihtml::append_node(node_struct* node, html_allocator& alloc,
	node_type type)
{
	node_struct* child = allocate_node(alloc, type);
	if (!child) {
		return nullptr;
	}

	child->parent = node;

	node_struct* first_child = node->first_child;

	if (first_child) {
		node_struct* last_child = first_child->prev_sibling_c;

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


attribute_struct*
pugihtml::append_attribute_ll(node_struct* node, html_allocator& alloc)
{
	attribute_struct* a = allocate_attribute(alloc);
	if (!a) return 0;

	attribute_struct* first_attribute = node->first_attribute;

	if (first_attribute)
	{
		attribute_struct* last_attribute =
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
node_output_attributes(html_buffered_writer& writer, const node& node)
{
	const char_t* default_name = PUGIHTML_TEXT(":anonymous");

	for (attribute a = node.first_attribute(); a; a = a.next_attribute()) {
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
pugihtml::node_output(html_buffered_writer& writer, const node& _node,
	const char_t* indent, unsigned int flags, unsigned int depth)
{
	const char_t* default_name = PUGIHTML_TEXT(":anonymous");

	if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
		for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

	switch (_node.type())
	{
	case node_document:
	{
		for (node n = _node.first_child(); n; n = n.next_sibling())
			node_output(writer, n, indent, flags, depth);
		break;
	}

	case node_element:
	{
		const char_t* name = _node.name()[0] ? _node.name() : default_name;

		writer.write('<');
		writer.write(name);

		node_output_attributes(writer, _node);

		if (flags & format_raw)
		{
			if (!_node.first_child())
				writer.write(' ', '/', '>');
			else
			{
				writer.write('>');

				for (node n = _node.first_child(); n; n = n.next_sibling())
					node_output(writer, n, indent, flags, depth + 1);

				writer.write('<', '/');
				writer.write(name);
				writer.write('>');
			}
		}
		else if (!_node.first_child()) {
			writer.write(' ', '/', '>', '\n');
		}
		else if (_node.first_child() == _node.last_child() &&
			(_node.first_child().type() == node_pcdata ||
			_node.first_child().type() == node_cdata)) {

			writer.write('>');

			if (_node.first_child().type() == node_pcdata)
				text_output_escaped(writer, _node.first_child().value(), ctx_special_pcdata);
			else
				text_output_cdata(writer,
					 _node.first_child().value());

			writer.write('<', '/');
			writer.write(name);
			writer.write('>', '\n');
		}
		else {
			writer.write('>', '\n');

			for (node n = _node.first_child(); n; n = n.next_sibling())
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
		text_output_escaped(writer, _node.value(), ctx_special_pcdata);
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_cdata:
		text_output_cdata(writer, _node.value());
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_comment:
		writer.write('<', '!', '-', '-');
		writer.write(_node.value());
		writer.write('-', '-', '>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_pi:
	case node_declaration:
		writer.write('<', '?');
		writer.write(_node.name()[0] ? _node.name() : default_name);

		if (_node.type() == node_declaration) {
			node_output_attributes(writer, _node);
		}
		else if (_node.value()[0])
		{
			writer.write(' ');
			writer.write(_node.value());
		}

		writer.write('?', '>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_doctype:
		writer.write('<', '!', 'D', 'O', 'C');
		writer.write('T', 'Y', 'P', 'E');

		if (_node.value()[0])
		{
			writer.write(' ');
			writer.write(_node.value());
		}

		writer.write('>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	default:
		assert(!"Invalid node type");
	}
}
