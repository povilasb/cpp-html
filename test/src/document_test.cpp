#include <gtest/gtest.h>

#include <cpp-html/document.hpp>
#include <cpp-html/node.hpp>

namespace html = cpphtml;


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


TEST(document, links)
{
	auto doc = html::document::create();

	auto div = html::node::create(html::node_element);
	div->name("div");
	doc->append_child(div);

	auto anchor = html::node::create(html::node_element);
	anchor->name("A");
	div->append_child(anchor);

	auto area = html::node::create(html::node_element);
	area->name("AREA");
	div->append_child(area);

	auto links = doc->links();
	ASSERT_EQ(2u, links.size());

	ASSERT_NE(nullptr, links[0]);
	ASSERT_EQ("A", links[0]->name());

	ASSERT_NE(nullptr, links[1]);
	ASSERT_EQ("AREA", links[1]->name());
}
