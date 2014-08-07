#include <cstdio>

#include <pugihtml/attribute.hpp>

#include "common.hpp"
#include "pugiutil.hpp"


using namespace pugihtml;


attribute::attribute(): attr_(0)
{
}


attribute::attribute(attribute_struct* attr): attr_(attr)
{
}


attribute::operator attribute::unspecified_bool_type() const
{
	return attr_ ? &attribute::attr_ : 0;
}


bool
attribute::operator!() const
{
	return this->empty();
}


bool
attribute::operator==(const attribute& r) const
{
	return (this->attr_ == r.attr_);
}


bool
attribute::operator!=(const attribute& r) const
{
	return (attr_ != r.attr_);
}


bool
attribute::operator<(const attribute& r) const
{
	return (attr_ < r.attr_);
}


bool
attribute::operator>(const attribute& r) const
{
	return (this->attr_ > r.attr_);
}


bool
attribute::operator<=(const attribute& r) const
{
	return (this->attr_ <= r.attr_);
}


bool
attribute::operator>=(const attribute& r) const
{
	return (attr_ >= r.attr_);
}


attribute
attribute::next_attribute() const
{
	return this->attr_ ? attribute(attr_->next_attribute) : attribute();
}


attribute
attribute::previous_attribute() const
{
	return this->attr_ && this->attr_->prev_attribute_c->next_attribute ?
		attribute(this->attr_->prev_attribute_c) : attribute();
}


int
attribute::as_int() const
{
	if (!attr_ || !attr_->value) return 0;

#ifdef PUGIHTML_WCHAR_MODE
	return (int)wcstol(attr_->value, 0, 10);
#else
	return (int)strtol(attr_->value, 0, 10);
#endif
}


unsigned int
attribute::as_uint() const
{
	if (!this->attr_ || !this->attr_->value) {
		return 0;
	}

#ifdef PUGIHTML_WCHAR_MODE
	return (unsigned int)wcstoul(attr_->value, 0, 10);
#else
	return (unsigned int)strtoul(attr_->value, 0, 10);
#endif
}


double
attribute::as_double() const
{
	if (!this->attr_ || !this->attr_->value) {
		return 0;
	}

#ifdef PUGIHTML_WCHAR_MODE
	return wcstod(attr_->value, 0);
#else
	return strtod(attr_->value, 0);
#endif
}


float
attribute::as_float() const
{
	if (!this->attr_ || !this->attr_->value) {
		return 0;
	}

#ifdef PUGIHTML_WCHAR_MODE
	return (float)wcstod(attr_->value, 0);
#else
	return (float)strtod(attr_->value, 0);
#endif
}


bool
attribute::as_bool() const
{
	if (!this->attr_ || !this->attr_->value) {
		return false;
	}

	// only look at first char
	char_t first = *attr_->value;

	// 1*, t* (true), T* (True), y* (yes), Y* (YES)
	return (first == '1' || first == 't' || first == 'T' || first == 'y'
		|| first == 'Y');
}


bool
attribute::empty() const
{
	return !attr_;
}


const
char_t* attribute::name() const
{
	return (attr_ && attr_->name) ? attr_->name : PUGIHTML_TEXT("");
}


const char_t*
attribute::value() const
{
	return (attr_ && attr_->value) ? attr_->value : PUGIHTML_TEXT("");
}


size_t
attribute::hash_value() const
{
	return static_cast<size_t>(reinterpret_cast<uintptr_t>(attr_)
		/ sizeof(attribute_struct));
}


attribute_struct*
attribute::internal_object() const
{
	return attr_;
}

attribute& attribute::operator=(const char_t* rhs)
{
	set_value(rhs);
	return *this;
}

attribute& attribute::operator=(int rhs)
{
	set_value(rhs);
	return *this;
}

attribute& attribute::operator=(unsigned int rhs)
{
	set_value(rhs);
	return *this;
}

attribute& attribute::operator=(double rhs)
{
	set_value(rhs);
	return *this;
}

attribute& attribute::operator=(bool rhs)
{
	set_value(rhs);
	return *this;
}

bool attribute::set_name(const char_t* rhs)
{
	if (!attr_) return false;

	return strcpy_insitu(attr_->name, attr_->header, html_memory_page_name_allocated_mask, rhs);
}

bool attribute::set_value(const char_t* rhs)
{
	if (!attr_) return false;

	return strcpy_insitu(attr_->value, attr_->header, html_memory_page_value_allocated_mask, rhs);
}

bool attribute::set_value(int rhs)
{
	char buf[128];
	sprintf(buf, "%d", rhs);

#ifdef PUGIHTML_WCHAR_MODE
	char_t wbuf[128];
	widen_ascii(wbuf, buf);

	return set_value(wbuf);
#else
	return set_value(buf);
#endif
}

bool attribute::set_value(unsigned int rhs)
{
	char buf[128];
	sprintf(buf, "%u", rhs);

#ifdef PUGIHTML_WCHAR_MODE
	char_t wbuf[128];
	widen_ascii(wbuf, buf);

	return set_value(wbuf);
#else
	return set_value(buf);
#endif
}

bool attribute::set_value(double rhs)
{
	char buf[128];
	sprintf(buf, "%g", rhs);

#ifdef PUGIHTML_WCHAR_MODE
	char_t wbuf[128];
	widen_ascii(wbuf, buf);

	return set_value(wbuf);
#else
	return set_value(buf);
#endif
}


bool
attribute::set_value(bool rhs)
{
	return set_value(rhs ? PUGIHTML_TEXT("true") : PUGIHTML_TEXT("false"));
}
