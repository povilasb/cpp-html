#ifndef PUGIHTML_PARSER_HPP
#define PUGIHTML_PARSER_HPP 1

#include <cstddef>
#include <stdexcept>

#include <pugihtml/pugihtml.hpp>
#include <pugihtml/document.hpp>


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


/**
 * Parsing status, returned as part of html_parse_result object.
 */
enum parse_status {
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

class parse_error : public std::runtime_error {
public:
	parse_error(const std::string& what) : std::runtime_error(what)
	{
	}

	parse_error(parse_status status) : std::runtime_error(""),
		status_(status)
	{
	}

	parse_status status() const
	{
		return this->status_;
	}

private:
	parse_status status_;
};


class parser {
public:
	// Parsing options

	// Minimal parsing mode (equivalent to turning all other flags off).
	// Only elements and PCDATA sections are added to the DOM tree, no text
	// conversions are performed.
	static const unsigned int parse_minimal = 0x0000;

	// This flag determines if processing instructions (node_pi) are added
	// to the DOM tree. This flag is off by default.
	static const unsigned int parse_pi = 0x0001;

	// This flag determines if comments (node_comment) are added to the
	// DOM tree. This flag is off by default.
	static const unsigned int parse_comments = 0x0002;

	// This flag determines if CDATA sections (node_cdata) are added to
	// the DOM tree. This flag is on by default.
	static const unsigned int parse_cdata = 0x0004;

	// This flag determines if plain character data (node_pcdata) that
	// consist only of whitespace are added to the DOM tree.
	// This flag is off by default; turning it on usually results in slower
	// parsing and more memory consumption.
	static const unsigned int parse_ws_pcdata = 0x0008;

	// This flag determines if character and entity references are expanded
	// during parsing. This flag is on by default.
	static const unsigned int parse_escapes = 0x0010;

	// This flag determines if EOL characters are normalized
	// (converted to #xA) during parsing. This flag is on by default.
	static const unsigned int parse_eol = 0x0020;

	// This flag determines if attribute values are normalized using CDATA
	// normalization rules during parsing. This flag is on by default.
	static const unsigned int parse_wconv_attribute = 0x0040;

	// This flag determines if attribute values are normalized using
	// NMTOKENS normalization rules during parsing. This flag is off by default.
	static const unsigned int parse_wnorm_attribute = 0x0080;

	// This flag determines if document declaration (node_declaration) is
	// added to the DOM tree. This flag is off by default.
	static const unsigned int parse_declaration = 0x0100;

	// This flag determines if document type declaration (node_doctype) is
	// added to the DOM tree. This flag is off by default.
	static const unsigned int parse_doctype = 0x0200;

	// The default parsing mode.
	// Elements, PCDATA and CDATA sections are added to the DOM tree,
	// character/reference entities are expanded, End-of-Line characters
	// are normalized, attribute values are normalized using CDATA
	// normalization rules.
	static const unsigned int parse_default = parse_cdata | parse_escapes
		| parse_wconv_attribute | parse_eol;

	// The full parsing mode.
	// Nodes of all types are added to the DOM tree, character/reference
	// entities are expanded, End-of-Line characters are normalized,
	// attribute values are normalized using CDATA normalization rules.
	static const unsigned int parse_full = parse_default | parse_pi
		| parse_comments | parse_declaration | parse_doctype;


	/**
	 * Advances the specified string pointer to the end of doctype element.
	 *
	 * DOCTYPE consists of nested sections of the following possible types:
	 * 1. <!-- ... -->, <? ... ?>, "...", '...'
	 * 2. <![...]]>
	 * 3. <!...>
	 * First group can not contain nested groups.
	 * Second group can contain nested groups of the same type.
	 * Third group can contain all other groups.
	 */
	const char_type* parse_doctype_primitive(const char_type* s);

/*
	const char_type* parse_doctype_ignore(const char_type* s);

	const char_type* parse_doctype_group(const char_type* s,
		char_type endch, bool toplevel);
*/

	/**
	 * Parse node contents, starting with exclamation mark.
	 */
	//char_type* parse_exclamation(char_type* s, node_struct* cursor,
	//	unsigned int optmsk, char_type endch);

	//char_type* parse_question(char_type* s, node_struct*& ref_cursor,
	//	unsigned int optmsk, char_type endch);

	/**
	 * Parses the specified HTML string and returns document object
	 * representing the HTML document tree.
	 *
	 * @param optmask parsing options defined in pugihtml.hpp. E.g.
	 *	You can configure to parse and add comment nodes to the
	 *	DOM tree.
	 */
	//std::shared_ptr<document> parse(const string_type& str_html,
	//	unsigned int optmsk);

	/**
	 * @return parse status description.
	 */
	string_type status_description() const;

private:
	parse_status status_ = status_ok;

	// Last parsed offset (in char_type units from start of input data).
	ptrdiff_t offset_ = 0;

	// Source document encoding.
	html_encoding encoding_ = encoding_utf8;

	char_type* error_offset_ = nullptr;
};

} //pugihtml.

#endif /* PUGIHTML_PARSER_HPP */
