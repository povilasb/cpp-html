#include <gtest/gtest.h>

#include <pugihtml/parser.hpp>

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
