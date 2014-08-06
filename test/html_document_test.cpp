#include <vector>

#include <gtest/gtest.h>

#include <html_document.hpp>
#include <html_node.hpp>


using namespace std;
namespace html = pugihtml;


class html_document_test : public ::testing::Test {
protected:
	html::html_document document;

	html_document_test()
	{
		string str_html =
			"<html><body>"
				"<a href=\"about.html\" />"
				"<a href=\"http://povilasb.com\" >"
				"<area href=\"doc.html\" >"
				"<div id=\"content\">contennt text</div>"
			"</body></html>"
		;

		document.load(str_html.c_str());
	}
};


TEST_F(html_document_test, get_root_node)
{
	html::html_node node = document.document_element();
	ASSERT_EQ(html::string_t("HTML"),
		html::string_t(node.name()));
}


TEST_F(html_document_test, links)
{
	vector<html::html_node> links = this->document.links();
	ASSERT_EQ(3u, links.size());
}


TEST_F(html_document_test, get_element_by_id_non_existant)
{
	html::html_node node = this->document.get_element_by_id("non-existant");
	ASSERT_TRUE(node.empty());
}


TEST_F(html_document_test, get_element_by_id)
{
	html::html_node node = this->document.get_element_by_id("content");
	ASSERT_TRUE(!node.empty());
	ASSERT_EQ(html::string_t("DIV"), html::string_t(node.name()));
}
