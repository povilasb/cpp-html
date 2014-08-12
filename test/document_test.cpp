#include <gtest/gtest.h>

#include <pugihtml/document.hpp>
#include <pugihtml/node.hpp>

namespace html = pugihtml;


TEST(document, create)
{
	auto doc = html::document::create();
	ASSERT_EQ(html::node_document, doc->type());
}


TEST(document, get_element_by_id)
{
	auto doc = html::document::create();

	auto div = html::node::create(html::node_element);
	div->append_attribute("ID", "content");
	div->name("div");

	doc->append_child(div);

	auto child = doc->get_element_by_id("content");
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("div", child->name());
}
