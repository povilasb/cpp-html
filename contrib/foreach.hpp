/*
 * Boost.Foreach support for cpp-html classes.
 * This file is provided to the public domain.
 * Written by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 */

#ifndef HEADER_PUGIHTML_FOREACH_HPP
#define HEADER_PUGIHTML_FOREACH_HPP

#include "cpp-html.hpp"

/*
 * These types add support for BOOST_FOREACH macro to node and document classes (child iteration only).
 * Example usage:
 * BOOST_FOREACH(node n, doc) {}
 */

namespace boost
{
	template <typename> struct range_mutable_iterator;
	template <typename> struct range_const_iterator;

	template<> struct range_mutable_iterator<pugi::node>
	{
		typedef pugi::node::iterator type;
	};

	template<> struct range_const_iterator<pugi::node>
	{
		typedef pugi::node::iterator type;
	};

	template<> struct range_mutable_iterator<pugi::document>
	{
		typedef pugi::document::iterator type;
	};

	template<> struct range_const_iterator<pugi::document>
	{
		typedef pugi::document::iterator type;
	};
}

/*
 * These types add support for BOOST_FOREACH macro to node and document classes (child/attribute iteration).
 * Example usage:
 * BOOST_FOREACH(node n, children(doc)) {}
 * BOOST_FOREACH(node n, attributes(doc)) {}
 */

namespace pugi
{
	struct node_children_adapter
	{
		typedef pugi::node::iterator iterator;
		typedef pugi::node::iterator const_iterator;

		node node;

		const_iterator begin() const
		{
			return node.begin();
		}

		const_iterator end() const
		{
			return node.end();
		}
	};

	node_children_adapter children(const pugi::node& node)
	{
		node_children_adapter result = {node};
		return result;
	}

	struct node_attribute_adapter
	{
		typedef pugi::node::attribute_iterator iterator;
		typedef pugi::node::attribute_iterator const_iterator;

		node node;

		const_iterator begin() const
		{
			return node.attributes_begin();
		}

		const_iterator end() const
		{
			return node.attributes_end();
		}
	};

	node_attribute_adapter attributes(const pugi::node& node)
	{
		node_attribute_adapter result = {node};
		return result;
	}
}

#endif
