#include <new>

#include "html_node.hpp"
#include "html_attribute.hpp"


using namespace pugihtml;


inline html_node_struct*
allocate_node(html_allocator& alloc, html_node_type type)
{
	html_memory_page* page;
	void* memory = alloc.allocate_memory(sizeof(html_node_struct), page);

	return new (memory) html_node_struct(page, type);
}


html_node_struct*
append_node(html_node_struct* node, html_allocator& alloc,
	html_node_type type = node_element)
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


inline html_attribute_struct*
allocate_attribute(html_allocator& alloc)
{
	html_memory_page* page;
	void* memory = alloc.allocate_memory(sizeof(html_attribute_struct), page);

	return new (memory) html_attribute_struct(page);
}


html_attribute_struct*
append_attribute_ll(html_node_struct* node, html_allocator& alloc)
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
