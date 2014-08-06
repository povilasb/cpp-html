#ifndef PUGIHTML_HTML_DOCUMENT_HPP
#define PUGIHTML_HTML_DOCUMENT_HPP 1

#include <vector>

#include "memory.hpp"
#include "html_node.hpp"
#include "common.hpp"
#include "parser.hpp"


namespace pugihtml
{

struct document_struct : public html_node_struct, public html_allocator {
	document_struct(html_memory_page* page):
		html_node_struct(page, node_document), html_allocator(page),
		buffer(0)
	{
	}

	const char_t* buffer;
};

// Document class (DOM tree root)
class PUGIHTML_CLASS document : public html_node {
private:
	char_t* _buffer;

	char _memory[192];

	// Non-copyable semantics
	document(const document&);
	const document& operator=(const document&);

	/**
	 * Initializes document: allocates memory page, etc.
	 */
	void create();

	void destroy();

	html_parse_result load_buffer_impl(void* contents, size_t size,
		unsigned int options, html_encoding encoding,
		bool is_mutable, bool own);

public:
	// Default constructor, makes empty document
	document();

	// Destructor, invalidates all node/attribute handles to this document
	~document();

	// Removes all nodes, leaving the empty document
	void reset();

	// Removes all nodes, then copies the entire contents of the specified document
	void reset(const document& proto);

#ifndef PUGIHTML_NO_STL
	// Load document from stream.
	html_parse_result load(std::basic_istream<char, std::char_traits<char> >& stream, unsigned int options = parser::parse_default, html_encoding encoding = encoding_auto);
	html_parse_result load(std::basic_istream<wchar_t, std::char_traits<wchar_t> >& stream, unsigned int options = parser::parse_default);
#endif

	// Load document from zero-terminated string. No encoding conversions are applied.
	html_parse_result load(const char_t* contents, unsigned int options = parser::parse_default);

	// Load document from file
	html_parse_result load_file(const char* path, unsigned int options = parser::parse_default, html_encoding encoding = encoding_auto);
	html_parse_result load_file(const wchar_t* path, unsigned int options = parser::parse_default, html_encoding encoding = encoding_auto);

	// Load document from buffer. Copies/converts the buffer, so it may be deleted or changed after the function returns.
	html_parse_result load_buffer(const void* contents, size_t size, unsigned int options = parser::parse_default, html_encoding encoding = encoding_auto);

	// Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
	// You should ensure that buffer data will persist throughout the document's lifetime, and free the buffer memory manually once document is destroyed.
	html_parse_result load_buffer_inplace(void* contents, size_t size, unsigned int options = parser::parse_default, html_encoding encoding = encoding_auto);

	// Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
	// You should allocate the buffer with pugihtml allocation function; document will free the buffer when it is no longer needed (you can't use it anymore).
	html_parse_result load_buffer_inplace_own(void* contents, size_t size, unsigned int options = parser::parse_default, html_encoding encoding = encoding_auto);

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

	/**
	 * Returns an array of all the links in the current document.
	 * The links collection counts <a href=""> tags and <area> tags.
	 */
	std::vector<html_node> links() const;

	/**
	 * Traverses DOM tree and searches for html node with the specified
	 * id attribute. If no tag is found, empty html node is returned.
	 */
	html_node get_element_by_id(const string_t& id);
};

} // pugihtml.

#endif /* PUGIHTML_HTML_DOCUMENT_HPP */
