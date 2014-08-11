#include <gtest/gtest.h>

#include <pugihtml/node.hpp>
#include <pugihtml/attribute.hpp>

namespace html = pugihtml;


TEST(node, create_text_node)
{
	auto text_node = html::node::create();
	ASSERT_EQ("", text_node->name());

	ASSERT_FALSE(text_node->parent());
	ASSERT_EQ(html::node_pcdata, text_node->type());

	text_node->value("text node value");
	ASSERT_EQ("text node value", text_node->value());
}


TEST(node, create_div_node)
{
	auto div = html::node::create(html::node_element);
	ASSERT_EQ(html::node_element, div->type());

	div->name("div");
	ASSERT_EQ("div", div->name());
}

TEST(node, append_attributes)
{
	auto div = html::node::create(html::node_element);
	auto attr = div->append_attribute("id", "content");
	ASSERT_EQ("id", attr->name());
	ASSERT_EQ("content", attr->value());

	attr = div->append_attribute("class", "block");
	ASSERT_EQ("class", attr->name());
	ASSERT_EQ("block", attr->value());

	attr = div->get_attribute("class");
	ASSERT_EQ("class", attr->name());
	ASSERT_EQ("block", attr->value());

	attr = div->get_attribute("id");
	ASSERT_EQ("id", attr->name());
	ASSERT_EQ("content", attr->value());

	attr = div->append_attribute("empty");
	ASSERT_EQ("empty", attr->name());
	ASSERT_EQ("", attr->value());
}


TEST(node, attribute_iteration)
{
	auto div = html::node::create(html::node_element);
	div->append_attribute("id", "content");
	div->append_attribute("class", "base");
	div->append_attribute("width", "100px");

	auto attr = div->first_attribute();
	ASSERT_EQ("id", attr->name());

	attr = div->last_attribute();
	ASSERT_EQ("width", attr->name());

	std::list<html::string_type> exp_attrs{"id", "class", "width"};
	auto it_attr = div->attributes_begin();
	ASSERT_TRUE(std::equal(std::begin(exp_attrs), std::end(exp_attrs),
		it_attr, [](const html::string_type& exp_name,
			const std::shared_ptr<html::attribute>& curr_attr) {
			return exp_name == curr_attr->name();
		}));
}


TEST(node, remove_first_attribute)
{
	auto div = html::node::create(html::node_element);
	div->append_attribute("id", "content");
	div->append_attribute("class", "base");
	div->append_attribute("width", "100px");

	div->remove_attribute("id");

	std::list<html::string_type> exp_attrs{"class", "width"};
	auto it_attr = div->attributes_begin();
	ASSERT_TRUE(std::equal(std::begin(exp_attrs), std::end(exp_attrs),
		it_attr, [](const html::string_type& exp_name,
			const std::shared_ptr<html::attribute>& curr_attr) {
			return exp_name == curr_attr->name();
		}));
}


TEST(node, remove_middle_attribute)
{
	auto div = html::node::create(html::node_element);
	div->append_attribute("id", "content");
	div->append_attribute("class", "base");
	div->append_attribute("width", "100px");

	div->remove_attribute("class");

	std::list<html::string_type> exp_attrs{"id", "width"};
	auto it_attr = div->attributes_begin();
	ASSERT_TRUE(std::equal(std::begin(exp_attrs), std::end(exp_attrs),
		it_attr, [](const html::string_type& exp_name,
			const std::shared_ptr<html::attribute>& curr_attr) {
			return exp_name == curr_attr->name();
		}));
}


TEST(node, remove_last_attribute)
{
	auto div = html::node::create(html::node_element);
	div->append_attribute("id", "content");
	div->append_attribute("class", "base");
	div->append_attribute("width", "100px");

	div->remove_attribute("width");

	std::list<html::string_type> exp_attrs{"id", "class"};
	auto it_attr = div->attributes_begin();
	ASSERT_TRUE(std::equal(std::begin(exp_attrs), std::end(exp_attrs),
		it_attr, [](const html::string_type& exp_name,
			const std::shared_ptr<html::attribute>& curr_attr) {
			return exp_name == curr_attr->name();
		}));
}


TEST(node, find_attribute)
{
	auto div = html::node::create(html::node_element);
	div->append_attribute("id", "content");
	div->append_attribute("class", "base");
	div->append_attribute("width", "100px");

	auto attr = div->find_attribute(
		[](const std::shared_ptr<html::attribute>& curr_attr) {
			return curr_attr->name() == "class";
		});

	ASSERT_EQ("base", attr->value());
}


TEST(node, child_iteration)
{
	// <div> <h1></h1> <p></p> <input/> </div>
	auto div = html::node::create(html::node_element);

	auto h1 = html::node::create(html::node_element);
	h1->name("h1");
	div->append_child(h1);

	auto p = html::node::create(html::node_element);
	p->name("p");
	auto text = html::node::create();
	text->value("text");
	p->append_child(text);
	div->append_child(p);

	auto input = html::node::create(html::node_element);
	input->name("input");
	div->append_child(input);

	auto child = div->first_child();
	ASSERT_EQ("h1", child->name());

	child = div->last_child();
	ASSERT_EQ("input", child->name());

	child = div->child("p");
	ASSERT_EQ("text", child->first_child()->value());

	std::list<html::string_type> exp_childs{"h1", "p", "input"};
	auto it_children = div->begin();
	ASSERT_TRUE(std::equal(std::begin(exp_childs), std::end(exp_childs),
		it_children, [](const html::string_type& exp_name,
			const std::shared_ptr<html::node>& child_node) {
				return child_node->name() == exp_name;
			}
		)
	);
}


TEST(node, sibling)
{
	// <div> <h1></h1> <p></p> <input/> </div>
	auto div = html::node::create(html::node_element);

	auto h1 = html::node::create(html::node_element);
	h1->name("h1");
	div->append_child(h1);

	auto p = html::node::create(html::node_element);
	p->name("p");
	div->append_child(p);

	auto input = html::node::create(html::node_element);
	input->name("input");
	div->append_child(input);

	auto child = div->child("p");
	ASSERT_NE(nullptr, child.get());

	auto prev_sibling = child->previous_sibling();
	ASSERT_NE(nullptr, prev_sibling.get());
	ASSERT_EQ("h1", prev_sibling->name());

	auto next_sibling = child->next_sibling();
	ASSERT_NE(nullptr, next_sibling);
	ASSERT_EQ("input", next_sibling->name());


	child = div->first_child();
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("h1", child->name());

	next_sibling = child->next_sibling("input");
	ASSERT_NE(nullptr, next_sibling);
	ASSERT_EQ("input", next_sibling->name());


	child = div->last_child();
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("input", child->name());

	prev_sibling = child->previous_sibling("h1");
	ASSERT_NE(nullptr, prev_sibling);
	ASSERT_EQ("h1", prev_sibling->name());
}
