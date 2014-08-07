#ifndef PUGIHTML_HTML_NODE_HPP
#define PUGIHTML_HTML_NODE_HPP 1

#include <cstddef>

#include "common.hpp"
#include "memory.hpp"
#include "attribute.hpp"
#include "html_writer.hpp"


namespace pugihtml
{

// Tree node types
enum html_node_type {
	node_null, // Empty (null) node handle
	node_document, // A document tree's absolute root
	node_element, // Element tag, i.e. '<node/>'
	node_pcdata, // Plain character data, i.e. 'text'
	node_cdata, // Character data, i.e. '<![CDATA[text]]>'
	node_comment, // Comment tag, i.e. '<!-- text -->'
	node_pi, // Processing instruction, i.e. '<?name?>'
	node_declaration, // Document declaration, i.e. '<?html version="1.0"?>'
	node_doctype // Document type declaration, i.e. '<!DOCTYPE doc>'
};

/**
 * An HTML document tree node.
 */
// TODO(povilas): consider if reinterpret_cast is really neccessary here.
struct html_node_struct {
	html_node_struct(html_memory_page* page, html_node_type type)
		: header(reinterpret_cast<uintptr_t>(page) | (type - 1)),
		parent(0), name(0), value(0), first_child(0), prev_sibling_c(0),
		next_sibling(0), first_attribute(0)
	{
	}

	// Pointer to memory page in which this node resides.
	uintptr_t header;

	html_node_struct* parent;

	char_t* name;
	//Pointer to any associated string data.
	char_t* value;

	html_node_struct* first_child;

	// Left brother (cyclic list)
	html_node_struct* prev_sibling_c;
	html_node_struct* next_sibling;

	attribute_struct* first_attribute;
};


class html_node_iterator;
class html_tree_walker;

/**
 * A light-weight handle for manipulating nodes in DOM tree.
 */
class PUGIHTML_CLASS html_node {
	friend class attribute_iterator;
	friend class html_node_iterator;

protected:
	html_node_struct* _root;

	typedef html_node_struct* html_node::*unspecified_bool_type;

public:
	/**
	 * Constructs an empty node.
	 */
	html_node();

	// Constructs node from internal pointer
	explicit html_node(html_node_struct* p);

	// Safe bool conversion operator
	operator unspecified_bool_type() const;

	// Borland C++ workaround
	bool operator!() const;

	// Comparison operators (compares wrapped node pointers)
	// TODO(povilas): consider is this is needed.
	bool operator==(const html_node& r) const;
	bool operator!=(const html_node& r) const;
	bool operator<(const html_node& r) const;
	bool operator>(const html_node& r) const;
	bool operator<=(const html_node& r) const;
	bool operator>=(const html_node& r) const;

	/**
	 * Check if node is empty.
	 */
	bool empty() const;

	// Get node type
	html_node_type type() const;

	// Get node name/value, or "" if node is empty or it has no name/value
	const char_t* name() const;
	const char_t* value() const;

	// Get attribute list
	attribute first_attribute() const;
	attribute last_attribute() const;

	// Get children list
	html_node first_child() const;
	html_node last_child() const;

	// Get next/previous sibling in the children list of the parent node
	html_node next_sibling() const;
	html_node previous_sibling() const;

	/**
	 * @return parent node or empty node if there's no parent.
	 */
	html_node parent() const;

	// Get root of DOM tree this node belongs to
	html_node root() const;

	// Get child, attribute or next/previous sibling with the specified name
	html_node child(const char_t* name) const;
	attribute get_attribute(const char_t* name) const;
	html_node next_sibling(const char_t* name) const;
	html_node previous_sibling(const char_t* name) const;

	// Get child value of current node; that is, value of the first child node of type PCDATA/CDATA
	const char_t* child_value() const;

	// Get child value of child with specified name. Equivalent to child(name).child_value().
	const char_t* child_value(const char_t* name) const;

	// Set node name/value (returns false if node is empty, there is not enough memory, or node can not have name/value)
	bool set_name(const char_t* rhs);
	bool set_value(const char_t* rhs);

	// Add attribute with specified name. Returns added attribute, or empty attribute on errors.
	attribute append_attribute(const char_t* name);
	attribute prepend_attribute(const char_t* name);
	attribute insert_attribute_after(const char_t* name, const attribute& attr);
	attribute insert_attribute_before(const char_t* name, const attribute& attr);

	// Add a copy of the specified attribute. Returns added attribute, or empty attribute on errors.
	attribute append_copy(const attribute& proto);
	attribute prepend_copy(const attribute& proto);
	attribute insert_copy_after(const attribute& proto, const attribute& attr);
	attribute insert_copy_before(const attribute& proto, const attribute& attr);

	// Add child node with specified type. Returns added node, or empty node on errors.
	html_node append_child(html_node_type type = node_element);
	html_node prepend_child(html_node_type type = node_element);
	html_node insert_child_after(html_node_type type, const html_node& node);
	html_node insert_child_before(html_node_type type, const html_node& node);

	// Add child element with specified name. Returns added node, or empty node on errors.
	html_node append_child(const char_t* name);
	html_node prepend_child(const char_t* name);
	html_node insert_child_after(const char_t* name, const html_node& node);
	html_node insert_child_before(const char_t* name, const html_node& node);

	// Add a copy of the specified node as a child. Returns added node, or empty node on errors.
	html_node append_copy(const html_node& proto);
	html_node prepend_copy(const html_node& proto);
	html_node insert_copy_after(const html_node& proto, const html_node& node);
	html_node insert_copy_before(const html_node& proto, const html_node& node);

	// Remove specified attribute
	bool remove_attribute(const attribute& a);
	bool remove_attribute(const char_t* name);

	// Remove specified child
	bool remove_child(const html_node& n);
	bool remove_child(const char_t* name);

	// Find attribute using predicate. Returns first attribute for which predicate returned true.
	template <typename Predicate> attribute find_attribute(Predicate pred) const
	{
		if (!_root) return attribute();

		for (attribute attrib = first_attribute(); attrib; attrib = attrib.next_attribute())
			if (pred(attrib))
				return attrib;

		return attribute();
	}

	// Find child node using predicate. Returns first child for which predicate returned true.
	template <typename Predicate> html_node find_child(Predicate pred) const
	{
		if (!_root) return html_node();

		for (html_node node = first_child(); node; node = node.next_sibling())
			if (pred(node))
				return node;

		return html_node();
	}

	/**
	 * Find node from subtree using predicate. Returns first node from
	 * subtree (depth-first), for which predicate returned true.
	 */
	template <typename Predicate> html_node
	find_node(Predicate pred) const {
		if (this->empty()) {
			return html_node();
		}

		html_node cur = first_child();
		while (cur._root && cur._root != this->_root) {
			if (pred(cur)) {
				return cur;
			}

			if (cur.first_child()) {
				cur = cur.first_child();
			}
			else if (cur.next_sibling()) {
				cur = cur.next_sibling();
			}
			else {
				while (!cur.next_sibling() &&
					cur._root != this->_root) {
					cur = cur.parent();
				}

				if (cur._root != this->_root) {
					cur = cur.next_sibling();
				}
			}
		}

		return html_node();
	}

	/**
	 * Find child node by attribute name/value. Checks only the specified
	 * tag nodes.
	 * Checks only child nodes. Does not traverse deeper levels.
	 *
	 * @param tag node tag name to check for attribute.
	 * @param attr_name attribute name to check value for.
	 * @param attr_value expected attribute value.
	 * @return html node with the specified tag name and attribute or
	 *	empty node if the specified criteria were not satisfied.
	 */
	html_node find_child_by_attribute(const char_t* tag,
		const char_t* attr_name, const char_t* attr_value) const;

	/**
	 * Find child node by attribute name/value. Checks only child nodes.
	 * Does not traverse deeper levels.
	 *
	 * @param attr_name attribute name to check value for.
	 * @param attr_value expected attribute value.
	 * @return html node with the specified tag name and attribute or
	 *	empty node if the specified criteria were not satisfied.
	 */
	html_node find_child_by_attribute(const char_t* attr_name,
		const char_t* attr_value) const;

#ifndef PUGIHTML_NO_STL
	// Get the absolute node path from root as a text string.
	string_t path(char_t delimiter = '/') const;
#endif

	// Search for a node by path consisting of node names and . or .. elements.
	html_node first_element_by_path(const char_t* path, char_t delimiter = '/') const;

	// Recursively traverse subtree with html_tree_walker
	bool traverse(html_tree_walker& walker);

#ifndef PUGIHTML_NO_XPATH
	// Select single node by evaluating XPath query. Returns first node from the resulting node set.
	xpath_node select_single_node(const char_t* query, xpath_variable_set* variables = 0) const;
	xpath_node select_single_node(const xpath_query& query) const;

	// Select node set by evaluating XPath query
	xpath_node_set select_nodes(const char_t* query, xpath_variable_set* variables = 0) const;
	xpath_node_set select_nodes(const xpath_query& query) const;
#endif

	// Print subtree using a writer object
	void print(html_writer& writer, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, html_encoding encoding = encoding_auto, unsigned int depth = 0) const;

#ifndef PUGIHTML_NO_STL
	// Print subtree to stream
	void print(std::basic_ostream<char, std::char_traits<char> >& os, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, html_encoding encoding = encoding_auto, unsigned int depth = 0) const;
	void print(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& os, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, unsigned int depth = 0) const;
#endif

	// Child nodes iterators
	typedef html_node_iterator iterator;

	iterator begin() const;
	iterator end() const;

	attribute_iterator attributes_begin() const;
	attribute_iterator attributes_end() const;

	// Get node offset in parsed file/string (in char_t units) for debugging purposes
	ptrdiff_t offset_debug() const;

	// Get hash value (unique for handles to the same object)
	size_t hash_value() const;

	// Get internal pointer
	html_node_struct* internal_object() const;
};

#ifdef __BORLANDC__
// Borland C++ workaround
bool PUGIHTML_FUNCTION operator&&(const html_node& lhs, bool rhs);
bool PUGIHTML_FUNCTION operator||(const html_node& lhs, bool rhs);
#endif

// Child node iterator (a bidirectional iterator over a collection of html_node)
class PUGIHTML_CLASS html_node_iterator
{
	friend class html_node;

private:
	html_node _wrap;
	html_node _parent;

	html_node_iterator(html_node_struct* ref, html_node_struct* parent);

public:
	// Iterator traits
	typedef ptrdiff_t difference_type;
	typedef html_node value_type;
	typedef html_node* pointer;
	typedef html_node& reference;

#ifndef PUGIHTML_NO_STL
	typedef std::bidirectional_iterator_tag iterator_category;
#endif

	// Default constructor
	html_node_iterator();

	// Construct an iterator which points to the specified node
	html_node_iterator(const html_node& node);

	// Iterator operators
	bool operator==(const html_node_iterator& rhs) const;
	bool operator!=(const html_node_iterator& rhs) const;

	html_node& operator*();
	html_node* operator->();

	const html_node_iterator& operator++();
	html_node_iterator operator++(int);

	const html_node_iterator& operator--();
	html_node_iterator operator--(int);
};

// Attribute iterator (a bidirectional iterator over a collection of attribute)
class PUGIHTML_CLASS attribute_iterator
{
	friend class html_node;

private:
	attribute _wrap;
	html_node _parent;

	attribute_iterator(attribute_struct* ref, html_node_struct* parent);

public:
	// Iterator traits
	typedef ptrdiff_t difference_type;
	typedef attribute value_type;
	typedef attribute* pointer;
	typedef attribute& reference;

#ifndef PUGIHTML_NO_STL
	typedef std::bidirectional_iterator_tag iterator_category;
#endif

	// Default constructor
	attribute_iterator();

	// Construct an iterator which points to the specified attribute
	attribute_iterator(const attribute& attr, const html_node& parent);

	// Iterator operators
	bool operator==(const attribute_iterator& rhs) const;
	bool operator!=(const attribute_iterator& rhs) const;

	attribute& operator*();
	attribute* operator->();

	const attribute_iterator& operator++();
	attribute_iterator operator++(int);

	const attribute_iterator& operator--();
	attribute_iterator operator--(int);
};


/**
 * Abstract tree walker class (see html_node::traverse)
 */
class PUGIHTML_CLASS html_tree_walker {
	friend class html_node;

private:
	int _depth;

protected:
	// Get current traversal depth
	int depth() const;

public:
	html_tree_walker();

	virtual ~html_tree_walker();

	/**
	 * Callback that is called when traversal begins. Always returns true.
	 * Override to change
	 *
	 * @return false if should stop iterating the tree.
	 */
	virtual bool begin(html_node& node);

	/**
	 * Callback that is called for each node traversed
	 *
	 * @return false if should stop iterating the tree.
	 */
	virtual bool for_each(html_node& node) = 0;

	/**
	 * Callback that is called when traversal ends.
	 *
	 * @return traversal state: success or failure.
	 */
	virtual bool end(html_node& node);
};


html_node_struct* append_node(html_node_struct* node, html_allocator& alloc,
	html_node_type type = node_element);

attribute_struct* append_attribute_ll(html_node_struct* node,
	html_allocator& alloc);

html_node_struct* allocate_node(html_allocator& alloc, html_node_type type);

void node_output(html_buffered_writer& writer, const html_node& node,
	const char_t* indent, unsigned int flags, unsigned int depth);

}

#endif /* PUGIHTML_HTML_NODE_HPP */
