#ifndef PUGIHTML_HTML_NODE_HPP
#define PUGIHTML_HTML_NODE_HPP 1


#include "common.hpp"
#include "memory.hpp"
#include "html_attribute.hpp"


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

	uintptr_t header;

	html_node_struct* parent;

	char_t* name;
	//Pointer to any associated string data.
	char_t* value;

	html_node_struct* first_child;

	// Left brother (cyclic list)
	html_node_struct* prev_sibling_c;
	html_node_struct* next_sibling;

	html_attribute_struct* first_attribute;
};


html_node_struct* append_node(html_node_struct* node, html_allocator& alloc,
	html_node_type type = node_element);

html_attribute_struct* append_attribute_ll(html_node_struct* node,
	html_allocator& alloc);

}

#endif /* PUGIHTML_HTML_NODE_HPP */
