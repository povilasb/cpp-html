#include <string>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cpp-html/tokenizer.hpp>


SCENARIO("token iterator can be created from string", "[token_iterator]")
{
	GIVEN("valid html5 string")
	{
		std::string str_html = "<html></html>";

		WHEN("token iterator is constructed from string")
		{
			cpphtml::token_iterator it_token(str_html);

			THEN("token iterator points to tag opening token")
			{
				REQUIRE(it_token->type
					== cpphtml::token_type::start_tag_open);
			}

			WHEN("iterator is increased")
			{
				++it_token;

				THEN("iterator points to string token")
				{
					REQUIRE(it_token->type
						== cpphtml::token_type::string);
				}
			}
		}
	}

	GIVEN("html with tag starting without tag open token")
	{
		std::string str_html = "html></html>";

		WHEN("token iterator is constructed from string")
		{
			cpphtml::token_iterator it_token(str_html);

			THEN("token iterator points to string token")
			{
				REQUIRE(it_token->type
					== cpphtml::token_type::string);
			}

			THEN("token value is tag name")
			{
				REQUIRE(it_token->value == "html");
			}
		}
	}
}
