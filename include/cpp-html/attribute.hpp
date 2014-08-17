#ifndef PUGIHTML_ATTRIBUTE_HPP
#define PUGIHTML_ATTRIBUTE_HPP 1

#include <memory>

#include <cpp-html/cpp-html.hpp>
#include <cpp-html/node.hpp>


namespace cpphtml
{

/**
 * Class for manipulating attributes in DOM tree.
 */
class attribute : public node {
public:
	/**
	 * Constructs attribute with default value "".
	 */
	static std::shared_ptr<attribute> create(const string_type& name,
		const string_type& value = "");

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
	attribute(const string_type& name, const string_type& value);

	string_type name_;
	string_type value_;
};

} // cpp-html.


#endif /* PUGIHTML_ATTRIBUTE_HPP */
