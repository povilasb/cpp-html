#ifndef CPPHTML_TOKENIZER_HPP
#define CPPHTML_TOKENIZER_HPP

#include <string>
#include <unordered_map>

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
	before_attribute_name,
	attribute_name,
	before_attribute_value,
	unquoted_attribute_value
};


typedef std::unordered_map<std::string, std::string> attribute_list_type;


struct token {
	token_type type;
	string_type value;

	bool has_attributes;
	attribute_list_type attributes;
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
	// TODO(povilas): change string to string_type.
	const std::string html_;
	const_char_iterator it_html_;
	token current_token_;
	tokenizer_state state_;

	string_type curr_attribute_name_;
	string_type curr_attribute_value_;


	bool on_data_state();
	bool on_tag_open_state();
	bool on_end_tag_open_state();
	bool on_tag_name_state();
	bool on_before_attribute_name_state();
	bool on_attribute_name_state();
	bool on_before_attribute_value_state();
	bool on_unquoted_attribute_value_state();
	token scan_string_token();

	/**
	 * @return true if EOF reached while scanning.
	 */
	bool is_eof() const;

	/**
	 * Creates new current token if current character is ASCII letter.
	 * Sets new scanner state.
	 *
	 * @param type new token type.
	 * @param new_state new tokenizer state.
	 */
	void create_tag_token_if_curr_char_is_letter(token_type type,
		tokenizer_state new_state);
};


} // cpphtml.

#endif // CPPHTML_TOKENIZER_HPP
