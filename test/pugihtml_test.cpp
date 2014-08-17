#include <string>

#include <gtest/gtest.h>

#include <cpp-html/document.hpp>


using namespace std;
namespace html = cpphtml;


TEST(cpp-html, parse_text)
{
	string str_html =
		"<html>"
			"<body>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	html::document doc;
	doc.load(str_html.c_str());
	string str_hello = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", str_hello);
}


TEST(cpp-html, parse_with_void_element)
{
	string str_html =
		"<html>"
			"<body>"
				"<br>"
				"<p>Hello world</p>"
			"</body>"
		"</hmtl>"
	;

	html::document doc;
	doc.load(str_html.c_str());
	string str_hello = doc.child("HTML").child("BODY").child("P")
		.first_child().value();
	ASSERT_EQ("Hello world", str_hello);
}
