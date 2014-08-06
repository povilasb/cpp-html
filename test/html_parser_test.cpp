#include <string>

#include <gtest/gtest.h>

#include <html_parser.hpp>
#include "memory.hpp"
#include <pugihtml.hpp>
#include <document.hpp>


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

	document doc;
	html_parse_result res = html_parser::parse(str_html,
		sizeof(str_html), doc.internal_object());

	string parsed_str = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", parsed_str);
}


TEST(html_parser, parse_with_void_element_self_closing)
{
	char str_html[] =
		"<html>"
			"<body>"
				"<br/>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	document doc;
	html_parse_result res = html_parser::parse(str_html,
		sizeof(str_html), doc.internal_object());

	string parsed_str = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", parsed_str);
}


TEST(html_parser, parse_with_void_element)
{
	char str_html[] =
		"<html>"
			"<body>"
				"<br>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	document doc;
	html_parse_result res = html_parser::parse(str_html,
		sizeof(str_html), doc.internal_object());

	string parsed_str = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", parsed_str);
}
