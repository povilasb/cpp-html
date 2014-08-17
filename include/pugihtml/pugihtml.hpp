#ifndef PUGIHTML_HPP
#define PUGIHTML_HPP 1

#include <string>


namespace pugihtml
{

// Macro for deprecated features
#ifndef PUGIHTML_DEPRECATED
#	if defined(__GNUC__)
#		define PUGIHTML_DEPRECATED __attribute__((deprecated))
#	elif defined(_MSC_VER) && _MSC_VER >= 1300
#		define PUGIHTML_DEPRECATED __declspec(deprecated)
#	else
#		define PUGIHTML_DEPRECATED
#	endif
#endif


// Inlining controls
#if defined(_MSC_VER) && _MSC_VER >= 1300
#	define PUGIHTML_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#	define PUGIHTML_NO_INLINE __attribute__((noinline))
#else
#	define PUGIHTML_NO_INLINE
#endif


#ifdef PUGIHTML_WCHAR_MODE
typedef wchar_t char_type;
#else
typedef char char_type;
#endif

typedef std::basic_string<char_type, std::char_traits<char_type>,
	std::allocator<char_type> > string_type;

} // pugihtml.

#endif /* PUGIHTML_HPP */

