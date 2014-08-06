#include <cstdio>

#include "html_attribute.hpp"
#include "common.hpp"
#include "pugiutil.hpp"


using namespace pugihtml;


html_attribute::html_attribute(): _attr(0)
{
}

html_attribute::html_attribute(html_attribute_struct* attr): _attr(attr)
{
}

html_attribute::operator html_attribute::unspecified_bool_type() const
{
	return _attr ? &html_attribute::_attr : 0;
}

bool html_attribute::operator!() const
{
	return !_attr;
}

bool html_attribute::operator==(const html_attribute& r) const
{
	return (_attr == r._attr);
}

bool html_attribute::operator!=(const html_attribute& r) const
{
	return (_attr != r._attr);
}

bool html_attribute::operator<(const html_attribute& r) const
{
	return (_attr < r._attr);
}

bool html_attribute::operator>(const html_attribute& r) const
{
	return (_attr > r._attr);
}

bool html_attribute::operator<=(const html_attribute& r) const
{
	return (_attr <= r._attr);
}

bool html_attribute::operator>=(const html_attribute& r) const
{
	return (_attr >= r._attr);
}

html_attribute html_attribute::next_attribute() const
{
	return _attr ? html_attribute(_attr->next_attribute) : html_attribute();
}

html_attribute html_attribute::previous_attribute() const
{
	return _attr && _attr->prev_attribute_c->next_attribute ? html_attribute(_attr->prev_attribute_c) : html_attribute();
}

int html_attribute::as_int() const
{
	if (!_attr || !_attr->value) return 0;

#ifdef PUGIHTML_WCHAR_MODE
	return (int)wcstol(_attr->value, 0, 10);
#else
	return (int)strtol(_attr->value, 0, 10);
#endif
}

unsigned int html_attribute::as_uint() const
{
	if (!_attr || !_attr->value) return 0;

#ifdef PUGIHTML_WCHAR_MODE
	return (unsigned int)wcstoul(_attr->value, 0, 10);
#else
	return (unsigned int)strtoul(_attr->value, 0, 10);
#endif
}

double html_attribute::as_double() const
{
	if (!_attr || !_attr->value) return 0;

#ifdef PUGIHTML_WCHAR_MODE
	return wcstod(_attr->value, 0);
#else
	return strtod(_attr->value, 0);
#endif
}

float html_attribute::as_float() const
{
	if (!_attr || !_attr->value) return 0;

#ifdef PUGIHTML_WCHAR_MODE
	return (float)wcstod(_attr->value, 0);
#else
	return (float)strtod(_attr->value, 0);
#endif
}

bool html_attribute::as_bool() const
{
	if (!_attr || !_attr->value) return false;

	// only look at first char
	char_t first = *_attr->value;

	// 1*, t* (true), T* (True), y* (yes), Y* (YES)
	return (first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y');
}

bool html_attribute::empty() const
{
	return !_attr;
}

const char_t* html_attribute::name() const
{
	return (_attr && _attr->name) ? _attr->name : PUGIHTML_TEXT("");
}

const char_t* html_attribute::value() const
{
	return (_attr && _attr->value) ? _attr->value : PUGIHTML_TEXT("");
}

size_t html_attribute::hash_value() const
{
	return static_cast<size_t>(reinterpret_cast<uintptr_t>(_attr) / sizeof(html_attribute_struct));
}

html_attribute_struct* html_attribute::internal_object() const
{
	return _attr;
}

html_attribute& html_attribute::operator=(const char_t* rhs)
{
	set_value(rhs);
	return *this;
}

html_attribute& html_attribute::operator=(int rhs)
{
	set_value(rhs);
	return *this;
}

html_attribute& html_attribute::operator=(unsigned int rhs)
{
	set_value(rhs);
	return *this;
}

html_attribute& html_attribute::operator=(double rhs)
{
	set_value(rhs);
	return *this;
}

html_attribute& html_attribute::operator=(bool rhs)
{
	set_value(rhs);
	return *this;
}

bool html_attribute::set_name(const char_t* rhs)
{
	if (!_attr) return false;

	return strcpy_insitu(_attr->name, _attr->header, html_memory_page_name_allocated_mask, rhs);
}

bool html_attribute::set_value(const char_t* rhs)
{
	if (!_attr) return false;

	return strcpy_insitu(_attr->value, _attr->header, html_memory_page_value_allocated_mask, rhs);
}

bool html_attribute::set_value(int rhs)
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

bool html_attribute::set_value(unsigned int rhs)
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

bool html_attribute::set_value(double rhs)
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


bool html_attribute::set_value(bool rhs)
{
	return set_value(rhs ? PUGIHTML_TEXT("true") : PUGIHTML_TEXT("false"));
}

#ifdef __BORLANDC__
bool
operator&&(const html_attribute& lhs, bool rhs)
{
	return (bool)lhs && rhs;
}

bool
operator||(const html_attribute& lhs, bool rhs)
{
	return (bool)lhs || rhs;
}
#endif
