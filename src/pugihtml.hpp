/**
 * pugihtml parser - version 1.0
 * --------------------------------------------------------
 * Copyright (c) 2012 Adgooroo, LLC (kgantchev [AT] adgooroo [DOT] com)
 *
 * This library is distributed under the MIT License. See notice in license.txt
 *
 * This work is based on the pugxml parser, which is:
 * Copyright (C) 2006-2010, by Arseny Kapoulkine (arseny [DOT] kapoulkine [AT] gmail [DOT] com)
 */


#ifndef HEADER_PUGIHTML_HPP
#define HEADER_PUGIHTML_HPP

#include <stddef.h>

#include "common.hpp"
#include "html_parser.hpp"
#include "html_node.hpp"
#include "html.hpp"
#include "html_attribute.hpp"
#include "html_writer.hpp"


// The PugiHTML namespace
namespace pugihtml
{
	// Forward declarations
	struct html_attribute_struct;
	struct html_node_struct;

	class html_node_iterator;
	class html_attribute_iterator;

	class html_tree_walker;

	class html_node;

	#ifndef PUGIHTML_NO_XPATH
	class xpath_node;
	class xpath_node_set;
	class xpath_query;
	class xpath_variable_set;
	#endif

	struct html_parse_result;

	// Document class (DOM tree root)
	class PUGIHTML_CLASS html_document : public html_node {
	private:
		char_t* _buffer;

		char _memory[192];

		// Non-copyable semantics
		html_document(const html_document&);
		const html_document& operator=(const html_document&);

		/**
		 * Initializes document: allocates memory page, etc.
		 */
		void create();

		void destroy();

		html_parse_result load_buffer_impl(void* contents, size_t size, unsigned int options, html_encoding encoding, bool is_mutable, bool own);

	public:
		// Default constructor, makes empty document
		html_document();

		// Destructor, invalidates all node/attribute handles to this document
		~html_document();

		// Removes all nodes, leaving the empty document
		void reset();

		// Removes all nodes, then copies the entire contents of the specified document
		void reset(const html_document& proto);

	#ifndef PUGIHTML_NO_STL
		// Load document from stream.
		html_parse_result load(std::basic_istream<char, std::char_traits<char> >& stream, unsigned int options = html_parser::parse_default, html_encoding encoding = encoding_auto);
		html_parse_result load(std::basic_istream<wchar_t, std::char_traits<wchar_t> >& stream, unsigned int options = html_parser::parse_default);
	#endif

		// Load document from zero-terminated string. No encoding conversions are applied.
		html_parse_result load(const char_t* contents, unsigned int options = html_parser::parse_default);

		// Load document from file
		html_parse_result load_file(const char* path, unsigned int options = html_parser::parse_default, html_encoding encoding = encoding_auto);
		html_parse_result load_file(const wchar_t* path, unsigned int options = html_parser::parse_default, html_encoding encoding = encoding_auto);

		// Load document from buffer. Copies/converts the buffer, so it may be deleted or changed after the function returns.
		html_parse_result load_buffer(const void* contents, size_t size, unsigned int options = html_parser::parse_default, html_encoding encoding = encoding_auto);

		// Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
		// You should ensure that buffer data will persist throughout the document's lifetime, and free the buffer memory manually once document is destroyed.
		html_parse_result load_buffer_inplace(void* contents, size_t size, unsigned int options = html_parser::parse_default, html_encoding encoding = encoding_auto);

		// Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
		// You should allocate the buffer with pugihtml allocation function; document will free the buffer when it is no longer needed (you can't use it anymore).
		html_parse_result load_buffer_inplace_own(void* contents, size_t size, unsigned int options = html_parser::parse_default, html_encoding encoding = encoding_auto);

		// Save HTML document to writer (semantics is slightly different from html_node::print, see documentation for details).
		void save(html_writer& writer, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, html_encoding encoding = encoding_auto) const;

	#ifndef PUGIHTML_NO_STL
		// Save HTML document to stream (semantics is slightly different from html_node::print, see documentation for details).
		void save(std::basic_ostream<char, std::char_traits<char> >& stream, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, html_encoding encoding = encoding_auto) const;
		void save(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default) const;
	#endif

		// Save HTML to file
		bool save_file(const char* path, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, html_encoding encoding = encoding_auto) const;
		bool save_file(const wchar_t* path, const char_t* indent = PUGIHTML_TEXT("\t"), unsigned int flags = format_default, html_encoding encoding = encoding_auto) const;

		// Get document element
		html_node document_element() const;
	};

#ifndef PUGIHTML_NO_XPATH
	// XPath query return type
	enum xpath_value_type
	{
		xpath_type_none,	  // Unknown type (query failed to compile)
		xpath_type_node_set,  // Node set (xpath_node_set)
		xpath_type_number,	  // Number
		xpath_type_string,	  // String
		xpath_type_boolean	  // Boolean
	};

	// XPath parsing result
	struct PUGIHTML_CLASS xpath_parse_result
	{
		// Error message (0 if no error)
		const char* error;

		// Last parsed offset (in char_t units from string start)
		ptrdiff_t offset;

		// Default constructor, initializes object to failed state
		xpath_parse_result();

		// Cast to bool operator
		operator bool() const;

		// Get error description
		const char* description() const;
	};

	// A single XPath variable
	class PUGIHTML_CLASS xpath_variable
	{
		friend class xpath_variable_set;

	protected:
		xpath_value_type _type;
		xpath_variable* _next;

		xpath_variable();

		// Non-copyable semantics
		xpath_variable(const xpath_variable&);
		xpath_variable& operator=(const xpath_variable&);

	public:
		// Get variable name
		const char_t* name() const;

		// Get variable type
		xpath_value_type type() const;

		// Get variable value; no type conversion is performed, default value (false, NaN, empty string, empty node set) is returned on type mismatch error
		bool get_boolean() const;
		double get_number() const;
		const char_t* get_string() const;
		const xpath_node_set& get_node_set() const;

		// Set variable value; no type conversion is performed, false is returned on type mismatch error
		bool set(bool value);
		bool set(double value);
		bool set(const char_t* value);
		bool set(const xpath_node_set& value);
	};

	// A set of XPath variables
	class PUGIHTML_CLASS xpath_variable_set
	{
	private:
		xpath_variable* _data[64];

		// Non-copyable semantics
		xpath_variable_set(const xpath_variable_set&);
		xpath_variable_set& operator=(const xpath_variable_set&);

		xpath_variable* find(const char_t* name) const;

	public:
		// Default constructor/destructor
		xpath_variable_set();
		~xpath_variable_set();

		// Add a new variable or get the existing one, if the types match
		xpath_variable* add(const char_t* name, xpath_value_type type);

		// Set value of an existing variable; no type conversion is performed, false is returned if there is no such variable or if types mismatch
		bool set(const char_t* name, bool value);
		bool set(const char_t* name, double value);
		bool set(const char_t* name, const char_t* value);
		bool set(const char_t* name, const xpath_node_set& value);

		// Get existing variable by name
		xpath_variable* get(const char_t* name);
		const xpath_variable* get(const char_t* name) const;
	};

	// A compiled XPath query object
	class PUGIHTML_CLASS xpath_query
	{
	private:
		void* _impl;
		xpath_parse_result _result;

		typedef void* xpath_query::*unspecified_bool_type;

		// Non-copyable semantics
		xpath_query(const xpath_query&);
		xpath_query& operator=(const xpath_query&);

	public:
		// Construct a compiled object from XPath expression.
		// If PUGIHTML_NO_EXCEPTIONS is not defined, throws xpath_exception on compilation errors.
		explicit xpath_query(const char_t* query, xpath_variable_set* variables = 0);

		// Destructor
		~xpath_query();

		// Get query expression return type
		xpath_value_type return_type() const;

		// Evaluate expression as boolean value in the specified context; performs type conversion if necessary.
		// If PUGIHTML_NO_EXCEPTIONS is not defined, throws std::bad_alloc on out of memory errors.
		bool evaluate_boolean(const xpath_node& n) const;

		// Evaluate expression as double value in the specified context; performs type conversion if necessary.
		// If PUGIHTML_NO_EXCEPTIONS is not defined, throws std::bad_alloc on out of memory errors.
		double evaluate_number(const xpath_node& n) const;

	#ifndef PUGIHTML_NO_STL
		// Evaluate expression as string value in the specified context; performs type conversion if necessary.
		// If PUGIHTML_NO_EXCEPTIONS is not defined, throws std::bad_alloc on out of memory errors.
		string_t evaluate_string(const xpath_node& n) const;
	#endif

		// Evaluate expression as string value in the specified context; performs type conversion if necessary.
		// At most capacity characters are written to the destination buffer, full result size is returned (includes terminating zero).
		// If PUGIHTML_NO_EXCEPTIONS is not defined, throws std::bad_alloc on out of memory errors.
		// If PUGIHTML_NO_EXCEPTIONS is defined, returns empty	set instead.
		size_t evaluate_string(char_t* buffer, size_t capacity, const xpath_node& n) const;

		// Evaluate expression as node set in the specified context.
		// If PUGIHTML_NO_EXCEPTIONS is not defined, throws xpath_exception on type mismatch and std::bad_alloc on out of memory errors.
		// If PUGIHTML_NO_EXCEPTIONS is defined, returns empty node set instead.
		xpath_node_set evaluate_node_set(const xpath_node& n) const;

		// Get parsing result (used to get compilation errors in PUGIHTML_NO_EXCEPTIONS mode)
		const xpath_parse_result& result() const;

		// Safe bool conversion operator
		operator unspecified_bool_type() const;

		// Borland C++ workaround
		bool operator!() const;
	};

	#ifndef PUGIHTML_NO_EXCEPTIONS
	// XPath exception class
	class PUGIHTML_CLASS xpath_exception: public std::exception
	{
	private:
		xpath_parse_result _result;

	public:
		// Construct exception from parse result
		explicit xpath_exception(const xpath_parse_result& result);

		// Get error message
		virtual const char* what() const throw();

		// Get parse result
		const xpath_parse_result& result() const;
	};
	#endif

	// XPath node class (either html_node or html_attribute)
	class PUGIHTML_CLASS xpath_node
	{
	private:
		html_node _node;
		html_attribute _attribute;

		typedef html_node xpath_node::*unspecified_bool_type;

	public:
		// Default constructor; constructs empty XPath node
		xpath_node();

		// Construct XPath node from HTML node/attribute
		xpath_node(const html_node& node);
		xpath_node(const html_attribute& attribute, const html_node& parent);

		// Get node/attribute, if any
		html_node node() const;
		html_attribute attribute() const;

		// Get parent of contained node/attribute
		html_node parent() const;

		// Safe bool conversion operator
		operator unspecified_bool_type() const;

		// Borland C++ workaround
		bool operator!() const;

		// Comparison operators
		bool operator==(const xpath_node& n) const;
		bool operator!=(const xpath_node& n) const;
	};

#ifdef __BORLANDC__
	// Borland C++ workaround
	bool PUGIHTML_FUNCTION operator&&(const xpath_node& lhs, bool rhs);
	bool PUGIHTML_FUNCTION operator||(const xpath_node& lhs, bool rhs);
#endif

	// A fixed-size collection of XPath nodes
	class PUGIHTML_CLASS xpath_node_set
	{
	public:
		// Collection type
		enum type_t
		{
			type_unsorted,			// Not ordered
			type_sorted,			// Sorted by document order (ascending)
			type_sorted_reverse		// Sorted by document order (descending)
		};

		// Constant iterator type
		typedef const xpath_node* const_iterator;

		// Default constructor. Constructs empty set.
		xpath_node_set();

		// Constructs a set from iterator range; data is not checked for duplicates and is not sorted according to provided type, so be careful
		xpath_node_set(const_iterator begin, const_iterator end, type_t type = type_unsorted);

		// Destructor
		~xpath_node_set();

		// Copy constructor/assignment operator
		xpath_node_set(const xpath_node_set& ns);
		xpath_node_set& operator=(const xpath_node_set& ns);

		// Get collection type
		type_t type() const;

		// Get collection size
		size_t size() const;

		// Indexing operator
		const xpath_node& operator[](size_t index) const;

		// Collection iterators
		const_iterator begin() const;
		const_iterator end() const;

		// Sort the collection in ascending/descending order by document order
		void sort(bool reverse = false);

		// Get first node in the collection by document order
		xpath_node first() const;

		// Check if collection is empty
		bool empty() const;

	private:
		type_t _type;

		xpath_node _storage;

		xpath_node* _begin;
		xpath_node* _end;

		void _assign(const_iterator begin, const_iterator end);
	};
#endif

#ifndef PUGIHTML_NO_STL
	// Convert wide string to UTF8
	std::basic_string<char, std::char_traits<char>, std::allocator<char> > PUGIHTML_FUNCTION as_utf8(const wchar_t* str);
	std::basic_string<char, std::char_traits<char>, std::allocator<char> > PUGIHTML_FUNCTION as_utf8(const std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >& str);

	// Convert UTF8 to wide string
	std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > PUGIHTML_FUNCTION as_wide(const char* str);
	std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > PUGIHTML_FUNCTION as_wide(const std::basic_string<char, std::char_traits<char>, std::allocator<char> >& str);
#endif
}

#if !defined(PUGIHTML_NO_STL) && (defined(_MSC_VER) || defined(__ICC))
namespace std
{
	// Workarounds for (non-standard) iterator category detection for older versions (MSVC7/IC8 and earlier)
	std::bidirectional_iterator_tag PUGIHTML_FUNCTION _Iter_cat(const pugihtml::html_node_iterator&);
	std::bidirectional_iterator_tag PUGIHTML_FUNCTION _Iter_cat(const pugihtml::html_attribute_iterator&);
}
#endif

#if !defined(PUGIHTML_NO_STL) && defined(__SUNPRO_CC)
namespace std
{
	// Workarounds for (non-standard) iterator category detection
	std::bidirectional_iterator_tag PUGIHTML_FUNCTION __iterator_category(const pugihtml::html_node_iterator&);
	std::bidirectional_iterator_tag PUGIHTML_FUNCTION __iterator_category(const pugihtml::html_attribute_iterator&);
}
#endif

#endif
