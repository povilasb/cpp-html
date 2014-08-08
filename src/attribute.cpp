#include <pugihtml/attribute.hpp>
#include <pugihtml/pugihtml.hpp>

namespace pugihtml
{

attribute::attribute(const string_type& name, const string_type& value)
	: name_(name), value_(value)
{
}


bool
attribute::operator==(const attribute& attr) const
{
	return this->name_ == attr.name_ && this->value_ == attr.value_;
}


bool
attribute::operator!=(const attribute& attr) const
{
	return !(*this == attr);
}


attribute&
attribute::operator=(const string_type& attr_val)
{
	this->value_ = attr_val;
	return *this;
}


string_type
attribute::name() const
{
	return this->name_;
}


string_type
attribute::value() const
{
	return this->value_;
}


void
attribute::value(const string_type& attr_val)
{
	this->value_ = attr_val;
}

} //pugihtml.
