#include <gtest/gtest.h>

#include <pugihtml/attribute.hpp>
#include <pugihtml/pugihtml.hpp>


namespace html = pugihtml;


TEST(attribute, create)
{
	auto attr = html::attribute::create("id", "content");
	ASSERT_NE(nullptr, attr);
	ASSERT_EQ("id", attr->name());
	ASSERT_EQ("content", attr->value());
}


TEST(attribute, compare)
{
	auto attr1 = html::attribute::create("id", "content");
	auto attr2 = html::attribute::create("id", "content");

	ASSERT_TRUE(*attr1 == *attr2);
	ASSERT_FALSE(*attr1 != *attr2);

	attr2->value("conten2");

	ASSERT_TRUE(*attr1 != *attr2);
	ASSERT_FALSE(*attr1 == *attr2);
}


TEST(attribute, set_value_assign_operator)
{
	auto attr = html::attribute::create("id");
	ASSERT_NE(nullptr, attr);

	*attr = "content";
	ASSERT_EQ("content", attr->value());
}
