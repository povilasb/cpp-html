#include <gtest/gtest.h>

#include <cpp-html/node.hpp>
#include <cpp-html/attribute.hpp>

namespace html = cpphtml;


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


TEST(node, get_parent_and_root)
{
	auto div = html::node::create(html::node_element);

	auto p = html::node::create(html::node_element);
	p->name("p");
	div->append_child(p);

	auto text = html::node::create(html::node_pcdata);
	text->value("string");
	p->append_child(text);

	auto parent = p->parent();
	ASSERT_EQ(div, parent);


	auto child = p->first_child();
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("string", child->value());
	ASSERT_EQ(div, child->root());
}


TEST(node, prepend_child)
{
	auto div = html::node::create(html::node_element);
	auto p = html::node::create(html::node_element);
	div->prepend_child(p);

	auto child = div->first_child();
	ASSERT_EQ(p, child);
	ASSERT_EQ(div, p->parent());
}


TEST(node, child_value)
{
	auto div = html::node::create(html::node_element);
	auto p = html::node::create(html::node_element);
	div->append_child(p);

	auto text = html::node::create(html::node_pcdata);
	text->value("content");
	div->append_child(text);

	ASSERT_EQ("content", div->child_value());
}


TEST(node, remove_child)
{
	auto div = html::node::create(html::node_element);

	auto p = html::node::create(html::node_element);
	p->name("p");
	div->append_child(p);

	auto anchor = html::node::create(html::node_element);
	anchor->name("a");
	div->append_child(anchor);

	div->remove_child("p");
	auto child = div->first_child();
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("a", child->name());
}


TEST(node, find_child)
{
	auto div = html::node::create(html::node_element);

	auto p = html::node::create(html::node_element);
	div->append_child(p);

	auto text = html::node::create(html::node_pcdata);
	text->value("pcdata");
	div->append_child(text);

	auto child = div->find_child([](const std::shared_ptr<html::node>& node) {
		return node->type() == html::node_pcdata;
	});
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("pcdata", child->value());
}


TEST(node, find_node)
{
	auto div = html::node::create(html::node_element);
	auto p = html::node::create(html::node_element);

	auto text = html::node::create(html::node_pcdata);
	text->value("data");

	p->append_child(text);
	div->append_child(p);

	auto child = div->find_node([](const std::shared_ptr<html::node>& node) {
		return node->type() == html::node_pcdata;
	});
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("data", child->value());
}


TEST(node, find_child_by_attribute)
{
	auto div = html::node::create(html::node_element);
	auto p = html::node::create(html::node_element);
	div->append_child(p);

	auto input = html::node::create(html::node_element);
	input->append_attribute("type", "submit");
	input->append_attribute("id", "btn-post");
	input->name("input");
	div->append_child(input);

	auto child = div->find_child_by_attribute("input", "type", "submit");
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("btn-post", child->get_attribute("id")->value());

	child = div->find_child_by_attribute("type", "submit");
	ASSERT_NE(nullptr, child);
	ASSERT_EQ("btn-post", child->get_attribute("id")->value());
}


TEST(node, get_path)
{
	auto div = html::node::create(html::node_element);
	div->name("div");

	auto p = html::node::create(html::node_element);
	p->name("p");

	auto input = html::node::create(html::node_element);
	input->name("input");

	p->append_child(input);
	div->append_child(p);

	ASSERT_EQ("div/p/input", input->path());
}


TEST(node_text_content, returns_whole_child_when_there_is_only_text_child_node)
{
	auto div = html::node::create(html::node_element);
	div->name("div");
	auto text = html::node::create(html::node_pcdata);
	text->value("content");
	div->append_child(text);

	ASSERT_EQ(div->text_content(), std::string("content"));
}


TEST(node_text_content, returns_empty_string_when_node_has_no_child_elements)
{
	auto div = html::node::create(html::node_element);
	div->name("div");

	ASSERT_EQ(div->text_content(), std::string(""));
}


TEST(node_text_content,
	returns_all_text_children_combined_when_there_is_child_hierarchy)
{
	auto div = html::node::create(html::node_element);
	div->name("div");

	auto text = html::node::create(html::node_pcdata);
	text->value("content1");
	div->append_child(text);

	auto p = html::node::create(html::node_element);
	p->name("p");
	auto text2 = html::node::create(html::node_pcdata);
	text2->value("content2");
	p->append_child(text2);
	div->append_child(p);

	ASSERT_EQ(std::string("content1content2"), div->text_content());
}
