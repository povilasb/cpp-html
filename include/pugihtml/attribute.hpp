#ifndef PUGIHTML_ATTRIBUTE_HPP
#define PUGIHTML_ATTRIBUTE_HPP 1

#include <pugihtml/pugihtml.hpp>


namespace pugihtml
{

/**
 * A light-weight handle for manipulating attributes in DOM tree.
 */
class attribute {
public:
	/**
	 * Constructs attribute with default value "".
	 */
	attribute(const string_type& name, const string_type& value = "");

	//TODO(povilas): add copy constructor, move constructor.

	/**
	 * Compares attribute name/value pairs.
	 */
	bool operator==(const attribute& attr) const;

	/**
	 * Compares attribute name/value pairs.
	 */
	bool operator!=(const attribute& attr) const;

	/**
	 * Set attribute value.
	 */
	attribute& operator=(const string_type& attr_val);

	/**
	 * @return attribute name or "" if attribute is empty.
	 */
	string_type name() const;

	/**
	 * @return attribute value or "" if attribute is empty.
	 */
	string_type value() const;

	/**
	 * Sets attribute value.
	 */
	void value(const string_type& attr_val);

private:
	string_type name_;
	string_type value_;
};

} // pugihtml.


#endif /* PUGIHTML_ATTRIBUTE_HPP */
