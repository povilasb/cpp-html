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

		WHEN("p tag is inside div which has no more children and p is "
			"not closed")
		{
			auto doc = parser.parse("<div><p>paragraph1</div>");

			THEN("parser succeeds")
			{
				REQUIRE(doc);
			}

			THEN("p element has a correct content")
			{
				REQUIRE(doc->first_child()->first_child()
					->first_child()->value() == "paragraph1");
			}
		}

		WHEN("td is last child element and it is not closed")
		{
			auto doc = parser.parse("<table><td>column1</table>");

			THEN("parser succeeds")
			{
				REQUIRE(doc);
			}
		}

		WHEN("tr is last child elemend and it is not closed")
		{
			auto doc = parser.parse(
				"<table><tr><td>column1</td></table>");

			THEN("parser succeeds")
			{
				REQUIRE(doc);
			}
		}

		WHEN("td tag is followed by another td tag and is not closed")
		{
			auto doc = parser.parse("<table><td>col1<td>col1</table>");

			THEN("parsing succeeds")
			{
				REQUIRE(doc);
			}
		}

		WHEN("tr tag is last child element and not closed and td tag "
			"is following.")
		{
			auto doc = parser.parse("<table><tr><td>col1</table>");

			THEN("parsing succeeds")
			{
				auto table = doc->first_child();
				REQUIRE(table->name() == "TABLE");

				auto tr = table->first_child();
				REQUIRE(tr->name() == "TR");

				auto td = tr->first_child();
				REQUIRE(td->name() == "TD");
			}
		}

		WHEN("not closed tr tag is followed by not closed another tr "
			"tag and both have child td tags")
		{
			auto doc = parser.parse("<table><tr><td>col1"
				"<tr><td>col2</table>");

			THEN("table has two child elements")
			{
				auto table = doc->first_child();
				REQUIRE(table->child_nodes().size() == 2);
			}
		}

		WHEN("not closed tbody tag has tr children")
		{
			auto doc = parser.parse("<table><tbody><tr><td>col1"
				"<tr><td>col2</table>");

			THEN("first child in table is tbody")
			{
				auto table = doc->first_child();
				auto tbody = table->first_child();
				REQUIRE(tbody->name() == "TBODY");

				THEN("tbody has two children elements")
				{
					REQUIRE(tbody->child_nodes().size() == 2);
				}
			}
		}

		WHEN("thead has no closing tag and it's last child element")
		{
			auto doc = parser.parse("<table><thead></table>");

			THEN("thead is closed automatically")
			{
				auto table = doc->first_child();

				REQUIRE(table->first_child()->name() == "THEAD");
			}
		}

		WHEN("thead has no closing tag and it's sibling is tbody")
		{
			auto doc = parser.parse("<table><thead><tbody></table>");

			THEN("thead is closed automatically")
			{
				auto table = doc->first_child();
				auto thead = table->first_child();

				REQUIRE(thead->name() == "THEAD");
				REQUIRE(thead->child_nodes().size() == 0);
			}
		}

		WHEN("th is not closed and followed by another th tag")
		{
			auto doc = parser.parse("<table><tr><th>header1<th>"
				"header2</table>");

			THEN("th is closed automatically")
			{
				auto table = doc->first_child();
				auto tr = table->first_child();
				auto th1 = tr->first_child();

				REQUIRE(th1->name() == "TH");
				REQUIRE(th1->first_child()->value() == "header1");

				THEN("th has a sibling, another th tag")
				{
					auto th2 = th1->next_sibling();

					REQUIRE(th2->name() == "TH");
					REQUIRE(th2->first_child()->value()
						== "header2");
				}
			}
		}

		WHEN("dd is not closed and is the last child element")
		{
			auto doc = parser.parse("<dl><dd>description1</dl>");

			THEN("dd is closed automaticallly")
			{
				auto dd = doc->first_child()->first_child();

				REQUIRE(dd->name() == "DD");
				REQUIRE(dd->first_child()->value()
					== "description1");
			}
		}

		WHEN("dd is not closed and it is followed by another dd tag")
		{
			auto doc = parser.parse("<dl><dd>description1"
				"<dd>description2</dd></dl>");

			THEN("dd is closed automatically")
			{
				auto dd = doc->first_child()->first_child();

				REQUIRE(dd->name() == "DD");
				REQUIRE(dd->first_child()->value()
					== "description1");
				REQUIRE(dd->next_sibling()->name() == "DD");
			}
		}

		WHEN("dt is not closed and is followed by another dt element")
		{
			auto doc = parser.parse("<dl><dt>term1<dt>term2</dt></dl>");

			THEN("dt is closed automatically")
			{
				auto dt = doc->first_child()->first_child();

				REQUIRE(dt->name() == "DT");
				REQUIRE(dt->first_child()->value()
					== "term1");
				REQUIRE(dt->next_sibling()->name() == "DT");
			}
		}

		WHEN("dt is not close and is followed by dd element")
		{
			auto doc = parser.parse("<dl><dt>term1<dd>desc1</dl>");

			THEN("dt is closed automatially.")
			{
				auto dt = doc->first_child()->first_child();

				REQUIRE(dt->name() == "DT");
				REQUIRE(dt->first_child()->value()
					== "term1");
				REQUIRE(dt->next_sibling()->name() == "DD");
			}
		}
	}
}
