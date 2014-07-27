#ifndef PUGIHTML_HTML_NODE_HPP
#define PUGIHTML_HTML_NODE_HPP 1


#include "common.hpp"


namespace pugihtml
{

/**
 * An HTML document tree node.
 */
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

}

#endif /* PUGIHTML_HTML_NODE_HPP */
