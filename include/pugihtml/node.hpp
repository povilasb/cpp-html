#ifndef PUGIHTML_NODE_HPP
#define PUGIHTML_NODE_HPP 1

#include <memory>
#include <list>
#include <algorithm>

#include <pugihtml/pugihtml.hpp>

#include "common.hpp"
#include "memory.hpp"
#include "attribute.hpp"
#include "html_writer.hpp"


namespace pugihtml
{

// Tree node types
enum node_type {
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


	node_struct* parent;

	char_t* name;
	//Pointer to any associated string data.
	char_t* value;

	node_struct* first_child;

	// Left brother (cyclic list)
	node_struct* prev_sibling_c;
	node_struct* next_sibling;

	attribute_struct* first_attribute;


/**
 * An HTML document tree node.
 */
class node {
public:
	/**
	 * Constructs node with the specified type. Default is pcdata aka text.
	 */
	node(node_type type = node_pcdata);

	/**
	 * @return node type.
	 */
	node_type type() const;

	/**
	 * @return node name. E.g. HTML or BODY, etc.
	 */
	string_type name() const;

	/**
	 * @return text inside node.
	 */
	string_type value() const;

	/**
	 * Change text node value.
	 */
	void value(const string_type& value);


	// Attribute related methods.

	/**
	 * Returns pointer to the first attribute or nullptr, if node has
	 * no attributes.
	 */
	std::shared_ptr<attribute> first_attribute() const;

	/**
	 * Returns pointer to the first attribute or nullptr, if node has
	 * no attributes.
	 */
	std::shared_ptr<attribute> last_attribute() const;

	/**
	 * @return pointer to the attribute with the specified name or nullptr,
	 *	if such attribute does not exist.
	 */
	std::shared_ptr<attribute> attribute(const string_type& name) const;

	/**
	 * Appends new attribute with the specified name to the end of attribute
	 * list.
	 *
	 * @param name new attribute name.
	 * @param value new attribute value.
	 * @return pointer to newly added attribute.
	 */
	std::shared_ptr<attribute> append_attribute(const string_type& name,
		const string_type& value = "");

	/**
	 * Appends new attribute to the end of attribute list.
	 *
	 * @return pointer to newly added attribute.
	 */
	std::shared_ptr<attribute> append_attribute(const attribute& attr);

	/**
	 * Prepends new attribute with the specified name to the beginning of
	 * attribute list.
	 *
	 * @param name new attribute name.
	 * @param value new attribute value.
	 * @return pointer to newly added attribute.
	 */
	std::shared_ptr<attribute> prepend_attribute(const string_type& name,
		const string_type& value);

	/**
	 * Prepends new attribute to the beginning of attribute list.
	 *
	 * @return pointer to newly added attribute.
	 */
	std::shared_ptr<attribute> prepend_attribute(const attribute& attr);

	/**
	 * Remove specified attribute if it exists.
	 *
	 * @return true on success, false if attribute with the specified name
	 *	was not found.
	 */
	bool remove_attribute(const string_type& name);


	// Child nodes related methods.

	/**
	 * @return first child node or nullptr, if no such exist.
	 */
	std::shared_ptr<node> first_child() const;

	/**
	 * @return first child node or nullptr, if no such exist.
	 */
	std::shared_ptr<node> last_child() const;

	/**
	 * @return pointer to the child node with the specified name or nullptr,
	 *	if such does not exist.
	 */
	std::shared_ptr<node> child(const string_type& name) const;

	/**
	 * @return next node in the children list of the parent node.
	 */
	std::shared_ptr<node> next_sibling() const;

	/**
	 * @return next sibling node with the specified name or nullptr, if
	 *	such node does not exist.
	 */
	std::shared_ptr<node> next_sibling(const string_type& name) const;

	/**
	 * @return previous the children list of the parent node.
	 */
	std::shared_ptr<node> previous_sibling() const;

	/**
	 * @return previous sibling node with the specified name or nullptr, if
	 *	such node does not exist.
	 */
	std::shared_ptr<node> previous_sibling(const string_type& name) const;

	/**
	 * @return parent node or nullptr, if there's no parent.
	 */
	std::shared_ptr<node> parent() const;

	/**
	 * @return root of DOM tree this node belongs to.
	 */
	std::shared_ptr<node> root() const;

	/**
	 * @return value of the first child node of type PCDATA/CDATA.
	 */
	string_type child_value() const;

	/**
	 * @return value of the first child node of type PCDATA/CDATA with the
	 *	specified name.
	 */
	string_type child_value(const string_type& name) const;

	/**
	 * Append new child node.
	 */
	void append_child(const node& node);

	/**
	 * Prepend new child node.
	 */
	void prepend_child(const node& node);

	/**
	 * Remove child node with the specified name.
	 *
	 * @return true on success, false if such node was not found.
	 */
	bool remove_child(const string_type& name);

	/**
	 * Find attribute using predicate. Returns first attribute for which
	 * predicate returned true.
	 */
	template <typename Predicate> std::shared_ptr<attribute>
	find_attribute(Predicate pred) const
	{
		return find_if(this->attributes.cbegin(),
			this->attribues.cend(), pred);
	}

	/**
	 * Find child node using predicate. Returns first child for which
	 * predicate returned true.
	 */
	template <typename Predicate> std::shared_ptr<node>
	find_child(Predicate pred) const
	{
		return find_if(this->children.cbegin(), this->children.cend(),
			pred);
	}

	/**
	 * Find node from subtree using predicate. Returns first node from
	 * subtree (depth-first), for which predicate returned true.
	 */
	template <typename Predicate> std::shared_ptr<node>
	find_node(Predicate pred) const
	{
		std::shared_ptr<node> cur = this->first_child();
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

		return node();
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
	node find_child_by_attribute(const char_t* tag,
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
	node find_child_by_attribute(const char_t* attr_name,
		const char_t* attr_value) const;

#ifndef PUGIHTML_NO_STL
	// Get the absolute node path from root as a text string.
	string_t path(char_t delimiter = '/') const;
#endif

	// Search for a node by path consisting of node names and . or .. elements.
	node first_element_by_path(const char_t* path, char_t delimiter = '/') const;

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
	typedef node_iterator iterator;

	iterator begin() const;
	iterator end() const;

	attribute_iterator attributes_begin() const;
	attribute_iterator attributes_end() const;

	// Get node offset in parsed file/string (in char_t units) for debugging purposes
	ptrdiff_t offset_debug() const;

	// Get hash value (unique for handles to the same object)
	size_t hash_value() const;

	// Get internal pointer
	node_struct* internal_object() const;
};

#ifdef __BORLANDC__
// Borland C++ workaround
bool PUGIHTML_FUNCTION operator&&(const node& lhs, bool rhs);
bool PUGIHTML_FUNCTION operator||(const node& lhs, bool rhs);
#endif

// Child node iterator (a bidirectional iterator over a collection of node)
class PUGIHTML_CLASS node_iterator
{
	friend class node;

private:
	node _wrap;
	node _parent;

	node_iterator(node_struct* ref, node_struct* parent);

public:
	// Iterator traits
	typedef ptrdiff_t difference_type;
	typedef node value_type;
	typedef node* pointer;
	typedef node& reference;

#ifndef PUGIHTML_NO_STL
	typedef std::bidirectional_iterator_tag iterator_category;
#endif

	// Default constructor
	node_iterator();

	// Construct an iterator which points to the specified node
	node_iterator(const node& node);

	// Iterator operators
	bool operator==(const node_iterator& rhs) const;
	bool operator!=(const node_iterator& rhs) const;

	node& operator*();
	node* operator->();

	const node_iterator& operator++();
	node_iterator operator++(int);

	const node_iterator& operator--();
	node_iterator operator--(int);
};

// Attribute iterator (a bidirectional iterator over a collection of attribute)
class PUGIHTML_CLASS attribute_iterator
{
	friend class node;

private:
	attribute _wrap;
	node _parent;

	attribute_iterator(attribute_struct* ref, node_struct* parent);

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
	attribute_iterator(const attribute& attr, const node& parent);

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
 * Abstract DOM tree node walker class (see node::traverse)
 */
class node_walker {
	friend class node;

private:
	int _depth;

protected:
	// Get current traversal depth
	int depth() const;

public:
	node_walker();

	virtual ~html_tree_walker();

	/**
	 * Callback that is called when traversal begins. Always returns true.
	 * Override to change
	 *
	 * @return false if should stop iterating the tree.
	 */
	virtual bool begin(node& node);

	/**
	 * Callback that is called for each node traversed
	 *
	 * @return false if should stop iterating the tree.
	 */
	virtual bool for_each(node& node) = 0;

	/**
	 * Callback that is called when traversal ends.
	 *
	 * @return traversal state: success or failure.
	 */
	virtual bool end(node& node);
};


void node_output(html_buffered_writer& writer, const node& node,
	const char_t* indent, unsigned int flags, unsigned int depth);

}

#endif /* PUGIHTML_NODE_HPP */
