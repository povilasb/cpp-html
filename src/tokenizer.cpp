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


token_iterator::token_iterator(const std::string& html) : html_(html),
	it_html_(this->html_.cbegin()), current_token_{token_type::illegal, ""},
	state_(tokenizer_state::initial)
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
	token next_token{token_type::illegal, ""};

	if (is_chartype(*this->it_html_, ct_start_symbol)) {
		const_char_iterator string_start = this->it_html_;
		while (is_chartype(*this->it_html_, ct_symbol)) {
			++this->it_html_;
		}

		string_type token_value(string_start, this->it_html_);
		next_token = token{token_type::string, token_value};
	}

	return next_token;
}


token
token_iterator::on_initial_state()
{
	token next_token{token_type::illegal, ""};

	// Check if the current character is the start tag character
	if (*this->it_html_ == tag_open_char) {
		++this->it_html_;
		next_token = token{token_type::start_tag_open, ""};

		this->state_ = tokenizer_state::tag_open_state;
	}
	else if (is_chartype(*this->it_html_, ct_start_symbol)) {
		next_token = this->scan_string_token();
	}

	return next_token;
}


token
token_iterator::on_tag_open_state()
{
	token next_token{token_type::illegal, ""};

	if (is_chartype(*this->it_html_, ct_start_symbol)) {
		next_token = this->scan_string_token();
	}

	return next_token;
}


token
token_iterator::next()
{
	token next_token{token_type::illegal, ""};

	switch (this->state_) {
	case tokenizer_state::initial:
		next_token = this->on_initial_state();
		break;

	case tokenizer_state::tag_open_state:
		next_token = this->on_tag_open_state();
		break;

	default:
		break;
	}

	return next_token;
}

} // cpphtml.
