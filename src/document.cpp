#include <vector>
#include <memory>

#include <pugihtml/document.hpp>
#include <pugihtml/node.hpp>
#include <pugihtml/attribute.hpp>


namespace pugihtml
{


std::shared_ptr<document>
document::create()
{
	return std::shared_ptr<document>(new document());
}


document::document() : node(node_document)
{
}


class links_walker : public node_walker {
public:
	links_walker(std::vector<std::shared_ptr<node> >& links) : links_(links)
	{
	}

	bool
	for_each(std::shared_ptr<node> node) override
	{
		if (node->name() == "A" ||
			node->name() == "AREA") {
			this->links_.push_back(node);
		}

		return true;
	}

private:
	std::vector<std::shared_ptr<node> >& links_;
};

std::vector<std::shared_ptr<node> >
document::links() const
{
	std::vector<std::shared_ptr<node> > result;

	links_walker html_walker(result);
	const_cast<document*>(this)->traverse(html_walker);

	return result;
}


std::shared_ptr<node>
document::get_element_by_id(const string_type& id) const
{
	return this->find_node([&](const std::shared_ptr<node>& node) {
		auto attr = node->get_attribute("ID");
		return attr ? attr->value() == id : false;
	});
}

} // pugihtml.
