#ifndef PUGIHTML_HTML_DOCUMENT_HPP
#define PUGIHTML_HTML_DOCUMENT_HPP 1

#include "memory.hpp"
#include "html_node.hpp"


namespace pugihtml
{

struct html_document_struct : public html_node_struct, public html_allocator {
		html_document_struct(html_memory_page* page):
			html_node_struct(page, node_document), html_allocator(page),
			buffer(0)
		{
		}

		const char_t* buffer;
};

} // pugihtml.

#endif /* PUGIHTML_HTML_DOCUMENT_HPP */
