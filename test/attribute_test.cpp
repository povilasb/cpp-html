#include <gtest/gtest.h>

#include <pugihtml/attribute.hpp>
#include <pugihtml/pugihtml.hpp>

#include "memory.hpp"


namespace html = pugihtml;


TEST(attribute, DISABLED_set_get_name)
{
	html::html_allocator allocator;
	html::attribute_struct* attr_s = html::attribute::allocate_attribute(
		allocator);
	html::attribute attr(attr_s);

	attr.set_name("id");
	ASSERT_EQ(html::string_type("id"), attr.name());
}
