#include <string>

#include <gtest/gtest.h>

#include <pugihtml/document.hpp>

#include "parser.hpp"
#include "memory.hpp"
#include "pugihtml.hpp"


using namespace std;
using namespace pugihtml;


TEST(parser, parse)
{
	char str_html[] =
		"<html>"
			"<body>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	document doc;
	html_parse_result res = parser::parse(str_html,
		sizeof(str_html), doc.internal_object());

	string parsed_str = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", parsed_str);
}


TEST(parser, parse_with_void_element_self_closing)
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
	html_parse_result res = parser::parse(str_html,
		sizeof(str_html), doc.internal_object());

	string parsed_str = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", parsed_str);
}


TEST(parser, parse_with_void_element)
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
	html_parse_result res = parser::parse(str_html,
		sizeof(str_html), doc.internal_object());

	string parsed_str = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", parsed_str);
}
