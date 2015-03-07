#ifndef CPPHTML_TOKENIZER_HPP
#define CPPHTML_TOKENIZER_HPP

#include <string>

#include <cpp-html/cpp-html.hpp>


namespace cpphtml
{


enum class token_type {
	illegal,
	// '<'
	start_tag_open,
	// '</'
	end_tag_open,
	// '<!'
	exclamation_tag_open,
	// -->
	comment_close,
	// '>'
	tag_close,
	// Any textual data.
	string,
	// '='
	attribute_assignment,
	// String consisting only of whitespace symbols ' ', '\t', '\r', '\n'.
	whitespace,
};


enum class tokenizer_state {
	initial,
	tag_open_state,
};


struct token {
	token_type type;
	string_type value;
};


class token_iterator {
public:
	typedef std::string::const_iterator const_char_iterator;


	token_iterator(const std::string& html);

	token* operator->();

	bool has_next() const;

	token next();

private:
	const std::string html_;
	const_char_iterator it_html_;
	token current_token_;
	tokenizer_state state_;


	token on_initial_state();
	token scan_string_token();
};


} // cpphtml.

#endif // CPPHTML_TOKENIZER_HPP
