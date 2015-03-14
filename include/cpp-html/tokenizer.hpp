#ifndef CPPHTML_TOKENIZER_HPP
#define CPPHTML_TOKENIZER_HPP

#include <string>

#include <cpp-html/cpp-html.hpp>


namespace cpphtml
{


enum class token_type {
	illegal,
	doctype,
	start_tag,
	end_tag,
	comment,
	// Any textual data.
	string,
	end_of_file
};


enum class tokenizer_state {
	data,
	tag_open,
	end_tag_open,
	tag_name,
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

	token_iterator& operator++();

	bool has_next() const;

	token next();

private:
	const std::string html_;
	const_char_iterator it_html_;
	token current_token_;
	tokenizer_state state_;


	bool on_data_state();
	bool on_tag_open_state();
	bool on_end_tag_open_state();
	bool on_tag_name_state();
	token scan_string_token();
};


} // cpphtml.

#endif // CPPHTML_TOKENIZER_HPP
