#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cstddef>
#include <memory>
#include <list>

#include <cpp-html/node.hpp>
#include <cpp-html/document.hpp>


namespace cpphtml
{

SCENARIO("node tree walker can be created")
{
	GIVEN("tree traverse lambda function")
	{
		string_type last_node;
		std::size_t last_depth = 0;

		auto on_each_node = [&](std::shared_ptr<node> node,
			std::size_t depth) {
			last_node = node->name();
			last_depth = depth;
			return true;
		};

		WHEN("make_node_walker is called with a lambda")
		{
			auto walker = make_node_walker(on_each_node);

			THEN("lambda function is called from walker.for_each()")
			{
				auto div = node::create(node_element);
				div->name("div");

				walker->for_each(div);

				REQUIRE(last_node == "div");
			}
		}
	}
}


SCENARIO("node tree can be translated to string")
{
	GIVEN("document with single child node")
	{
		auto div = node::create(node_element);
		div->name("div");

		WHEN("to_string is called on a document object")
		{
			string_type str_tree = div->to_string();

			THEN("tree html string is child node start and end tags.")
			{
				REQUIRE(str_tree == "<div>\n</div>\n");
			}
		}
	}

	GIVEN("document with two level depth tree")
	{
		auto div = node::create(node_element);
		div->name("div");

		auto p = node::create(node_element);
		p->name("p");
		div->append_child(p);

		WHEN("document is translated to html string")
		{
			string_type str_tree = div->to_string();

			THEN("tree html string is one child nested in another")
			{
				REQUIRE(str_tree == std::string("<div>\n\t<p>\n\t</p>\n</div>\n"));
			}
		}
	}
}


std::shared_ptr<node>
make_html_element(string_type tag_name)
{
	std::shared_ptr<node> element = node::create(node_element);
	element->name(tag_name);
	return element;
}


SCENARIO("node tree can be traversed with the std::function")
{
	GIVEN("node tree with 3 elements")
	{
		auto doc = make_html_element("");

		auto div = make_html_element("div");
		doc->append_child(div);

		auto p = make_html_element("p");
		div->append_child(p);

		auto input = make_html_element("input");
		div->append_child(input);

		WHEN("tree is traversed with lambda function that always returns true")
		{
			std::size_t nodes_visited = 0;
			doc->traverse([&](std::shared_ptr<node>) {
				++nodes_visited;
				return true;
			});

			THEN("visited node count is 3")
			{
				REQUIRE(nodes_visited == 3);
			}
		}

		WHEN("traverse function returns false when finds tag 'P'")
		{
			std::size_t nodes_visited = 0;
			doc->traverse([&](std::shared_ptr<node> node) {
				++nodes_visited;
				return node->name() != "p";
			});

			THEN("2 nodes are visited")
			{
				REQUIRE(nodes_visited == 2);
			}
		}
	}
}


SCENARIO("multiple nodes can be found by predicate")
{
	GIVEN("node tree with hierarchy depth 2")
	{
		auto doc = make_html_element("");
		auto div = make_html_element("div");
		doc->append_child(div);

		auto p = make_html_element("p");
		div->append_child(p);

		auto input = make_html_element("input");
		div->append_child(input);

		auto p2 = make_html_element("p");
		div->append_child(p2);

		WHEN("node list is searched by predicate matching nodes by name")
		{
			std::list<std::shared_ptr<node> > p_tags =
				div->find_nodes([](std::shared_ptr<node> node) {
				return node->name() == "p";
			});
		}
	}
}

} // cpphtml.
