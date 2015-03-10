#include <string>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cpp-html/parser.hpp>


SCENARIO("cpphtml parser creates DOM document from html string", "[parser]")
{
	GIVEN("html parser with default parse options")
	{
		cpphtml::parser parser;

		WHEN("void element with attributes tag name ends with />")
		{
			auto doc = parser.parse("<br class=line /><div></div>");

			THEN("valid html document is created")
			{
				REQUIRE(doc->first_child()->name() == "BR");
			}
		}

		WHEN("void element with attributes tag name does not end with />")
		{
			std::string str_html{"<br class=line /<div></div>"};

			THEN("parse_error is thrown")
			{
				REQUIRE_THROWS_AS(parser.parse(str_html),
					cpphtml::parse_error);
			}
		}

		WHEN("void element without attribues tag name ends with />")
		{
			auto doc = parser.parse("<br/><div></div>");

			THEN("valid html DOM document is created")
			{
				REQUIRE(doc->first_child()->name() == "BR");
			}
		}

		WHEN("void element without attribues tag name does not end with />")
		{
			std::string str_html{"<br/<div></div>"};

			THEN("parse_error is thrown")
			{
				REQUIRE_THROWS_AS(parser.parse(str_html),
					cpphtml::parse_error);
			}
		}
	}
}
