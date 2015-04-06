#ifndef PUGIHTML_NODE_HPP
#define PUGIHTML_NODE_HPP 1

#include <list>
#include <algorithm>
#include <iterator>
#include <memory>
#include <functional>
#include <cstddef>

#include <cpp-html/cpp-html.hpp>
#include <cpp-html/config.hpp>


namespace cpphtml
{

// Tree node types
enum node_type {
	node_null, // Empty (null) node handle
	node_document, // A document tree's absolute root
	node_element, // Element tag, i.e. '<node/>'
	node_attribute,
	node_pcdata, // Plain character data, i.e. 'text'
	node_cdata, // Character data, i.e. '<![CDATA[text]]>'
	node_comment, // Comment tag, i.e. '<!-- text -->'
	node_pi, // Processing instruction, i.e. '<?name?>'
	node_declaration, // Document declaration, i.e. '<?html version="1.0"?>'
	node_doctype // Document type declaration, i.e. '<!DOCTYPE doc>'
};


class node_walker;
class attribute;

/**
 * An HTML document tree node.
 */
class node : public std::enable_shared_from_this<node> {
public:
	/**
	 * Node children interator type.
	 */
	typedef std::list<std::shared_ptr<node> >::iterator iterator;

	/**
	 * Node attribute interator type.
	 */
	typedef std::list<std::shared_ptr<attribute> >::iterator
		attribute_iterator;


	/**
	 * Constructs node with the specified type. Default is pcdata aka text.
	 */
	static std::shared_ptr<node> create(node_type type = node_pcdata);

	/**
	 * @return node type.
	 */
	node_type type() const;

	/**
	 * @return node name. E.g. HTML or BODY, etc.
	 */
	string_type name() const;

	/**
	 * Sets node tag name. Node name is optional. E.g. pcdata nodes
	 * do not have a name.
	 */
	void name(const string_type& name);

	/**
	 * @return text inside node.
	 */
	string_type value() const;

	/**
	 * Change text node value.
	 */
	void value(const string_type& value);

	/**
	 * @return the textual content of the specified node, and all its
	 *	descendants.
	 */
	string_type text_content() const;


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
	std::shared_ptr<attribute> get_attribute(const string_type& name) const;

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
	std::shared_ptr<attribute> append_attribute(
		std::shared_ptr<attribute> attr);

	/**
	 * Prepends new attribute with the specified name to the beginning of
	 * attribute list.
	 *
	 * @param name new attribute name.
	 * @param value new attribute value.
	 * @return pointer to newly added attribute.
	 */
	std::shared_ptr<attribute> prepend_attribute(const string_type& name,
		const string_type& value = "");

	/**
	 * Prepends new attribute to the beginning of attribute list.
	 *
	 * @return pointer to newly added attribute.
	 */
	std::shared_ptr<attribute> prepend_attribute(
		std::shared_ptr<attribute> attr);

	/**
	 * Remove specified attribute if it exists.
	 *
	 * @return true on success, false if attribute with the specified name
	 *	was not found.
	 */
	bool remove_attribute(const string_type& name);

	/**
	 * Find attribute using predicate. Returns first attribute for which
	 * predicate returned true.
	 */
	template <typename Predicate> std::shared_ptr<attribute>
	find_attribute(Predicate pred) const
	{
		auto it_attr = std::find_if(std::begin(this->attributes_),
			std::end(this->attributes_), pred);
		return it_attr == std::end(this->attributes_) ? nullptr
			: *it_attr;
	}


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
	 * Returns empty string, if no PCDATA/CDATA child nodes are found.
	 *
	 * @return value of the first child node of type PCDATA/CDATA.
	 */
	string_type child_value() const;

	/**
	 * @return value of the first child node with the specified name.
	 */
	string_type child_value(const string_type& name) const;

	/**
	 * Append new child node.
	 */
	void append_child(std::shared_ptr<node> _node);

	/**
	 * Prepend new child node.
	 */
	void prepend_child(std::shared_ptr<node> _node);

	/**
	 * Remove child node with the specified name.
	 *
	 * @return true on success, false if such node was not found.
	 */
	bool remove_child(const string_type& name);

	/**
	 * @return immutable list of child nodes.
	 */
	const std::list<std::shared_ptr<node> >& child_nodes() const;

	/**
	 * Find child node using predicate. Returns first child for which
	 * predicate returned true.
	 */
	template <typename Predicate> std::shared_ptr<node>
	find_child(Predicate pred) const
	{
		auto it_node = find_if(this->children_.cbegin(),
			this->children_.cend(), pred);
		return it_node != std::end(this->children_) ? *it_node : nullptr;
	}

	/**
	 * Find node from subtree using predicate. Returns first node from
	 * subtree (depth-first), for which predicate returned true.
	 */
	template <typename Predicate> std::shared_ptr<node>
	find_node(Predicate pred) const
	{
		std::shared_ptr<node> child = this->first_child();
		int traverse_depth = 1;
		while (traverse_depth > 0) {
			if (pred(child)) {
				return child;
			}

			if (child->first_child()) {
				child = child->first_child();
				++traverse_depth;
			}
			else if (child->next_sibling()) {
				child = child->next_sibling();
			}
			else {
				while (!child->next_sibling() &&
					traverse_depth > 0) {
					child = child->parent();
					--traverse_depth;
				}

				if (traverse_depth > 0) {
					child = child->next_sibling();
				}
			}
		}

		return std::shared_ptr<node>(nullptr);
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
	std::shared_ptr<node> find_child_by_attribute(const string_type& tag,
		const string_type& attr_name,
		const string_type& attr_value) const;

	/**
	 * Find child node by attribute name/value. Checks only child nodes.
	 * Does not traverse deeper levels.
	 *
	 * @param attr_name attribute name to check value for.
	 * @param attr_value expected attribute value.
	 * @return html node with the specified tag name and attribute or
	 *	empty node if the specified criteria were not satisfied.
	 */
	std::shared_ptr<node> find_child_by_attribute(
		const string_type& attr_name,
		const string_type& attr_value) const;

	/**
	 * Get the absolute node path from root as a text string.
	 */
	string_type path(char_type delimiter = '/') const;

	/**
	 * Search for a node by path consisting of node names.
	 */
	std::shared_ptr<node> first_element_by_path(const string_type& path,
		char_type delimiter = '/') const;

	/**
	 * Recursively traverse subtree with node_walker.
	 *
	 * @return same as walker.end() return value.
	 */
	bool traverse(node_walker& walker);

	/**
	 * @return node children begin iterator pointing to the first child
	 *	or end() iterator if this node has no children nodes.
	 */
	iterator begin();

	/**
	 * @return node children iterator refering to the past the last
	 *	child node.
	 */
	iterator end();

	attribute_iterator attributes_begin();
	attribute_iterator attributes_end();

	/**
	 * Converts DOM tree back to html string format starting from the
	 * children of current node.
	 */
	string_type to_string(std::size_t indentation = 0) const;

protected:
	/**
	 * Creates node with the specified type.
	 * Prevents from creating non-shared node.
	 */
	node(node_type type = node_pcdata);

private:
	std::weak_ptr<node> parent_;
	// Iterator in parent child nodes. Used for next_sibling(),
	// prev_sibling().
	iterator parent_it_;

	string_type name_;
	string_type value_;
	node_type type_;

	std::list<std::shared_ptr<node> > children_;
	std::list<std::shared_ptr<attribute> > attributes_;
};


/**
 * Abstract DOM tree node walker class (see node::traverse)
 */
class node_walker {
	friend class node;

public:
	node_walker();

	virtual ~node_walker();

	/**
	 * Callback that is called when traversal begins. Always returns true.
	 * Override to change
	 *
	 * @return false if should stop iterating the tree.
	 */
	virtual bool begin(std::shared_ptr<node> node);

	/**
	 * Callback that is called for each node traversed
	 *
	 * @return false if should stop iterating the tree.
	 */
	virtual bool for_each(std::shared_ptr<node> node) = 0;

	/**
	 * Callback that is called when traversal ends.
	 *
	 * @return traversal state: success or failure.
	 */
	virtual bool end(std::shared_ptr<node> node);

protected:
	/**
	 * @return current traversal depth.
	 */
	int depth() const;

private:
	int depth_;
};


typedef std::function<bool (std::shared_ptr<node>, std::size_t)>
	node_walker_callback;

/**
 * Makes node_walker object which implements only for_each method from the
 * specified function object.
 */
std::shared_ptr<node_walker> make_node_walker(node_walker_callback for_each);


}

#endif /* PUGIHTML_NODE_HPP */
