#include <string>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cpp-html/parser.hpp>


SCENARIO("cpphtml parser creates DOM document from html string", "[parser]")
{
	GIVEN("html parser with default parse options")
	{
		cpphtml::parser parser;

		WHEN("void element tag name does not end with />")
		{
			std::string str_html{"<br class=line /<div></div>"};

			THEN("parse_error is thrown")
			{
				REQUIRE_THROWS_AS(parser.parse(str_html),
					cpphtml::parse_error);
			}
		}
	}
}
