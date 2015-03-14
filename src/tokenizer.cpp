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

const char_type tag_open_char = '<';
const char_type tag_close_char = '>';


token_iterator::token_iterator(const std::string& html) : html_(html),
	it_html_(this->html_.cbegin() - 1), current_token_{token_type::illegal, ""},
	state_(tokenizer_state::data)
{
	this->current_token_ = this->next();
}


token*
token_iterator::operator->()
{
	return &this->current_token_;
}


token_iterator&
token_iterator::operator++()
{
	this->current_token_ = this->next();
	return *this;
}


bool
token_iterator::has_next() const
{
	return this->it_html_ == this->html_.cend();
}

token
token_iterator::scan_string_token()
{
	const_char_iterator string_start = this->it_html_;
	while (is_chartype(*this->it_html_, ct_symbol)) {
		++this->it_html_;
	}

	string_type token_value(string_start, this->it_html_);
	return token{token_type::string, token_value};
}


bool
token_iterator::on_data_state()
{
	bool token_emitted = false;

	++this->it_html_;

	// Check if the current character is the start tag character
	if (*this->it_html_ == tag_open_char) {
		this->state_ = tokenizer_state::tag_open;
	}
	else if (is_chartype(*this->it_html_, ct_start_symbol)) {
		this->state_ = tokenizer_state::tag_name;

		std::string tag_name;
		tag_name = *this->it_html_;
		this->current_token_ = token{token_type::start_tag, tag_name};
	}

	return token_emitted;
}


bool
token_iterator::on_tag_open_state()
{
	bool token_emitted = false;

	++this->it_html_;

	if (is_chartype(*this->it_html_, ct_start_symbol)) {
		this->state_ = tokenizer_state::tag_name;

		std::string tag_name;
		tag_name = *this->it_html_;
		this->current_token_ = token{token_type::start_tag, tag_name};
	}

	return token_emitted;
}


bool
token_iterator::on_tag_name_state()
{
	bool token_emitted = false;

	++this->it_html_;

	while (is_chartype(*this->it_html_, ct_start_symbol)) {
		this->current_token_.value += *this->it_html_;
		++this->it_html_;
	}

	if (*this->it_html_ == tag_close_char) {
		token_emitted = true;
		this->state_ = tokenizer_state::data;
	}

	return token_emitted;
}


token
token_iterator::next()
{
	bool token_emitted = false;
	while (!token_emitted) {
		switch (this->state_) {
		case tokenizer_state::data:
			token_emitted = this->on_data_state();
			break;

		case tokenizer_state::tag_open:
			token_emitted = this->on_tag_open_state();
			break;

		case tokenizer_state::tag_name:
			token_emitted = this->on_tag_name_state();
			break;

		default:
			break;
		}
	}

	return this->current_token_;
}

} // cpphtml.
