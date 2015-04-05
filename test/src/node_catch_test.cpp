#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cstddef>

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

} // cpphtml.
