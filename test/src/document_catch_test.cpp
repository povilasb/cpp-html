
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cpp-html/document.hpp>
#include <cpp-html/node.hpp>


namespace cpphtml
{

SCENARIO("document can be queried for elements by tag name.", "[document]")
{
	GIVEN("a document with 3 div elements")
	{
		auto doc = document::create();
		auto html = node::create(node_element);
		html->name("html");
		doc->append_child(html);

		for (size_t i = 1; i <= 3; ++i) {
			auto div = node::create(node_element);
			div->name("div");
			html->append_child(div);
		}

		REQUIRE(html->child_nodes().size() == 3);

		WHEN("queried for div elements")
		{
			auto div_elements = doc->get_elements_by_tag_name("div");

			THEN("list with 3 elements is returned")
			{
				REQUIRE(div_elements.size() == 3);
				REQUIRE(div_elements[0]->name() == "div");
				REQUIRE(div_elements[1]->name() == "div");
				REQUIRE(div_elements[2]->name() == "div");
			}
		}
	}
}

} // cpphtml.
