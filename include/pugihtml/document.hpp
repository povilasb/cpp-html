#ifndef PUGIHTML_DOCUMENT_HPP
#define PUGIHTML_DOCUMENT_HPP 1

#include <vector>
#include <memory>

#include <pugihtml/pugihtml.hpp>
#include <pugihtml/encoding.hpp>
#include <pugihtml/node.hpp>


namespace pugihtml
{

/**
 * Document class (DOM tree root).
 */
class document : public node {
public:
	/**
	 * Builds an empty document.
	 */
	document();

	/**
	 * Load document from stream.
	 */
	/*
	html_parse_result load(std::basic_istream<char_type,
		std::char_traits<char_type> >& stream,
		unsigned int options = parser::parse_default,
		html_encoding encoding = encoding_auto);
	*/

	/**
	 * Save HTML document to stream.
	 */
	/*
	void save(std::basic_ostream<char_type,
		std::char_traits<char_type> >& stream,
		const string_type& indent = "\t",
		unsigned int flags = format_default,
		html_encoding encoding = encoding_auto) const;
	*/

	/**
	 * Returns an array of all the links in the current document.
	 * The links collection counts <a href=""> tags and <area> tags.
	 */
	std::vector<std::shared_ptr<node> > links() const;

	/**
	 * Traverses DOM tree and searches for html node with the specified
	 * id attribute. If no tag is found, empty html node is returned.
	 */
	std::shared_ptr<node> get_element_by_id(const string_type& id) const;

};

} // pugihtml.

#endif /* PUGIHTML_DOCUMENT_HPP */
