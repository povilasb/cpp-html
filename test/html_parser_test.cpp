#include <string>

#include <gtest/gtest.h>

#include <html_parser.hpp>
#include "memory.hpp"
#include <html_node.hpp>


using namespace std;
using namespace pugihtml;


TEST(html_parser, parse)
{
	char str_html[] =
		"<html>"
			"<body>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	html_allocator& allocator(mem_page);
	html_node_struct* node = html_node::allocate_node(allocator,
		node_document);
	html_parse_result res = html_parser::parse(str_html,
		sizeof(str_html), node);
}
