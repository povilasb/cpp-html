#ifndef CPPHTML_HPP
#define CPPHTML_HPP

#include <string>
#include <memory>


namespace cpphtml
{

#ifdef PUGIHTML_WCHAR_MODE
typedef wchar_t char_type;
#else
typedef char char_type;
#endif

// String type.
typedef std::basic_string<char_type, std::char_traits<char_type>,
	std::allocator<char_type> > string_type;

class document;
// HTML Document type.
typedef std::shared_ptr<document> document_type;

} // cpp-html.

#endif // CPPHTML_HPP
