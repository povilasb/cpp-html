#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cpp-html/node.hpp>


namespace cpphtml
{

SCENARIO("node tree walker can be created")
{
	GIVEN("")
	{
		WHEN("make_node_walker is called with a lambda")
		{
			std::string last_node;

			auto walker = make_node_walker([&](std::shared_ptr<node> node) {
				last_node = node->name();
				return true;
			});

			THEN("walker with for_each method is returned")
			{
				auto div = node::create(node_element);
				div->name("div");

				walker->for_each(div);

				REQUIRE(last_node == "div");
			}
		}
	}
}

} // cpphtml.
