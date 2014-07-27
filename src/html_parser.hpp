#ifndef PUGIHTML_HTML_PARSER_HPP
#define PUGIHTML_HTML_PARSER_HPP 1

#include <csetjmp>

#include "memory.hpp"
#include "common.hpp"
#include "html_node.hpp"


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
