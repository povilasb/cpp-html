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
