#ifndef PUGIHTML_HTML_PARSER_HPP
#define PUGIHTML_HTML_PARSER_HPP 1

#include <csetjmp>

#include "memory.hpp"
#include "common.hpp"
#include "html_node.hpp"
#include "pugihtml.hpp"


namespace pugihtml
{

enum chartype_t {
	ct_parse_pcdata = 1, // \0, &, \r, <
	ct_parse_attr = 2, // \0, &, \r, ', "
	ct_parse_attr_ws = 4, // \0, &, \r, ', ", \n, tab
	ct_space = 8, // \r, \n, space, tab
	ct_parse_cdata = 16, // \0, ], >, \r
	ct_parse_comment = 32, // \0, -, >, \r
	ct_symbol = 64, // Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
	ct_start_symbol = 128 // Any symbol > 127, a-z, A-Z, _, :
};


// Parsing status, returned as part of html_parse_result object
enum html_parse_status {
	status_ok = 0,              // No error

	status_file_not_found,      // File was not found during load_file()
	status_io_error,            // Error reading from file/stream
	status_out_of_memory,       // Could not allocate memory
	status_internal_error,      // Internal error occurred

	status_unrecognized_tag,    // Parser could not determine tag type

	status_bad_pi,              // Parsing error occurred while parsing document declaration/processing instruction
	status_bad_comment,         // Parsing error occurred while parsing comment
	status_bad_cdata,           // Parsing error occurred while parsing CDATA section
	status_bad_doctype,         // Parsing error occurred while parsing document type declaration
	status_bad_pcdata,          // Parsing error occurred while parsing PCDATA section
	status_bad_start_element,   // Parsing error occurred while parsing start element tag
	status_bad_attribute,       // Parsing error occurred while parsing element attribute
	status_bad_end_element,     // Parsing error occurred while parsing end element tag
	status_end_element_mismatch // There was a mismatch of start-end tags (closing tag had incorrect name, some tag was not closed or there was an excessive closing tag)
};


// Parsing result
struct html_parse_result {
	// Parsing status (see html_parse_status)
	html_parse_status status;

	// Last parsed offset (in char_t units from start of input data)
	ptrdiff_t offset;

	// Source document encoding
	html_encoding encoding;

	// Default constructor, initializes object to failed state
	html_parse_result();

	// Cast to bool operator
	operator bool() const;

	// Get error description
	const char* description() const;
};


struct html_parser {
public:
	html_parser(const html_allocator& alloc);

	/**
	 * DOCTYPE consists of nested sections of the following possible types:
	 * 1. <!-- ... -->, <? ... ?>, "...", '...'
	 * 2. <![...]]>
	 * 3. <!...>
	 * First group can not contain nested groups.
	 * Second group can contain nested groups of the same type.
	 * Third group can contain all other groups.
	 */
	char_t* parse_doctype_primitive(char_t* s);

	char_t* parse_doctype_ignore(char_t* s);

	char_t* parse_doctype_group(char_t* s, char_t endch, bool toplevel);

	char_t* parse_exclamation(char_t* s, html_node_struct* cursor,
		unsigned int optmsk, char_t endch);

	char_t* parse_question(char_t* s, html_node_struct*& ref_cursor,
		unsigned int optmsk, char_t endch);

	void parse(char_t* s, html_node_struct* htmldoc, unsigned int optmsk,
		char_t endch);

	static html_parse_result parse(char_t* buffer, size_t length,
		html_node_struct* root, unsigned int optmsk);

//private:
	html_allocator alloc;
	char_t* error_offset;
	jmp_buf error_handler;
};

} //pugihtml.

#endif /* PUGIHTML_HTML_PARSER_HPP */
