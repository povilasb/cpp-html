#ifndef PUGIHTML_DOCUMENT_HPP
#define PUGIHTML_DOCUMENT_HPP 1

#include <vector>
#include <memory>

#include <pugihtml/pugihtml.hpp>
#include <pugihtml/node.hpp>


namespace pugihtml
{

/**
 * Document class (DOM tree root).
 */
class document : public node {
public:
	/**
	 * Builds an empty document. It's html node with type node_document.
	 */
	static std::shared_ptr<document> create();

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

private:
	/**
	 * Builds an empty document. It's html node with type node_document.
	 */
	document();
};

} // pugihtml.

#endif /* PUGIHTML_DOCUMENT_HPP */
