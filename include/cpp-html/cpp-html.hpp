#ifndef PUGIHTML_HPP
#define PUGIHTML_HPP 1

#include <string>


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

} // cpp-html.

#endif /* PUGIHTML_HPP */
