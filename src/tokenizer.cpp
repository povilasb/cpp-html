#include <string>

#include <cpp-html/tokenizer.hpp>
#include <cpp-html/parser.hpp>


namespace cpphtml
{

/**
 * This table maps ASCII symbols with their possible types in enum chartype_t.
 */
const unsigned char chartype_table[256] = {
	55,  0,   0,   0,   0,   0,   0,   0,      0,   12,  12,  0,   0,   62,  0,   0,   // 0-15
	0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 16-31
	10,   0,   4,   0,   0,   0,   4,   4,      0,   0,   0,   0,   0,   96,  64,  0,   // 32-47
	64,  64,  64,  64,  64,  64,  64,  64,     64,  64,  192, 0,   1,   0,   50,  0,   // 48-63
	0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 64-79
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0,   0,   16,  0,   192, // 80-95
	0,   192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 96-111
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0, 0, 0, 0, 0,           // 112-127

	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 128+
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192
};


inline bool
is_chartype(char_type ch, enum chartype_t char_type)
{
	return chartype_table[static_cast<unsigned char>(ch)] & (char_type);
}


token_iterator::token_iterator(const std::string& html) : html_(html),
	current_token_{token_type::illegal, ""}
{
	this->it_html_ = this->html_.cbegin();
	this->current_token_ = this->next();
}


token*
token_iterator::operator->()
{
	return &this->current_token_;
}


bool
token_iterator::has_next() const
{
	return this->it_html_ == this->html_.cend();
}


token
token_iterator::next()
{
	token next_token{token_type::illegal, ""};
	// Check if the current character is the start tag character
	if (*this->it_html_ == '<') {
		++this->it_html_;
		next_token = token{token_type::start_tag_open, ""};
	}
	else if (is_chartype(*this->it_html_, ct_start_symbol)) {
		const_char_iterator string_start = this->it_html_;
		while (is_chartype(*this->it_html_, ct_symbol)) {
			++this->it_html_;
		}

		string_type token_value(string_start, this->it_html_);
		next_token = token{token_type::string, token_value};
	}

	return next_token;
}

} // cpphtml.
