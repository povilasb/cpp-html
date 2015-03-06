#ifndef CPPHTML_TOKENIZER_HPP
#define CPPHTML_TOKENIZER_HPP

#include <string>


namespace cpphtml
{


enum class token_type {
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


struct token {
	token_type type;
	std::string value;
};


class token_iterator {
public:
	token_iterator(const std::string& html);

	token* operator->();

private:
	std::string html_;
	token current_token_;
};


} // cpphtml.

#endif /* CPPHTML_TOKENIZER_HPP */
