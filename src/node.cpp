#include <new>
#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>

#include <pugihtml/node.hpp>
#include <pugihtml/attribute.hpp>


namespace pugihtml
{

enum chartypex_t {
	ctx_special_pcdata = 1, // Any symbol >= 0 and < 32 (except \t, \r, \n), &, <, >
	ctx_special_attr = 2, // Any symbol >= 0 and < 32 (except \t), &, <, >, "
	ctx_start_symbol = 4, // Any symbol > 127, a-z, A-Z, _
	ctx_digit = 8, // 0-9
	ctx_symbol = 16 // Any symbol > 127, a-z, A-Z, 0-9, _, -, .
};


const unsigned char chartypex_table[256] = {
	3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 3, 3, 2, 3, 3, // 0-15
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 16-31
	0, 0, 2, 0, 0, 0, 3, 0, 0,  0,  0,  0,  0, 16, 16,  0,	  // 32-47
	24, 24, 24, 24, 24, 24, 24, 24,    24, 24, 0,  0,  3,  0,  3,  0,	  // 48-63

	0,	20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 64-79
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  20,	  // 80-95
	0,	20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 96-111
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  0,	  // 112-127

	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 128+
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20
};

#ifdef PUGIHTML_WCHAR_MODE
#define IS_CHARTYPE_IMPL(c, ct, table) ((static_cast<unsigned int>(c) < 128 ? table[static_cast<unsigned int>(c)] : table[128]) & (ct))
#else
#define IS_CHARTYPE_IMPL(c, ct, table) (table[static_cast<unsigned char>(c)] & (ct))
#endif

#define IS_CHARTYPEX(c, ct) IS_CHARTYPE_IMPL(c, ct, chartypex_table)


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
	auto attr = std::shared_ptr<attribute>(new attribute(name, value));
	this->attributes_.push_back(attr);
	return attr;
}


std::shared_ptr<attribute>
node::append_attribute(const attribute& attr)
{
	auto sh_attr = std::shared_ptr<attribute>(new attribute(attr));
	this->attributes_.push_back(sh_attr);
	return sh_attr;
}


std::shared_ptr<attribute>
node::prepend_attribute(const string_type& name, const string_type& value)
{
	auto attr = std::shared_ptr<attribute>(new attribute(name, value));
	this->attributes_.push_front(attr);
	return attr;
}


std::shared_ptr<attribute>
node::prepend_attribute(const attribute& attr)
{
	auto sh_attr = std::shared_ptr<attribute>(new attribute(attr));
	this->attributes_.push_front(sh_attr);
	return sh_attr;
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
node::parent() const
{
	return this->parent_.lock();
}


std::shared_ptr<node>
node::root() const
{
	auto parent = this->parent_.lock();
	while (parent) {
		parent = parent->parent_.lock();
	}

	return parent;
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
	// TODO: set parent node.
	// TODO: set iterator in parent children list.
	this->children_.push_back(_node);
}


void
node::prepend_child(std::shared_ptr<node> _node)
{
	this->children_.push_front(_node);
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
		curr_node = curr_node->parent();
		result = curr_node->name() + delimiter + result;
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

	if (!walker.begin(*this)) {
		return false;
	}

	walker.depth_ = 0;
	std::shared_ptr<node> child = this->first_child();
	if (child) {
		++walker.depth_;

		do {
			if (!walker.for_each(*child)) {
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

	return walker.end(*this);
}


/*
void
node::print(html_writer& writer, const string_type& indent = "\t",
	unsigned int flags, html_encoding encoding, unsigned int depth) const
{
	html_buffered_writer buffered_writer(writer, encoding);

	node_output(buffered_writer, *this, indent, flags, depth);
}


void
node::print(std::basic_ostream<char_type, std::char_traits<char_type> >& os,
	string_type& indent, unsigned int flags, html_encoding encoding,
	unsigned int depth = 0) const
{
	html_writer_stream writer(os);
	print(writer, indent, flags, encoding, depth);
}
*/


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
node_walker::begin(node&)
{
	return true;
}


bool
node_walker::end(node&)
{
	return true;
}


/*
void
text_output_escaped(html_buffered_writer& writer, const string_type& str,
	chartypex_t type)
{
	while (*s) {
		const char_t* prev = s;

		// While *s is a usual symbol
		while (!IS_CHARTYPEX(*s, type)) ++s;

		writer.write(prev, static_cast<size_t>(s - prev));

		switch (*s)
		{
			case 0: break;
			case '&':
				writer.write('&', 'a', 'm', 'p', ';');
				++s;
				break;
			case '<':
				writer.write('&', 'l', 't', ';');
				++s;
				break;
			case '>':
				writer.write('&', 'g', 't', ';');
				++s;
				break;
			case '"':
				writer.write('&', 'q', 'u', 'o', 't', ';');
				++s;
				break;
			default: // s is not a usual symbol
			{
				unsigned int ch = static_cast<unsigned int>(*s++);
				assert(ch < 32);

				writer.write('&', '#',
					static_cast<char_t>((ch / 10) + '0'),
					static_cast<char_t>((ch % 10) + '0'),
					';');
			}
		}
	}
}


void
node_output_attributes(html_buffered_writer& writer, const node& node)
{
	const char_t* default_name = PUGIHTML_TEXT(":anonymous");

	for (attribute a = node.first_attribute(); a; a = a.next_attribute()) {
		writer.write(' ');
		writer.write(a.name()[0] ? a.name().c_str() : default_name);
		writer.write('=', '"');

		text_output_escaped(writer, a.value(), ctx_special_attr);

		writer.write('"');
	}
}


void
text_output_cdata(html_buffered_writer& writer, const char_t* s)
{
	do
	{
		writer.write('<', '!', '[', 'C', 'D');
		writer.write('A', 'T', 'A', '[');

		const char_t* prev = s;

		// look for ]]> sequence - we can't output it as is since it terminates CDATA
		while (*s && !(s[0] == ']' && s[1] == ']' && s[2] == '>')) ++s;

		// skip ]] if we stopped at ]]>, > will go to the next CDATA section
		if (*s) s += 2;

		writer.write(prev, static_cast<size_t>(s - prev));

		writer.write(']', ']', '>');
	}
	while (*s);
}


void
pugihtml::node_output(html_buffered_writer& writer, const node& _node,
	const char_t* indent, unsigned int flags, unsigned int depth)
{
	const char_t* default_name = PUGIHTML_TEXT(":anonymous");

	if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
		for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

	switch (_node.type())
	{
	case node_document:
	{
		for (node n = _node.first_child(); n; n = n.next_sibling())
			node_output(writer, n, indent, flags, depth);
		break;
	}

	case node_element:
	{
		const char_t* name = _node.name()[0] ? _node.name() : default_name;

		writer.write('<');
		writer.write(name);

		node_output_attributes(writer, _node);

		if (flags & format_raw)
		{
			if (!_node.first_child())
				writer.write(' ', '/', '>');
			else
			{
				writer.write('>');

				for (node n = _node.first_child(); n; n = n.next_sibling())
					node_output(writer, n, indent, flags, depth + 1);

				writer.write('<', '/');
				writer.write(name);
				writer.write('>');
			}
		}
		else if (!_node.first_child()) {
			writer.write(' ', '/', '>', '\n');
		}
		else if (_node.first_child() == _node.last_child() &&
			(_node.first_child().type() == node_pcdata ||
			_node.first_child().type() == node_cdata)) {

			writer.write('>');

			if (_node.first_child().type() == node_pcdata)
				text_output_escaped(writer, _node.first_child().value(), ctx_special_pcdata);
			else
				text_output_cdata(writer,
					 _node.first_child().value());

			writer.write('<', '/');
			writer.write(name);
			writer.write('>', '\n');
		}
		else {
			writer.write('>', '\n');

			for (node n = _node.first_child(); n; n = n.next_sibling())
				node_output(writer, n, indent, flags, depth + 1);

			if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
				for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

			writer.write('<', '/');
			writer.write(name);
			writer.write('>', '\n');
		}

		break;
	}

	case node_pcdata:
		text_output_escaped(writer, _node.value(), ctx_special_pcdata);
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_cdata:
		text_output_cdata(writer, _node.value());
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_comment:
		writer.write('<', '!', '-', '-');
		writer.write(_node.value());
		writer.write('-', '-', '>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_pi:
	case node_declaration:
		writer.write('<', '?');
		writer.write(_node.name()[0] ? _node.name() : default_name);

		if (_node.type() == node_declaration) {
			node_output_attributes(writer, _node);
		}
		else if (_node.value()[0])
		{
			writer.write(' ');
			writer.write(_node.value());
		}

		writer.write('?', '>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	case node_doctype:
		writer.write('<', '!', 'D', 'O', 'C');
		writer.write('T', 'Y', 'P', 'E');

		if (_node.value()[0])
		{
			writer.write(' ');
			writer.write(_node.value());
		}

		writer.write('>');
		if ((flags & format_raw) == 0) writer.write('\n');
		break;

	default:
		assert(!"Invalid node type");
	}
}
*/

} // pugihtml.
