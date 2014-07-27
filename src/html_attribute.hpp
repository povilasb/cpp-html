#ifndef PUGIHTML_HTML_ATTRINBUTE_HPP
#define PUGIHTML_HTML_ATTRINBUTE_HPP 1

#include "memory.hpp"
#include "common.hpp"


namespace pugihtml
{

/**
 * A 'name=value' HTML attribute structure.
 */
struct html_attribute_struct {
	html_attribute_struct(html_memory_page* page)
		: header(reinterpret_cast<uintptr_t>(page)), name(0), value(0),
		prev_attribute_c(0), next_attribute(0)
	{
	}

	uintptr_t header;

	char_t* name; // Pointer to attribute name.
	char_t* value; // Pointer to attribute value.

	// Previous attribute (cyclic list)
	html_attribute_struct* prev_attribute_c;
	html_attribute_struct* next_attribute; // Next attribute
};

} // pugihtml.


#endif /* PUGIHTML_HTML_ATTRINBUTE_HPP */
