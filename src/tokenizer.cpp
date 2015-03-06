#include <string>

#include <cpp-html/tokenizer.hpp>


namespace cpphtml
{

token_iterator::token_iterator(const std::string& html) : html_(html),
	current_token_{token_type::start_tag_open, ""}
{
}


token*
token_iterator::operator->()
{
	return &this->current_token_;
}

} // cpphtml.
