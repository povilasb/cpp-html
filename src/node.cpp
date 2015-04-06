#include <new>
#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <functional>

#include <cpp-html/node.hpp>
#include <cpp-html/attribute.hpp>


namespace cpphtml
{


// TODO(povilas): use thes before append_child() and prepend_child().
inline bool
allow_insert_child(node_type parent, node_type child)
{
	if (parent != node_document && parent != node_element) {
		return false;
	}

	if (child == node_document || child == node_null) {
		return false;
	}

	if (parent != node_document && (child == node_declaration
		|| child == node_doctype)) {
		return false;
	}

	return true;
}


node::node(node_type type) : type_(type)
{
}


std::shared_ptr<node>
node::create(node_type type)
{
	return std::shared_ptr<node>(new node(type));
}


node_type
node::type() const
{
	return this->type_;
}


string_type
node::name() const
{
	return this->name_;
}


void
node::name(const string_type& name)
{
	this->name_ = name;
}


string_type
node::value() const
{
	return this->value_;
}


void
node::value(const string_type& value)
{
	switch (type())
	{
	case node_pi:
	case node_cdata:
	case node_pcdata:
	case node_comment:
	case node_doctype:
		this->value_ = value;
		break;

	default:
		return;
	}

}


string_type
node::text_content() const
{
	if (this->children_.size() == 0) {
		return "";
	}

	std::string text;
	auto tree_walker = make_node_walker([&](std::shared_ptr<node> node,
		std::size_t) {

		text += node->value();
		return true;
	});
	(const_cast<node*>(this))->traverse(*tree_walker);

	return text;
}


std::shared_ptr<attribute>
node::first_attribute() const
{
	return this->attributes_.empty() ? nullptr : this->attributes_.front();
}


std::shared_ptr<attribute>
node::last_attribute() const
{
	return this->attributes_.empty() ? nullptr : this->attributes_.back();
}


std::shared_ptr<attribute>
node::get_attribute(const string_type& name) const
{
	auto it_attr = std::find_if(std::begin(this->attributes_),
		std::end(this->attributes_),
		[&](const std::shared_ptr<attribute>& attr) {
			return attr->name() == name;
		});

	return it_attr == this->attributes_.cend() ? nullptr : *it_attr;
}


std::shared_ptr<attribute>
node::append_attribute(const string_type& name, const string_type& value)
{
	auto attr = attribute::create(name, value);
	this->attributes_.push_back(attr);
	return attr;
}


std::shared_ptr<attribute>
node::append_attribute(std::shared_ptr<attribute> attr)
{
	this->attributes_.push_back(attr);
	return attr;
}


std::shared_ptr<attribute>
node::prepend_attribute(const string_type& name, const string_type& value)
{
	auto attr = attribute::create(name, value);
	this->attributes_.push_front(attr);
	return attr;
}


std::shared_ptr<attribute>
node::prepend_attribute(std::shared_ptr<attribute> attr)
{
	this->attributes_.push_front(attr);
	return attr;
}


bool
node::remove_attribute(const string_type& name)
{
	bool result = false;

	this->attributes_.remove_if([&](const std::shared_ptr<attribute>& attr) {
		result = attr->name() == name;
		return result;
	});

	return result;
}


std::shared_ptr<node>
node::first_child() const
{
	return this->children_.empty() ? nullptr : this->children_.front();
}


std::shared_ptr<node>
node::last_child() const
{
	return this->children_.empty() ? nullptr : this->children_.back();
}


std::shared_ptr<node>
node::child(const string_type& name) const
{
	auto it_child = std::find_if(std::begin(this->children_),
		std::end(this->children_),
		[&](const std::shared_ptr<node>& child) {
			return child->name() == name;
		});

	return it_child == std::end(this->children_) ? nullptr : *it_child;
}


std::shared_ptr<node>
node::next_sibling() const
{
	auto it_next_node = this->parent_it_;
	auto parent = this->parent_.lock();
	return parent && ++it_next_node != std::end(parent->children_)
		? *it_next_node : nullptr;
}


std::shared_ptr<node>
node::next_sibling(const string_type& name) const
{
	auto parent = this->parent_.lock();
	if (!parent) {
		return nullptr;
	}

	auto it_next_sibling = this->parent_it_;
	++it_next_sibling;

	auto result = std::find_if(it_next_sibling,
		std::end(parent->children_),
		[&](const std::shared_ptr<node>& child) {
			return child->name() == name;
		});

	return result == std::end(parent->children_) ? nullptr : *result;
}


std::shared_ptr<node>
node::previous_sibling() const
{
	auto parent = this->parent_.lock();
	return parent && this->parent_it_ != std::begin(parent->children_)
		? *(--node::iterator(this->parent_it_)) : nullptr;
}


std::shared_ptr<node>
node::previous_sibling(const string_type& name) const
{
	auto parent = this->parent_.lock();
	if (!parent) {
		return nullptr;
	}

	if (this->parent_it_ == std::begin(parent->children_)) {
		return nullptr;
	}

	auto result = std::end(parent->children_);
	for (auto it_prev_sibling = this->parent_it_; it_prev_sibling
		!= std::begin(parent->children_); ){

		--it_prev_sibling;
		if ((*it_prev_sibling)->name() == name) {
			result = it_prev_sibling;
		}
	}

	return result != std::end(parent->children_) ? *result : nullptr;
}


std::shared_ptr<node>
node::parent() const
{
	return this->parent_.lock();
}


std::shared_ptr<node>
node::root() const
{
	std::shared_ptr<node> result;

	auto parent = this->parent_.lock();
	while (parent) {
		result = parent;
		parent = parent->parent_.lock();
	}

	return result;
}


string_type
node::child_value() const
{
	auto it_child = std::find_if(std::begin(this->children_),
		std::end(this->children_),
		[&](const std::shared_ptr<node>& child) {
			return child->type() == node_pcdata
				|| child->type() == node_cdata;
		});

	return it_child == std::end(this->children_) ? "" : (*it_child)->value();
}


string_type
node::child_value(const string_type& name) const
{
	auto child = this->child(name);
	return child ? child->child_value() : "";
}


void
node::append_child(std::shared_ptr<node> _node)
{
	_node->parent_ = this->shared_from_this();
	this->children_.push_back(_node);
	_node->parent_it_ = --std::end(this->children_);
}


void
node::prepend_child(std::shared_ptr<node> _node)
{
	_node->parent_ = this->shared_from_this();
	this->children_.push_front(_node);
	_node->parent_it_ = std::begin(this->children_);
}


bool
node::remove_child(const string_type& name)
{
	bool result = false;

	this->children_.remove_if([&](const std::shared_ptr<node>& child) {
		result = true;
		return child->name() == name;
	});

	return result;
}


const std::list<std::shared_ptr<node> >&
node::child_nodes() const
{
	return this->children_;
}


std::shared_ptr<node>
node::find_child_by_attribute(const string_type& tag,
	const string_type& attr_name,
	const string_type& attr_value) const
{
	auto it_child = std::find_if(std::begin(this->children_),
		std::end(this->children_),
		[&](const std::shared_ptr<node>& child) {
			if (child->name() != tag) {
				return false;
			}

			std::shared_ptr<attribute> attr =
				child->get_attribute(attr_name);
			return attr && attr->value() == attr_value;
		});

	return it_child == std::end(this->children_) ? nullptr : *it_child;
}


std::shared_ptr<node>
node::find_child_by_attribute(const string_type& attr_name,
	const string_type& attr_value) const
{
	auto it_child = std::find_if(std::begin(this->children_),
		std::end(this->children_),
		[&](const std::shared_ptr<node>& child) {

			std::shared_ptr<attribute> attr =
				child->get_attribute(attr_name);
			return attr && attr->value() == attr_value;
		});

	return it_child == std::end(this->children_) ? nullptr : *it_child;
}


string_type
node::path(char_type delimiter) const
{
	string_type result = this->name();

	auto curr_node = this->parent();
	while (curr_node) {
		result = curr_node->name() + delimiter + result;
		curr_node = curr_node->parent();
	}

	return result;
}


std::shared_ptr<node>
node::first_element_by_path(const string_type& path,
	char_type delimiter) const
{
	(void) path;
	(void) delimiter;
	// TODO(povilas): implement.
	throw std::runtime_error("Not implemented.");
	return nullptr;
}


bool
node::traverse(node_walker& walker)
{

	if (!walker.begin(this->shared_from_this())) {
		return false;
	}

	walker.depth_ = 0;
	std::shared_ptr<node> child = this->first_child();
	if (child) {
		++walker.depth_;

		do {
			if (!walker.for_each(child)) {
				return false;
			}

			if (child->first_child()) {
				++walker.depth_;
				child = child->first_child();
			}
			else if (child->next_sibling()) {
				child = child->next_sibling();
			}
			else {
				// Borland C++ workaround
				while (!child->next_sibling()
					&& walker.depth_ > 0
					&& child->parent()) {
					--walker.depth_;
					child = child->parent();
				}

				if (walker.depth_ > 0) {
					child = child->next_sibling();
				}
			}
		}
		while (child && walker.depth_ > 0);
	}

	return walker.end(this->shared_from_this());
}


bool
node::traverse(std::function<bool (std::shared_ptr<node>)> predicate,
	std::size_t depth)
{
	if (depth > 0) {
		bool proceed = predicate(this->shared_from_this());
		if (!proceed) {
			return false;
		}
	}

	for (auto child : this->children_) {
		bool proceed = child->traverse(predicate, depth + 1);
		if (!proceed) {
			return false;
		}
	}

	return true;
}


node::iterator
node::begin()
{
	return std::begin(this->children_);
}


node::iterator
node::end()
{
	return std::end(this->children_);
}


node::attribute_iterator
node::attributes_begin()
{
	return std::begin(this->attributes_);
}


node::attribute_iterator
node::attributes_end()
{
	return std::end(this->attributes_);
}


inline string_type
make_tabs(size_t tab_count)
{
	string_type tabs;

	for (size_t i = 1; i <= tab_count; ++i) {
		tabs += '\t';
	}

	return tabs;
}


inline string_type
make_tag_name(std::size_t indentation, string_type tag_name,
	string_type tag_prefix)
{
	if (tag_name == "") {
		return "";
	}

	return  make_tabs(indentation) + tag_prefix + tag_name + ">\n";
}


inline string_type
make_start_tag_name(std::size_t indentation, string_type tag_name)
{
	return make_tag_name(indentation, tag_name, "<");
}


inline string_type
make_end_tag_name(std::size_t indentation, string_type tag_name)
{
	return make_tag_name(indentation, tag_name, "</");
}


string_type
node::to_string(std::size_t indentation) const
{
	string_type str_html = make_start_tag_name(indentation, this->name());

	for (auto node : this->children_) {
		str_html += node->to_string(indentation + 1);
	}

	return str_html + make_end_tag_name(indentation, this->name());
}


node_walker::node_walker(): depth_(0)
{
}


node_walker::~node_walker()
{
}


int
node_walker::depth() const
{
	return this->depth_;
}


bool
node_walker::begin(std::shared_ptr<node>)
{
	return true;
}


bool
node_walker::end(std::shared_ptr<node>)
{
	return true;
}


class for_each_walker : public node_walker {
public:
	for_each_walker(node_walker_callback for_each)
		: for_each_(for_each)
	{
	}

	virtual bool
	for_each(std::shared_ptr<node> node) override
	{
		return this->for_each_(node, this->depth());
	}

private:
	node_walker_callback for_each_;
};

std::shared_ptr<node_walker>
make_node_walker(node_walker_callback for_each)
{
	return std::make_shared<for_each_walker>(for_each);
}

} // cpp-html.
