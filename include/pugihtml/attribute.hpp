#ifndef PUGIHTML_ATTRIBUTE_HPP
#define PUGIHTML_ATTRIBUTE_HPP 1

#include <pugihtml/pugihtml.hpp>

#include "memory.hpp"
#include "common.hpp"


namespace pugihtml
{

/**
 * A 'name=value' HTML attribute structure.
 */
struct attribute_struct {
	attribute_struct(html_memory_page* page)
		: header(reinterpret_cast<uintptr_t>(page)), name(0), value(0),
		prev_attribute_c(0), next_attribute(0)
	{
	}

	uintptr_t header;

	char_t* name; // Pointer to attribute name.
	char_t* value; // Pointer to attribute value.

	// Previous attribute (cyclic list)
	attribute_struct* prev_attribute_c;
	attribute_struct* next_attribute; // Next attribute
};


class node;
class attribute_iterator;

/**
 * A light-weight handle for manipulating attributes in DOM tree.
 */
class attribute {
	friend class attribute_iterator;
	friend class node;

private:
	attribute_struct* attr_;

	typedef attribute_struct* attribute::*unspecified_bool_type;

public:
	/**
	 * Constructs an empty attribute.
	 */
	attribute();

	/**
	 * Constructs attribute from internal pointer.
	 */
	explicit attribute(attribute_struct* attr);

	/**
	 * Safe bool conversion operator.
	 */
	operator unspecified_bool_type() const;

	/**
	 * Comparison operators (compares wrapped attribute pointers).
	 */
	bool operator==(const attribute& r) const;
	bool operator!=(const attribute& r) const;
	bool operator<(const attribute& r) const;
	bool operator>(const attribute& r) const;
	bool operator<=(const attribute& r) const;
	bool operator>=(const attribute& r) const;

	/**
	 * Alias for empty().
	 */
	bool operator!() const;

	/**
	 *  Check if attribute is empty.
	 */
	bool empty() const;

	/**
	 * @return attribute name or "" if attribute is empty.
	 */
	const string_type& name() const;

	/**
	 * @return attribute value or "" if attribute is empty.
	 */
	const char_t* value() const;

	/**
	 * Get attribute value as integer number, or 0 if conversion did not
	 * succeed or attribute is empty.
	 */
	int as_int() const;

	/**
	 * Get attribute value as unsigned integer number, or 0 if conversion
	 * did not succeed or attribute is empty.
	 */
	unsigned int as_uint() const;

	/**
	 * Get attribute value as double type number, or 0 if conversion
	 * did not succeed or attribute is empty.
	 */
	double as_double() const;

	/**
	 * Get attribute value as float number, or 0 if conversion
	 * did not succeed or attribute is empty.
	 */
	float as_float() const;

	/**
	 * Get attribute value as bool (returns true if first character is in
	 * '1tTyY' set), or false if attribute is empty.
	 */
	bool as_bool() const;

	/**
	 * Set attribute name (returns false if attribute is empty or there
	 * is not enough memory).
	 */
	bool set_name(const string_type& name);

	/**
	 * Set attribute value (returns false if attribute is empty or there
	 * is not enough memory).
	 */
	bool set_value(const char_t* rhs);

	/**
	 * Set attribute value with type conversion (numbers are converted to
	 * strings, boolean is converted to "true"/"false").
	 */
	bool set_value(int rhs);
	bool set_value(unsigned int rhs);
	bool set_value(double rhs);
	bool set_value(bool rhs);

	/**
	 * Set attribute value (equivalent to set_value without error checking).
	 */
	attribute& operator=(const char_t* rhs);
	attribute& operator=(int rhs);
	attribute& operator=(unsigned int rhs);
	attribute& operator=(double rhs);
	attribute& operator=(bool rhs);

	/**
	 * Get next/previous attribute in the attribute list of the parent node.
	 */
	attribute next_attribute() const;
	attribute previous_attribute() const;

	/**
	 * Get hash value (unique for handles to the same object).
	 */
	size_t hash_value() const;

	/**
	 * Get internal pointer.
	 */
	attribute_struct* internal_object() const;
};

} // pugihtml.


#endif /* PUGIHTML_ATTRIBUTE_HPP */
