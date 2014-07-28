#include <string>

#include <gtest/gtest.h>

#include <html_parser.hpp>
#include "memory.hpp"
#include <html_node.hpp>


using namespace std;
using namespace pugihtml;


TEST(html_parser, DISABLED_parse)
{
	char str_html[] =
		"<html>"
			"<body>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	//void* page_memory = reinterpret_cast<void*>(
	//(reinterpret_cast<uintptr_t>(_memory)
	// + (html_memory_page_alignment - 1))
	//& ~(html_memory_page_alignment - 1));

/*
	char page_memory[32768];
	html_memory_page* mem_page = html_memory_page::construct(&page_memory);
	ASSERT_NE(nullptr, mem_page);
*/

	html_allocator allocator(nullptr);
	html_node_struct* node = pugihtml::allocate_node(allocator,
		node_document);
	ASSERT_NE(node, nullptr);

	html_parse_result res = html_parser::parse(str_html,
		sizeof(str_html), node);

	cout << node->name << endl;
	ASSERT_EQ("HTML", node->name);
}
