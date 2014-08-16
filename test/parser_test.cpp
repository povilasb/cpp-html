#include <gtest/gtest.h>

#include <pugihtml/parser.hpp>
#include <pugihtml/attribute.hpp>

namespace html = pugihtml;


TEST(parser, advance_doctype_primitive)
{
	html::char_type html[] = "<!-- html comment -->";
	auto doctype_end = html::parser::advance_doctype_primitive(html);
	ASSERT_EQ('\0', doctype_end[0]);
}


TEST(parser, advance_doctype_ignore)
{
	html::char_type html[] = "<![ unknown doctype ]]><html></html>";
	auto doctype_end = html::parser::advance_doctype_ignore(html);
	ASSERT_EQ('<', doctype_end[0]);
}


TEST(parser, advance_doctype_group)
{
	html::char_type html[] = "<!DOCTYPE <![ metadata ]]> ><html></html>";
	auto doctype_end = html::parser::advance_doctype_group(html, '>');
	ASSERT_EQ('<', doctype_end[0]);
}


TEST(parser, parse_exclamation_comment)
{
	html::parser parser(html::parser::parse_comments);

	html::char_type html[] = "<!--html comment-->";
	auto comment_end = parser.parse_exclamation(html);
	ASSERT_EQ('\0', comment_end[0]);

	auto doc = parser.get_document();
	ASSERT_NE(nullptr, doc);

	auto comment = doc->first_child();
	ASSERT_NE(nullptr, comment);
	ASSERT_EQ("html comment", comment->value());
}


TEST(parser, parse_exclamation_doctype)
{
	html::parser parser(html::parser::parse_doctype);

	html::char_type html[] = "<!DOCTYPE html>";
	auto doctype_end = parser.parse_exclamation(html);
	ASSERT_EQ('\0', doctype_end[0]);

	auto doc = parser.get_document();
	ASSERT_NE(nullptr, doc);

	auto doctype = doc->first_child();
	ASSERT_NE(nullptr, doctype);
	ASSERT_EQ("html", doctype->value());
}


TEST(parser, parse_elements_simple)
{
	html::string_type html{"<html></html>"};

	html::parser parser;
	auto doc = parser.parse(html);
	ASSERT_NE(nullptr, doc);

	auto child = doc->first_child();
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("HTML", child->name());
}


TEST(parser, parse_elements_single_child)
{
	html::string_type html{"<html><body></body></html>"};

	html::parser parser;
	auto doc = parser.parse(html);
	ASSERT_NE(nullptr, doc);

	auto html_node = doc->first_child();
	ASSERT_NE(nullptr, html_node);
	ASSERT_EQ("HTML", html_node->name());

	auto body = html_node->first_child();
	ASSERT_NE(nullptr, body);
	ASSERT_EQ("BODY", body->name());
}


TEST(parser, parse_elements_multiple_children)
{
	html::string_type html{"<html><body><div></div><p></p></body></html>"};

	html::parser parser;
	auto doc = parser.parse(html);
	ASSERT_NE(nullptr, doc);

	auto div = doc->first_child()->first_child()->first_child();
	ASSERT_NE(nullptr, div);
	ASSERT_EQ("DIV", div->name());

	auto p = div->next_sibling();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ("P", p->name());
}


TEST(parser, parse_doctype_and_elements)
{
	html::string_type html{"<!DOCTYPE html><html><body></body></html>"};

	html::parser parser;
	auto doc = parser.parse(html);
	ASSERT_NE(nullptr, doc);
}


TEST(parser, parse_void_elements)
{
	html::string_type str_html{"<html><br><p></p><br/></html>"};

	html::parser parser;
	auto doc = parser.parse(str_html);
	ASSERT_NE(nullptr, doc);

	auto html = doc->first_child();
	ASSERT_NE(nullptr, html);

	auto br1 = html->first_child();
	ASSERT_NE(nullptr, br1);
	ASSERT_EQ("BR", br1->name());

	auto p = br1->next_sibling();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ("P", p->name());

	auto br2 = p->next_sibling();
	ASSERT_NE(nullptr, br2);
	ASSERT_EQ("BR", br2->name());
}


TEST(parser, parse_single_element_with_attribute)
{
	html::string_type str_html{"<html id='content'></html>"};

	html::parser parser;
	auto doc = parser.parse(str_html);
	ASSERT_NE(nullptr, doc);

	auto html = doc->first_child();
	ASSERT_NE(nullptr, html);
	ASSERT_EQ("HTML", html->name());

	auto attr = html->get_attribute("ID");
	ASSERT_NE(nullptr, attr);
	ASSERT_EQ("content", attr->value());
}


TEST(parser, parse_single_element_with_multi_attributes)
{
	html::string_type str_html{"<html id='content' class=\"main\"></html>"};

	html::parser parser;
	auto doc = parser.parse(str_html);
	ASSERT_NE(nullptr, doc);

	auto html = doc->first_child();
	ASSERT_NE(nullptr, html);
	ASSERT_EQ("HTML", html->name());

	auto attr = html->get_attribute("ID");
	ASSERT_NE(nullptr, attr);
	ASSERT_EQ("content", attr->value());

	attr = html->get_attribute("CLASS");
	ASSERT_NE(nullptr, attr);
	ASSERT_EQ("main", attr->value());
}


TEST(parser, parse_tree_find_element_by_attribute)
{
	html::string_type str_html{
		"<!DOCTYPE html>"
		"<html> <body>"
			"<div id =  'content'  >"
				"Some content text."
			"</div>"
		"\n</body></html>"
		};

	html::parser parser;
	auto doc = parser.parse(str_html);
	ASSERT_NE(nullptr, doc);
}
