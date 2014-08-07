/**
 * pugihtml parser - version 0.1
 * --------------------------------------------------------
 * Copyright (c) 2012 Adgooroo, LLC (kgantchev [AT] adgooroo [DOT] com)
 *
 * This library is distributed under the MIT License. See notice in license.txt
 *
 * This work is based on the pugxml parser, which is:
 * Copyright (C) 2006-2010, by Arseny Kapoulkine (arseny [DOT] kapoulkine [AT] gmail [DOT] com)
 */

#include "pugihtml.hpp"
#include "pugiutil.hpp"
#include "memory.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "html_writer.hpp"
#include "utf_decoder.hpp"
#include "document.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <wchar.h>
#include <set>

#ifndef PUGIHTML_NO_XPATH
#	include <math.h>
#	include <float.h>
#endif

#ifndef PUGIHTML_NO_STL
#	include <istream>
#	include <ostream>
#	include <string>
#	include <algorithm>
#endif

// For placement new
#include <new>

#ifdef _MSC_VER
#	pragma warning(disable: 4127) // conditional expression is constant
#	pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#	pragma warning(disable: 4611) // interaction between '_setjmp' and C++ object destruction is non-portable
#	pragma warning(disable: 4702) // unreachable code
#	pragma warning(disable: 4996) // this function or variable may be unsafe
#endif

#ifdef __INTEL_COMPILER
#	pragma warning(disable: 177) // function was declared but never referenced
#	pragma warning(disable: 1478 1786) // function was declared "deprecated"
#endif

#ifdef __BORLANDC__
#	pragma warn -8008 // condition is always false
#	pragma warn -8066 // unreachable code
#endif

#ifdef __SNC__
#	pragma diag_suppress=178 // function was declared but never referenced
#	pragma diag_suppress=237 // controlling expression is constant
#endif

// uintptr_t
#if !defined(_MSC_VER) || _MSC_VER >= 1600
#	include <stdint.h>
#else
#	if _MSC_VER < 1300
// No native uintptr_t in MSVC6
typedef size_t uintptr_t;
#	endif
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef __int32 int32_t;
#endif

// Inlining controls
#if defined(_MSC_VER) && _MSC_VER >= 1300
#	define PUGIHTML_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#	define PUGIHTML_NO_INLINE __attribute__((noinline))
#else
#	define PUGIHTML_NO_INLINE
#endif


using namespace pugihtml;

// TODO(povilas): remove these constants when parser is removed from
//	this file.

	// Parsing options

	// Minimal parsing mode (equivalent to turning all other flags off).
	// Only elements and PCDATA sections are added to the DOM tree, no text
	// conversions are performed.
	static const unsigned int parse_minimal = 0x0000;

	// This flag determines if processing instructions (node_pi) are added
	// to the DOM tree. This flag is off by default.
	static const unsigned int parse_pi = 0x0001;

	// This flag determines if comments (node_comment) are added to the
	// DOM tree. This flag is off by default.
	static const unsigned int parse_comments = 0x0002;

	// This flag determines if CDATA sections (node_cdata) are added to
	// the DOM tree. This flag is on by default.
	static const unsigned int parse_cdata = 0x0004;

	// This flag determines if plain character data (node_pcdata) that
	// consist only of whitespace are added to the DOM tree.
	// This flag is off by default; turning it on usually results in slower
	// parsing and more memory consumption.
	static const unsigned int parse_ws_pcdata = 0x0008;

	// This flag determines if character and entity references are expanded
	// during parsing. This flag is on by default.
	static const unsigned int parse_escapes = 0x0010;

	// This flag determines if EOL characters are normalized
	// (converted to #xA) during parsing. This flag is on by default.
	static const unsigned int parse_eol = 0x0020;

	// This flag determines if attribute values are normalized using CDATA
	// normalization rules during parsing. This flag is on by default.
	static const unsigned int parse_wconv_attribute = 0x0040;

	// This flag determines if attribute values are normalized using
	// NMTOKENS normalization rules during parsing. This flag is off by default.
	static const unsigned int parse_wnorm_attribute = 0x0080;

	// This flag determines if document declaration (node_declaration) is
	// added to the DOM tree. This flag is off by default.
	static const unsigned int parse_declaration = 0x0100;

	// This flag determines if document type declaration (node_doctype) is
	// added to the DOM tree. This flag is off by default.
	static const unsigned int parse_doctype = 0x0200;

	// The default parsing mode.
	// Elements, PCDATA and CDATA sections are added to the DOM tree,
	// character/reference entities are expanded, End-of-Line characters
	// are normalized, attribute values are normalized using CDATA
	// normalization rules.
	static const unsigned int parse_default = parse_cdata | parse_escapes
		| parse_wconv_attribute | parse_eol;

	// The full parsing mode.
	// Nodes of all types are added to the DOM tree, character/reference
	// entities are expanded, End-of-Line characters are normalized,
	// attribute values are normalized using CDATA normalization rules.
	static const unsigned int parse_full = parse_default | parse_pi
		| parse_comments | parse_declaration | parse_doctype;

// String utilities
namespace
{
#ifdef PUGIHTML_NO_STL
	char_t* find_first_of(char_t* first1,char_t* last1,char_t* first2,char_t* last2){
		for ( ; first1 != last1; ++first1 )
			for (char_t* it=first2; it!=last2; ++it)
				if (*it==*first1)		   // or: if (comp(*it,*first)) for the pred version
					return first1;
		return last1;
	}
#else
#define find_first_of std::find_first_of
#endif

#ifdef PUGIHTML_WCHAR_MODE
	// Convert string to wide string, assuming all symbols are ASCII
	void widen_ascii(wchar_t* dest, const char* source)
	{
		for (const char* i = source; *i; ++i) *dest++ = *i;
		*dest = 0;
	}
#endif
}


namespace
{
	// TODO(povilas): use from document.hpp.
	struct document_struct: public node_struct, public html_allocator
	{
		document_struct(html_memory_page* page): node_struct(page, node_document), html_allocator(page), buffer(0)
		{
		}

		const char_t* buffer;
	};

}


// Unicode utilities
namespace
{
	struct utf16_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

		static value_type high(value_type result, uint32_t)
		{
			return result + 2;
		}
	};

	struct utf32_counter
	{
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t)
		{
			return result + 1;
		}

		static value_type high(value_type result, uint32_t)
		{
			return result + 1;
		}
	};

	template <size_t size> struct wchar_selector;

	template <> struct wchar_selector<2>
	{
		typedef uint16_t type;
		typedef utf16_counter counter;
		typedef utf16_writer writer;
	};

	template <> struct wchar_selector<4>
	{
		typedef uint32_t type;
		typedef utf32_counter counter;
		typedef utf32_writer writer;
	};

	typedef wchar_selector<sizeof(wchar_t)>::counter wchar_counter;
	typedef wchar_selector<sizeof(wchar_t)>::writer wchar_writer;

	inline void convert_wchar_endian_swap(wchar_t* result, const wchar_t* data, size_t length)
	{
		for (size_t i = 0; i < length; ++i) result[i] = static_cast<wchar_t>(endian_swap(static_cast<wchar_selector<sizeof(wchar_t)>::type>(data[i])));
	}
}

namespace
{
	enum chartype_t
	{
		ct_parse_pcdata = 1,	// \0, &, \r, <
		ct_parse_attr = 2,		// \0, &, \r, ', "
		ct_parse_attr_ws = 4,	// \0, &, \r, ', ", \n, tab
		ct_space = 8,			// \r, \n, space, tab
		ct_parse_cdata = 16,	// \0, ], >, \r
		ct_parse_comment = 32,	// \0, -, >, \r
		ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
		ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
	};

	const unsigned char chartype_table[256] =
	{
		55,  0,   0,   0,	0,	 0,   0,   0,	   0,	12,  12,  0,   0,	63,  0,   0,   // 0-15
		0,	 0,   0,   0,	0,	 0,   0,   0,	   0,	0,	 0,   0,   0,	0,	 0,   0,   // 16-31
		8,	 0,   6,   0,	0,	 0,   7,   6,	   0,	0,	 0,   0,   0,	96,  64,  0,   // 32-47
		64,  64,  64,  64,	64,  64,  64,  64,	   64,	64,  192, 0,   1,	0,	 48,  0,   // 48-63
		0,	 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 64-79
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0,   0,	16,  0,   192, // 80-95
		0,	 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 96-111
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 0, 0, 0, 0, 0,		   // 112-127

		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192, // 128+
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192,
		192, 192, 192, 192, 192, 192, 192, 192,    192, 192, 192, 192, 192, 192, 192, 192
	};

	const unsigned char chartypex_table[256] =
	{
		3,	3,	3,	3,	3,	3,	3,	3,	   3,  0,  2,  3,  3,  2,  3,  3,	  // 0-15
		3,	3,	3,	3,	3,	3,	3,	3,	   3,  3,  3,  3,  3,  3,  3,  3,	  // 16-31
		0,	0,	2,	0,	0,	0,	3,	0,	   0,  0,  0,  0,  0, 16, 16,  0,	  // 32-47
		24, 24, 24, 24, 24, 24, 24, 24,    24, 24, 0,  0,  3,  0,  3,  0,	  // 48-63

		0,	20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 64-79
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  20,	  // 80-95
		0,	20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 96-111
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 0,  0,  0,  0,  0,	  // 112-127

		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,	  // 128+
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20,    20, 20, 20, 20, 20, 20, 20, 20
	};

#ifdef PUGIHTML_WCHAR_MODE
	#define IS_CHARTYPE_IMPL(c, ct, table) ((static_cast<unsigned int>(c) < 128 ? table[static_cast<unsigned int>(c)] : table[128]) & (ct))
#else
	#define IS_CHARTYPE_IMPL(c, ct, table) (table[static_cast<unsigned char>(c)] & (ct))
#endif

	#define IS_CHARTYPE(c, ct) IS_CHARTYPE_IMPL(c, ct, chartype_table)
	#define IS_CHARTYPEX(c, ct) IS_CHARTYPE_IMPL(c, ct, chartypex_table)

#ifndef PUGIHTML_NO_STL
	std::wstring as_wide_impl(const char* str, size_t size)
	{
		const uint8_t* data = reinterpret_cast<const uint8_t*>(str);

		// first pass: get length in wchar_t units
		size_t length = utf_decoder<wchar_counter>::decode_utf8_block(data, size, 0);

		// allocate resulting string
		std::wstring result;
		result.resize(length);

		// second pass: convert to wchar_t
		if (length > 0)
		{
			wchar_writer::value_type begin = reinterpret_cast<wchar_writer::value_type>(&result[0]);
			wchar_writer::value_type end = utf_decoder<wchar_writer>::decode_utf8_block(data, size, begin);

			assert(begin + length == end);
			(void)!end;
		}

		return result;
	}
#endif

	// Utility macro for last character handling
	#define ENDSWITH(c, e) ((c) == (e) || ((c) == 0 && endch == (e)))

	typedef char_t* (*strconv_pcdata_t)(char_t*);

	template <typename opt_eol, typename opt_escape> struct strconv_pcdata_impl
	{
		static char_t* parse(char_t* s)
		{
			gap g;

			while (true)
			{
				while (!IS_CHARTYPE(*s, ct_parse_pcdata)) ++s;

				if (*s == '<') // PCDATA ends here
				{
					*g.flush(s) = 0;

					return s + 1;
				}
				else if (opt_eol::value && *s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
				{
					*s++ = '\n'; // replace first one with 0x0a

					if (*s == '\n') g.push(s, 1);
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (*s == 0)
				{
					return s;
				}
				else ++s;
			}
		}
	};

	strconv_pcdata_t get_strconv_pcdata(unsigned int optmask)
	{
		STATIC_ASSERT(parser::parse_escapes == 0x10
			&& parser::parse_eol == 0x20);

		switch ((optmask >> 4) & 3) // get bitmask for flags (eol escapes)
		{
		case 0: return strconv_pcdata_impl<opt_false, opt_false>::parse;
		case 1: return strconv_pcdata_impl<opt_false, opt_true>::parse;
		case 2: return strconv_pcdata_impl<opt_true, opt_false>::parse;
		case 3: return strconv_pcdata_impl<opt_true, opt_true>::parse;
		default: return 0; // should not get here
		}
	}

	typedef char_t* (*strconv_attribute_t)(char_t*, char_t);

	template <typename opt_escape> struct strconv_attribute_impl
	{
		static char_t* parse_wnorm(char_t* s, char_t end_quote)
		{
			gap g;

			// trim leading whitespaces
			if (IS_CHARTYPE(*s, ct_space))
			{
				char_t* str = s;

				do ++str;
				while (IS_CHARTYPE(*str, ct_space));

				g.push(s, str - s);
			}

			while (true)
			{
				while (!IS_CHARTYPE(*s, ct_parse_attr_ws | ct_space)) ++s;

				if (*s == end_quote)
				{
					char_t* str = g.flush(s);

					do *str-- = 0;
					while (IS_CHARTYPE(*str, ct_space));

					return s + 1;
				}
				else if (IS_CHARTYPE(*s, ct_space))
				{
					*s++ = ' ';

					if (IS_CHARTYPE(*s, ct_space))
					{
						char_t* str = s + 1;
						while (IS_CHARTYPE(*str, ct_space)) ++str;

						g.push(s, str - s);
					}
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}

		static char_t* parse_wconv(char_t* s, char_t end_quote)
		{
			gap g;

			while (true)
			{
				while (!IS_CHARTYPE(*s, ct_parse_attr_ws)) ++s;

				if (*s == end_quote)
				{
					*g.flush(s) = 0;

					return s + 1;
				}
				else if (IS_CHARTYPE(*s, ct_space))
				{
					if (*s == '\r')
					{
						*s++ = ' ';

						if (*s == '\n') g.push(s, 1);
					}
					else *s++ = ' ';
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}

		static char_t* parse_eol(char_t* s, char_t end_quote)
		{
			gap g;

			while (true)
			{
				while (!IS_CHARTYPE(*s, ct_parse_attr)) ++s;

				if (*s == end_quote)
				{
					*g.flush(s) = 0;

					return s + 1;
				}
				else if (*s == '\r')
				{
					*s++ = '\n';

					if (*s == '\n') g.push(s, 1);
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}

		static char_t* parse_simple(char_t* s, char_t end_quote)
		{
			gap g;

			while (true)
			{
				while (!IS_CHARTYPE(*s, ct_parse_attr)) ++s;

				if (*s == end_quote)
				{
					*g.flush(s) = 0;

					return s + 1;
				}
				else if (opt_escape::value && *s == '&')
				{
					s = strconv_escape(s, g);
				}
				else if (!*s)
				{
					return 0;
				}
				else ++s;
			}
		}
	};

	strconv_attribute_t get_strconv_attribute(unsigned int optmask)
	{
		STATIC_ASSERT(parser::parse_escapes == 0x10
			&& parser::parse_eol == 0x20
			&& parser::parse_wconv_attribute == 0x40
			&& parser::parse_wnorm_attribute == 0x80);

		switch ((optmask >> 4) & 15) // get bitmask for flags (wconv wnorm eol escapes)
		{
		case 0:  return strconv_attribute_impl<opt_false>::parse_simple;
		case 1:  return strconv_attribute_impl<opt_true>::parse_simple;
		case 2:  return strconv_attribute_impl<opt_false>::parse_eol;
		case 3:  return strconv_attribute_impl<opt_true>::parse_eol;
		case 4:  return strconv_attribute_impl<opt_false>::parse_wconv;
		case 5:  return strconv_attribute_impl<opt_true>::parse_wconv;
		case 6:  return strconv_attribute_impl<opt_false>::parse_wconv;
		case 7:  return strconv_attribute_impl<opt_true>::parse_wconv;
		case 8:  return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 9:  return strconv_attribute_impl<opt_true>::parse_wnorm;
		case 10: return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 11: return strconv_attribute_impl<opt_true>::parse_wnorm;
		case 12: return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 13: return strconv_attribute_impl<opt_true>::parse_wnorm;
		case 14: return strconv_attribute_impl<opt_false>::parse_wnorm;
		case 15: return strconv_attribute_impl<opt_true>::parse_wnorm;
		default: return 0; // should not get here
		}
	}

	// TODO(povilas): remove
	inline html_parse_result make_parse_result(html_parse_status status, ptrdiff_t offset = 0)
	{
		html_parse_result result;
		result.status = status;
		result.offset = offset;

		return result;
	}

	// TODO(povilas): remove.
	struct parser
	{
		html_allocator alloc;
		char_t* error_offset;
		jmp_buf error_handler;

		// Parser utilities.
		#define SKIPWS()			{ while (IS_CHARTYPE(*s, ct_space)) ++s; }
		#define OPTSET(OPT)			( optmsk & OPT )
		#define PUSHNODE(TYPE)		{ cursor = append_node(cursor, alloc, TYPE); if (!cursor) THROW_ERROR(status_out_of_memory, s); }
		#define POPNODE()			{ cursor = cursor->parent; }
		#define SCANFOR(X)			{ while (*s != 0 && !(X)) ++s; }
		#define SCANWHILE(X)		{ while ((X)) ++s; }
		#define ENDSEG()			{ ch = *s; *s = 0; ++s; }
		#define THROW_ERROR(err, m)	error_offset = m, longjmp(error_handler, err)
		#define CHECK_ERROR(err, m)	{ if (*s == 0) THROW_ERROR(err, m); }

		parser(const html_allocator& alloc): alloc(alloc), error_offset(0)
		{
		}

		// DOCTYPE consists of nested sections of the following possible types:
		// <!-- ... -->, <? ... ?>, "...", '...'
		// <![...]]>
		// <!...>
		// First group can not contain nested groups
		// Second group can contain nested groups of the same type
		// Third group can contain all other groups
		char_t* parse_doctype_primitive(char_t* s)
		{
			if (*s == '"' || *s == '\'')
			{
				// quoted string
				char_t ch = *s++;
				SCANFOR(*s == ch);
				if (!*s) THROW_ERROR(status_bad_doctype, s);

				s++;
			}
			else if (s[0] == '<' && s[1] == '?')
			{
				// <? ... ?>
				s += 2;
				SCANFOR(s[0] == '?' && s[1] == '>'); // no need for ENDSWITH because ?> can't terminate proper doctype
				if (!*s) THROW_ERROR(status_bad_doctype, s);

				s += 2;
			}
			else if (s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-')
			{
				s += 4;
				SCANFOR(s[0] == '-' && s[1] == '-' && s[2] == '>'); // no need for ENDSWITH because --> can't terminate proper doctype
				if (!*s) THROW_ERROR(status_bad_doctype, s);

				s += 4;
			}
			else THROW_ERROR(status_bad_doctype, s);

			return s;
		}

		char_t* parse_doctype_ignore(char_t* s)
		{
			assert(s[0] == '<' && s[1] == '!' && s[2] == '[');
			s++;

			while (*s)
			{
				if (s[0] == '<' && s[1] == '!' && s[2] == '[')
				{
					// nested ignore section
					s = parse_doctype_ignore(s);
				}
				else if (s[0] == ']' && s[1] == ']' && s[2] == '>')
				{
					// ignore section end
					s += 3;

					return s;
				}
				else s++;
			}

			THROW_ERROR(status_bad_doctype, s);

			return s;
		}

		char_t* parse_doctype_group(char_t* s, char_t endch, bool toplevel)
		{
			assert(s[0] == '<' && s[1] == '!');
			s++;

			while (*s)
			{
				if (s[0] == '<' && s[1] == '!' && s[2] != '-')
				{
					if (s[2] == '[')
					{
						// ignore
						s = parse_doctype_ignore(s);
					}
					else
					{
						// some control group
						s = parse_doctype_group(s, endch, false);
					}
				}
				else if (s[0] == '<' || s[0] == '"' || s[0] == '\'')
				{
					// unknown tag (forbidden), or some primitive group
					s = parse_doctype_primitive(s);
				}
				else if (*s == '>')
				{
					s++;

					return s;
				}
				else s++;
			}

			if (!toplevel || endch != '>') THROW_ERROR(status_bad_doctype, s);

			return s;
		}

		char_t* parse_exclamation(char_t* s, node_struct* cursor, unsigned int optmsk, char_t endch)
		{
			// parse node contents, starting with exclamation mark
			++s;

			if (*s == '-') // '<!-...'
			{
				++s;

				if (*s == '-') // '<!--...'
				{
					++s;

					if (OPTSET(parse_comments))
					{
						PUSHNODE(node_comment); // Append a new node on the tree.
						cursor->value = s; // Save the offset.
					}

					if (OPTSET(parse_eol)
						&& OPTSET(parse_comments))
					{
						s = strconv_comment(s, endch);

						if (!s) THROW_ERROR(status_bad_comment, cursor->value);
					}
					else
					{
						// Scan for terminating '-->'.
						SCANFOR(s[0] == '-' && s[1] == '-' && ENDSWITH(s[2], '>'));
						CHECK_ERROR(status_bad_comment, s);

						if (OPTSET(parse_comments))
							*s = 0; // Zero-terminate this segment at the first terminating '-'.

						s += (s[2] == '>' ? 3 : 2); // Step over the '\0->'.
					}
				}
				else THROW_ERROR(status_bad_comment, s);
			}
			else if (*s == '[')
			{
				// '<![CDATA[...'
				if (*++s=='C' && *++s=='D' && *++s=='A' && *++s=='T' && *++s=='A' && *++s == '[')
				{
					++s;

					if (OPTSET(parse_cdata))
					{
						PUSHNODE(node_cdata); // Append a new node on the tree.
						cursor->value = s; // Save the offset.

						if (OPTSET(parse_eol))
						{
							s = strconv_cdata(s, endch);

							if (!s) THROW_ERROR(status_bad_cdata, cursor->value);
						}
						else
						{
							// Scan for terminating ']]>'.
							SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
							CHECK_ERROR(status_bad_cdata, s);

							*s++ = 0; // Zero-terminate this segment.
						}
					}
					else // Flagged for discard, but we still have to scan for the terminator.
					{
						// Scan for terminating ']]>'.
						SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
						CHECK_ERROR(status_bad_cdata, s);

						++s;
					}

					s += (s[1] == '>' ? 2 : 1); // Step over the last ']>'.
				}
				else THROW_ERROR(status_bad_cdata, s);
			}
			else if (s[0] == 'D' && s[1] == 'O' && s[2] == 'C' && s[3] == 'T' && s[4] == 'Y' && s[5] == 'P' && ENDSWITH(s[6], 'E'))
			{
				s -= 2;

				if (cursor->parent) THROW_ERROR(status_bad_doctype, s);

				char_t* mark = s + 9;

				s = parse_doctype_group(s, endch, true);

				if (OPTSET(parse_doctype))
				{
					while (IS_CHARTYPE(*mark, ct_space)) ++mark;

					PUSHNODE(node_doctype);

					cursor->value = mark;

					assert((s[0] == 0 && endch == '>') || s[-1] == '>');
					s[*s == 0 ? 0 : -1] = 0;

					POPNODE();
				}
			}
			else if (*s == 0 && endch == '-') THROW_ERROR(status_bad_comment, s);
			else if (*s == 0 && endch == '[') THROW_ERROR(status_bad_cdata, s);
			else THROW_ERROR(status_unrecognized_tag, s);

			return s;
		}

		char_t* parse_question(char_t* s, node_struct*& ref_cursor, unsigned int optmsk, char_t endch)
		{
			// load into registers
			node_struct* cursor = ref_cursor;
			char_t ch = 0;

			// parse node contents, starting with question mark
			++s;

			// read PI target
			char_t* target = s;

			if (!IS_CHARTYPE(*s, ct_start_symbol)) THROW_ERROR(status_bad_pi, s);

			SCANWHILE(IS_CHARTYPE(*s, ct_symbol));
			CHECK_ERROR(status_bad_pi, s);

			// determine node type; stricmp / strcasecmp is not portable
			bool declaration = (target[0] | ' ') == 'x' && (target[1] | ' ') == 'm' && (target[2] | ' ') == 'l' && target + 3 == s;

			if (declaration ? OPTSET(parse_declaration) : OPTSET(parse_pi))
			{
				if (declaration)
				{
					// disallow non top-level declarations
					if (cursor->parent) THROW_ERROR(status_bad_pi, s);

					PUSHNODE(node_declaration);
				}
				else
				{
					PUSHNODE(node_pi);
				}

				cursor->name = target;

				ENDSEG();

				// parse value/attributes
				if (ch == '?')
				{
					// empty node
					if (!ENDSWITH(*s, '>')) THROW_ERROR(status_bad_pi, s);
					s += (*s == '>');

					if(cursor->parent)
					{
						POPNODE(); // Pop.
					}
					else
					{
						// We have reached the root node so do not
						// attempt to pop anymore nodes
						return s;
					}
				}
				else if (IS_CHARTYPE(ch, ct_space))
				{
					SKIPWS();

					// scan for tag end
					char_t* value = s;

					SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
					CHECK_ERROR(status_bad_pi, s);

					if (declaration)
					{
						// replace ending ? with / so that 'element' terminates properly
						*s = '/';

						// we exit from this function with cursor at node_declaration, which is a signal to parse() to go to LOC_ATTRIBUTES
						s = value;
					}
					else
					{
						// store value and step over >
						cursor->value = value;

						if(cursor->parent)
						{
							POPNODE(); // Pop.
						}
						else
						{
							// We have reached the root node so do not
							// attempt to pop anymore nodes
							return s;
						}
						ENDSEG();

						s += (*s == '>');
					}
				}
				else THROW_ERROR(status_bad_pi, s);
			}
			else
			{
				// scan for tag end
				SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
				CHECK_ERROR(status_bad_pi, s);

				s += (s[1] == '>' ? 2 : 1);
			}

			// store from registers
			ref_cursor = cursor;

			return s;
		}

		void parse(char_t* s, node_struct* htmldoc, unsigned int optmsk, char_t endch)
		{
			strconv_attribute_t strconv_attribute = get_strconv_attribute(optmsk);
			strconv_pcdata_t strconv_pcdata = get_strconv_pcdata(optmsk);

			char_t ch = 0;

			// Point the cursor to the html document node
			node_struct* cursor = htmldoc;

			// Set the marker
			char_t* mark = s;

			// It's necessary to keep another mark when we have to roll
			// back the name and the mark at the same time.
			char_t* sMark = s;
			char_t* nameMark = s;

			// Parse while the current character is not '\0'
			while (*s != 0)
			{
				// Check if the current character is the start tag character
				if (*s == '<')
				{
					// Move to the next character
					++s;

					// Check if the current character is a start symbol
				LOC_TAG:
					if (IS_CHARTYPE(*s, ct_start_symbol)) // '<#...'
					{
						// Append a new node to the tree.
						PUSHNODE(node_element);

						// Set the current element's name
						cursor->name = s;

						// Scan while the current character is a symbol belonging
						// to the set of symbols acceptable within a tag. In other
						// words, scan until the termination symbol is discovered.
						SCANWHILE(IS_CHARTYPE(*s, ct_symbol));

						// Save char in 'ch', terminate & step over.
						ENDSEG();

						// Capitalize the tag name
						to_upper(cursor->name);

						if (ch == '>')
						{
							// end of tag
						}
						else if (IS_CHARTYPE(ch, ct_space))
						{
						LOC_ATTRIBUTES:
							while (true)
							{
								SKIPWS(); // Eat any whitespace.

								if (IS_CHARTYPE(*s, ct_start_symbol)) // <... #...
								{
									attribute_struct* a = append_attribute_ll(cursor, alloc); // Make space for this attribute.
									if (!a) THROW_ERROR(status_out_of_memory, s);

									a->name = s; // Save the offset.

									SCANWHILE(IS_CHARTYPE(*s, ct_symbol)); // Scan for a terminator.
									CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

									ENDSEG(); // Save char in 'ch', terminate & step over.

									// Capitalize the attribute name
									to_upper(a->name);

									//$ redundant, left for performance
									CHECK_ERROR(status_bad_attribute, s);

									if (IS_CHARTYPE(ch, ct_space))
									{
										SKIPWS(); // Eat any whitespace.
										CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

										ch = *s;
										++s;
									}

									if (ch == '=') // '<... #=...'
									{
										SKIPWS(); // Eat any whitespace.

										if (*s == '"' || *s == '\'') // '<... #="...'
										{
											ch = *s; // Save quote char to avoid breaking on "''" -or- '""'.
											++s; // Step over the quote.
											a->value = s; // Save the offset.

											s = strconv_attribute(s, ch);

											if (!s) THROW_ERROR(status_bad_attribute, a->value);

											// After this line the loop continues from the start;
											// Whitespaces, / and > are ok, symbols and EOF are wrong,
											// everything else will be detected
											if (IS_CHARTYPE(*s, ct_start_symbol)) THROW_ERROR(status_bad_attribute, s);
										}
										else THROW_ERROR(status_bad_attribute, s);
									}
									else {//hot fix:try to fix that attribute=value situation;
										ch='"';
										a->value=s;
										char_t* dp=s;
										char_t temp;
										//find the end of attribute,200 length is enough,it must return a position;
										temp=*dp;//backup the former char
										*dp='"';//hack at the end position;
										s=strconv_attribute(s,ch);
										*dp=temp;//restore it;
										s--;// back it because we come to the " but it is > indeed;
										if (IS_CHARTYPE(*s, ct_start_symbol)) THROW_ERROR(status_bad_attribute, s);

									}
									//THROW_ERROR(status_bad_attribute, s);
								}
								else if (*s == '/')
								{
									++s;

									if (*s == '>')
									{
										if(cursor->parent)
										{
											POPNODE(); // Pop.
										}
										else
										{
											// We have reached the root node so do not
											// attempt to pop anymore nodes
											break;
										}
										s++;
										break;
									}
									else if (*s == 0 && endch == '>')
									{
										if(cursor->parent)
										{
											POPNODE(); // Pop.
										}
										else
										{
											// We have reached the root node so do not
											// attempt to pop anymore nodes
											break;
										}
										break;
									}
									else THROW_ERROR(status_bad_start_element, s);
								}
								else if (*s == '>')
								{
									++s;

									break;
								}
								else if (*s == 0 && endch == '>')
								{
									break;
								}
								else THROW_ERROR(status_bad_start_element, s);
							}// while

							// !!!
						}
						else if (ch == '/') // '<#.../'
						{
							if (!ENDSWITH(*s, '>')) THROW_ERROR(status_bad_start_element, s);

							if(cursor->parent)
							{
								POPNODE(); // Pop.
							}
							else
							{
								// We have reached the root node so do not
								// attempt to pop anymore nodes
								break;
							}

							s += (*s == '>');
						}
						else if (ch == 0)
						{
							// we stepped over null terminator, backtrack & handle closing tag
							--s;

							if (endch != '>') THROW_ERROR(status_bad_start_element, s);
						}
						else THROW_ERROR(status_bad_start_element, s);
					}
					else if (*s == '/')
					{
						++s;

						char_t* name = cursor->name;
						if (!name)
						{
							// TODO ignore exception
							//THROW_ERROR(status_end_element_mismatch, s);
						}
						else
						{
							sMark = s;
							nameMark = name;
							// Read the name while the character is a symbol
							while (IS_CHARTYPE(*s, ct_symbol))
							{
								// Check if we're closing the correct tag name:
								// if the cursor tag does not match the current
								// closing tag then throw an exception.
								if (*s++ != *name++)
								{
									// TODO POPNODE or ignore exception
									//THROW_ERROR(status_end_element_mismatch, s);

									// Return to the last position where we started
									// reading the expected closing tag name.
									s = sMark;
									name = nameMark;
									break;
								}
							}
							// Check if the end element is valid
							if (*name)
							{
								if (*s == 0 && name[0] == endch && name[1] == 0)
								{
									THROW_ERROR(status_bad_end_element, s);
								}
								else
								{
									// TODO POPNODE or ignore exception
									//THROW_ERROR(status_end_element_mismatch, s);
								}
							}
						}

						// The tag was closed so we have to pop the
						// node off the "stack".
						if(cursor->parent)
						{
							POPNODE(); // Pop.
						}
						else
						{
							// We have reached the root node so do not
							// attempt to pop anymore nodes
							break;
						}

						SKIPWS();

						// If there end of the string is reached.
						if (*s == 0)
						{
							// Check if the end character specified is the
							// same as the closing tag.
							if (endch != '>')
							{
								THROW_ERROR(status_bad_end_element, s);
							}
						}
						else
						{
							// Skip the end character becaue the tag
							// was closed and the node was popped off
							// the "stack".
							if (*s != '>')
							{
								// Continue parsing
								continue;
								// TODO ignore the exception
								// THROW_ERROR(status_bad_end_element, s);
							}
							++s;
						}
					}
					else if (*s == '?') // '<?...'
					{
						s = parse_question(s, cursor, optmsk, endch);

						assert(cursor);
						if ((cursor->header & html_memory_page_type_mask) + 1 == node_declaration)
						{
							goto LOC_ATTRIBUTES;
						}
					}
					else if (*s == '!') // '<!...'
					{
						s = parse_exclamation(s, cursor, optmsk, endch);
					}
					else if (*s == 0 && endch == '?')
					{
						THROW_ERROR(status_bad_pi, s);
					}
					else
					{
						THROW_ERROR(status_unrecognized_tag, s);
					}
				}
				else
				{
					mark = s; // Save this offset while searching for a terminator.

					SKIPWS(); // Eat whitespace if no genuine PCDATA here.

					if ((!OPTSET(parse_ws_pcdata) || mark == s) && (*s == '<' || !*s))
					{
						continue;
					}

					s = mark;

					if (cursor->parent)
					{
						PUSHNODE(node_pcdata); // Append a new node on the tree.
						cursor->value = s; // Save the offset.

						s = strconv_pcdata(s);

						POPNODE(); // Pop since this is a standalone.

						if (!*s) break;
					}
					else
					{
						SCANFOR(*s == '<'); // '...<'
						if (!*s) break;

						++s;
					}

					// We're after '<'
					goto LOC_TAG;
				}
			}

			// Check that last tag is closed
			if (cursor != htmldoc)
			{
				// TODO POPNODE or ignore exception
				// THROW_ERROR(status_end_element_mismatch, s);
				cursor = htmldoc;
			}
		}

		static html_parse_result parse(char_t* buffer, size_t length, node_struct* root, unsigned int optmsk)
		{
			document_struct* htmldoc = static_cast<document_struct*>(root);

			// store buffer for offset_debug
			htmldoc->buffer = buffer;

			// early-out for empty documents
			if (length == 0) return make_parse_result(status_ok);

			// create parser on stack
			parser parser(*htmldoc);

			// save last character and make buffer zero-terminated (speeds up parsing)
			char_t endch = buffer[length - 1];
			buffer[length - 1] = 0;

			// perform actual parsing
			int error = setjmp(parser.error_handler);

			if (error == 0)
			{
				parser.parse(buffer, htmldoc, optmsk, endch);
			}

			html_parse_result result = make_parse_result(static_cast<html_parse_status>(error), parser.error_offset ? parser.error_offset - buffer : 0);
			assert(result.offset >= 0 && static_cast<size_t>(result.offset) <= length);

			// update allocator state
			*static_cast<html_allocator*>(htmldoc) = parser.alloc;

			// since we removed last character, we have to handle the only possible false positive
			if (result && endch == '<')
			{
				// there's no possible well-formed document with < at the end
				return make_parse_result(status_unrecognized_tag, length);
			}

			return result;
		}
	};
}

namespace pugihtml
{
	// TODO(povilas): move to attribute.
	attribute_iterator::attribute_iterator()
	{
	}

	attribute_iterator::attribute_iterator(const attribute& attr, const node& parent): _wrap(attr), _parent(parent)
	{
	}

	attribute_iterator::attribute_iterator(attribute_struct* ref, node_struct* parent): _wrap(ref), _parent(parent)
	{
	}

	bool attribute_iterator::operator==(const attribute_iterator& rhs) const
	{
		return _wrap._attr == rhs._wrap._attr && _parent._root == rhs._parent._root;
	}

	bool attribute_iterator::operator!=(const attribute_iterator& rhs) const
	{
		return _wrap._attr != rhs._wrap._attr || _parent._root != rhs._parent._root;
	}

	attribute& attribute_iterator::operator*()
	{
		assert(_wrap._attr);
		return _wrap;
	}

	attribute* attribute_iterator::operator->()
	{
		assert(_wrap._attr);
		return &_wrap;
	}

	const attribute_iterator& attribute_iterator::operator++()
	{
		assert(_wrap._attr);
		_wrap._attr = _wrap._attr->next_attribute;
		return *this;
	}

	attribute_iterator attribute_iterator::operator++(int)
	{
		attribute_iterator temp = *this;
		++*this;
		return temp;
	}

	const attribute_iterator& attribute_iterator::operator--()
	{
		_wrap = _wrap._attr ? _wrap.previous_attribute() : _parent.last_attribute();
		return *this;
	}

	attribute_iterator attribute_iterator::operator--(int)
	{
		attribute_iterator temp = *this;
		--*this;
		return temp;
	}

#ifndef PUGIHTML_NO_STL

	std::wstring PUGIHTML_FUNCTION as_wide(const char* str)
	{
		assert(str);

		return as_wide_impl(str, strlen(str));
	}

	std::wstring PUGIHTML_FUNCTION as_wide(const std::string& str)
	{
		return as_wide_impl(str.c_str(), str.size());
	}
#endif

	void PUGIHTML_FUNCTION set_memory_management_functions(allocation_function allocate, deallocation_function deallocate)
	{
		global_allocate = allocate;
		global_deallocate = deallocate;
	}

	allocation_function PUGIHTML_FUNCTION get_memory_allocation_function()
	{
		return global_allocate;
	}

	deallocation_function PUGIHTML_FUNCTION get_memory_deallocation_function()
	{
		return global_deallocate;
	}
}

#if !defined(PUGIHTML_NO_STL) && (defined(_MSC_VER) || defined(__ICC))
namespace std
{
	// Workarounds for (non-standard) iterator category detection for older versions (MSVC7/IC8 and earlier)
	std::bidirectional_iterator_tag _Iter_cat(const node_iterator&)
	{
		return std::bidirectional_iterator_tag();
	}

	std::bidirectional_iterator_tag _Iter_cat(const attribute_iterator&)
	{
		return std::bidirectional_iterator_tag();
	}
}
#endif

#if !defined(PUGIHTML_NO_STL) && defined(__SUNPRO_CC)
namespace std
{
	// Workarounds for (non-standard) iterator category detection
	std::bidirectional_iterator_tag __iterator_category(const node_iterator&)
	{
		return std::bidirectional_iterator_tag();
	}

	std::bidirectional_iterator_tag __iterator_category(const attribute_iterator&)
	{
		return std::bidirectional_iterator_tag();
	}
}
#endif

#ifndef PUGIHTML_NO_XPATH

// STL replacements
namespace
{
	struct equal_to
	{
		template <typename T> bool operator()(const T& lhs, const T& rhs) const
		{
			return lhs == rhs;
		}
	};

	struct not_equal_to
	{
		template <typename T> bool operator()(const T& lhs, const T& rhs) const
		{
			return lhs != rhs;
		}
	};

	struct less
	{
		template <typename T> bool operator()(const T& lhs, const T& rhs) const
		{
			return lhs < rhs;
		}
	};

	struct less_equal
	{
		template <typename T> bool operator()(const T& lhs, const T& rhs) const
		{
			return lhs <= rhs;
		}
	};

	template <typename T> void swap(T& lhs, T& rhs)
	{
		T temp = lhs;
		lhs = rhs;
		rhs = temp;
	}

	template <typename I, typename Pred> I min_element(I begin, I end, const Pred& pred)
	{
		I result = begin;

		for (I it = begin + 1; it != end; ++it)
			if (pred(*it, *result))
				result = it;

		return result;
	}

	template <typename I> void reverse(I begin, I end)
	{
		while (begin + 1 < end) swap(*begin++, *--end);
	}

	template <typename I> I unique(I begin, I end)
	{
		// fast skip head
		while (begin + 1 < end && *begin != *(begin + 1)) begin++;

		if (begin == end) return begin;

		// last written element
		I write = begin++;

		// merge unique elements
		while (begin != end)
		{
			if (*begin != *write)
				*++write = *begin++;
			else
				begin++;
		}

		// past-the-end (write points to live element)
		return write + 1;
	}

	template <typename I> void copy_backwards(I begin, I end, I target)
	{
		while (begin != end) *--target = *--end;
	}

	template <typename I, typename Pred, typename T> void insertion_sort(I begin, I end, const Pred& pred, T*)
	{
		assert(begin != end);

		for (I it = begin + 1; it != end; ++it)
		{
			T val = *it;

			if (pred(val, *begin))
			{
				// move to front
				copy_backwards(begin, it, it + 1);
				*begin = val;
			}
			else
			{
				I hole = it;

				// move hole backwards
				while (pred(val, *(hole - 1)))
				{
					*hole = *(hole - 1);
					hole--;
				}

				// fill hole with element
				*hole = val;
			}
		}
	}

	// std variant for elements with ==
	template <typename I, typename Pred> void partition(I begin, I middle, I end, const Pred& pred, I* out_eqbeg, I* out_eqend)
	{
		I eqbeg = middle, eqend = middle + 1;

		// expand equal range
		while (eqbeg != begin && *(eqbeg - 1) == *eqbeg) --eqbeg;
		while (eqend != end && *eqend == *eqbeg) ++eqend;

		// process outer elements
		I ltend = eqbeg, gtbeg = eqend;

		for (;;)
		{
			// find the element from the right side that belongs to the left one
			for (; gtbeg != end; ++gtbeg)
				if (!pred(*eqbeg, *gtbeg))
				{
					if (*gtbeg == *eqbeg) swap(*gtbeg, *eqend++);
					else break;
				}

			// find the element from the left side that belongs to the right one
			for (; ltend != begin; --ltend)
				if (!pred(*(ltend - 1), *eqbeg))
				{
					if (*eqbeg == *(ltend - 1)) swap(*(ltend - 1), *--eqbeg);
					else break;
				}

			// scanned all elements
			if (gtbeg == end && ltend == begin)
			{
				*out_eqbeg = eqbeg;
				*out_eqend = eqend;
				return;
			}

			// make room for elements by moving equal area
			if (gtbeg == end)
			{
				if (--ltend != --eqbeg) swap(*ltend, *eqbeg);
				swap(*eqbeg, *--eqend);
			}
			else if (ltend == begin)
			{
				if (eqend != gtbeg) swap(*eqbeg, *eqend);
				++eqend;
				swap(*gtbeg++, *eqbeg++);
			}
			else swap(*gtbeg++, *--ltend);
		}
	}

	template <typename I, typename Pred> void median3(I first, I middle, I last, const Pred& pred)
	{
		if (pred(*middle, *first)) swap(*middle, *first);
		if (pred(*last, *middle)) swap(*last, *middle);
		if (pred(*middle, *first)) swap(*middle, *first);
	}

	template <typename I, typename Pred> void median(I first, I middle, I last, const Pred& pred)
	{
		if (last - first <= 40)
		{
			// median of three for small chunks
			median3(first, middle, last, pred);
		}
		else
		{
			// median of nine
			size_t step = (last - first + 1) / 8;

			median3(first, first + step, first + 2 * step, pred);
			median3(middle - step, middle, middle + step, pred);
			median3(last - 2 * step, last - step, last, pred);
			median3(first + step, middle, last - step, pred);
		}
	}

	template <typename I, typename Pred> void sort(I begin, I end, const Pred& pred)
	{
		// sort large chunks
		while (end - begin > 32)
		{
			// find median element
			I middle = begin + (end - begin) / 2;
			median(begin, middle, end - 1, pred);

			// partition in three chunks (< = >)
			I eqbeg, eqend;
			partition(begin, middle, end, pred, &eqbeg, &eqend);

			// loop on larger half
			if (eqbeg - begin > end - eqend)
			{
				sort(eqend, end, pred);
				end = eqbeg;
			}
			else
			{
				sort(begin, eqbeg, pred);
				begin = eqend;
			}
		}

		// insertion sort small chunk
		if (begin != end) insertion_sort(begin, end, pred, &*begin);
	}
}

// Allocator used for AST and evaluation stacks
namespace
{
	struct xpath_memory_block
	{
		xpath_memory_block* next;

		char data[4096];
	};

	class xpath_allocator
	{
		xpath_memory_block* _root;
		size_t _root_size;

	public:
	#ifdef PUGIHTML_NO_EXCEPTIONS
		jmp_buf* error_handler;
	#endif

		xpath_allocator(xpath_memory_block* root, size_t root_size = 0): _root(root), _root_size(root_size)
		{
		#ifdef PUGIHTML_NO_EXCEPTIONS
			error_handler = 0;
		#endif
		}

		void* allocate_nothrow(size_t size)
		{
			const size_t block_capacity = sizeof(_root->data);

			// align size so that we're able to store pointers in subsequent blocks
			size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

			if (_root_size + size <= block_capacity)
			{
				void* buf = _root->data + _root_size;
				_root_size += size;
				return buf;
			}
			else
			{
				size_t block_data_size = (size > block_capacity) ? size : block_capacity;
				size_t block_size = block_data_size + offsetof(xpath_memory_block, data);

				xpath_memory_block* block = static_cast<xpath_memory_block*>(global_allocate(block_size));
				if (!block) return 0;

				block->next = _root;

				_root = block;
				_root_size = size;

				return block->data;
			}
		}

		void* allocate(size_t size)
		{
			void* result = allocate_nothrow(size);

			if (!result)
			{
			#ifdef PUGIHTML_NO_EXCEPTIONS
				assert(error_handler);
				longjmp(*error_handler, 1);
			#else
				throw std::bad_alloc();
			#endif
			}

			return result;
		}

		void* reallocate(void* ptr, size_t old_size, size_t new_size)
		{
			// align size so that we're able to store pointers in subsequent blocks
			old_size = (old_size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
			new_size = (new_size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

			// we can only reallocate the last object
			assert(ptr == 0 || static_cast<char*>(ptr) + old_size == _root->data + _root_size);

			// adjust root size so that we have not allocated the object at all
			bool only_object = (_root_size == old_size);

			if (ptr) _root_size -= old_size;

			// allocate a new version (this will obviously reuse the memory if possible)
			void* result = allocate(new_size);
			assert(result);

			// we have a new block
			if (result != ptr && ptr)
			{
				// copy old data
				assert(new_size > old_size);
				memcpy(result, ptr, old_size);

				// free the previous page if it had no other objects
				if (only_object)
				{
					assert(_root->data == result);
					assert(_root->next);

					xpath_memory_block* next = _root->next->next;

					if (next)
					{
						// deallocate the whole page, unless it was the first one
						global_deallocate(_root->next);
						_root->next = next;
					}
				}
			}

			return result;
		}

		void revert(const xpath_allocator& state)
		{
			// free all new pages
			xpath_memory_block* cur = _root;

			while (cur != state._root)
			{
				xpath_memory_block* next = cur->next;

				global_deallocate(cur);

				cur = next;
			}

			// restore state
			_root = state._root;
			_root_size = state._root_size;
		}

		void release()
		{
			xpath_memory_block* cur = _root;
			assert(cur);

			while (cur->next)
			{
				xpath_memory_block* next = cur->next;

				global_deallocate(cur);

				cur = next;
			}
		}
	};

	struct xpath_allocator_capture
	{
		xpath_allocator_capture(xpath_allocator* alloc): _target(alloc), _state(*alloc)
		{
		}

		~xpath_allocator_capture()
		{
			_target->revert(_state);
		}

		xpath_allocator* _target;
		xpath_allocator _state;
	};

	struct xpath_stack
	{
		xpath_allocator* result;
		xpath_allocator* temp;
	};

	struct xpath_stack_data
	{
		xpath_memory_block blocks[2];
		xpath_allocator result;
		xpath_allocator temp;
		xpath_stack stack;

	#ifdef PUGIHTML_NO_EXCEPTIONS
		jmp_buf error_handler;
	#endif

		xpath_stack_data(): result(blocks + 0), temp(blocks + 1)
		{
			blocks[0].next = blocks[1].next = 0;

			stack.result = &result;
			stack.temp = &temp;

		#ifdef PUGIHTML_NO_EXCEPTIONS
			result.error_handler = temp.error_handler = &error_handler;
		#endif
		}

		~xpath_stack_data()
		{
			result.release();
			temp.release();
		}
	};
}

// String class
namespace
{
	class xpath_string
	{
		const char_t* _buffer;
		bool _uses_heap;

		static char_t* duplicate_string(const char_t* string, size_t length, xpath_allocator* alloc)
		{
			char_t* result = static_cast<char_t*>(alloc->allocate((length + 1) * sizeof(char_t)));
			assert(result);

			memcpy(result, string, length * sizeof(char_t));
			result[length] = 0;

			return result;
		}

		static char_t* duplicate_string(const char_t* string, xpath_allocator* alloc)
		{
			return duplicate_string(string, strlength(string), alloc);
		}

	public:
		xpath_string(): _buffer(PUGIHTML_TEXT("")), _uses_heap(false)
		{
		}

		explicit xpath_string(const char_t* str, xpath_allocator* alloc)
		{
			bool empty = (*str == 0);

			_buffer = empty ? PUGIHTML_TEXT("") : duplicate_string(str, alloc);
			_uses_heap = !empty;
		}

		explicit xpath_string(const char_t* str, bool use_heap): _buffer(str), _uses_heap(use_heap)
		{
		}

		xpath_string(const char_t* begin, const char_t* end, xpath_allocator* alloc)
		{
			assert(begin <= end);

			bool empty = (begin == end);

			_buffer = empty ? PUGIHTML_TEXT("") : duplicate_string(begin, static_cast<size_t>(end - begin), alloc);
			_uses_heap = !empty;
		}

		void append(const xpath_string& o, xpath_allocator* alloc)
		{
			// skip empty sources
			if (!*o._buffer) return;

			// fast append for constant empty target and constant source
			if (!*_buffer && !_uses_heap && !o._uses_heap)
			{
				_buffer = o._buffer;
			}
			else
			{
				// need to make heap copy
				size_t target_length = strlength(_buffer);
				size_t source_length = strlength(o._buffer);
				size_t length = target_length + source_length;

				// allocate new buffer
				char_t* result = static_cast<char_t*>(alloc->reallocate(_uses_heap ? const_cast<char_t*>(_buffer) : 0, (target_length + 1) * sizeof(char_t), (length + 1) * sizeof(char_t)));
				assert(result);

				// append first string to the new buffer in case there was no reallocation
				if (!_uses_heap) memcpy(result, _buffer, target_length * sizeof(char_t));

				// append second string to the new buffer
				memcpy(result + target_length, o._buffer, source_length * sizeof(char_t));
				result[length] = 0;

				// finalize
				_buffer = result;
				_uses_heap = true;
			}
		}

		const char_t* c_str() const
		{
			return _buffer;
		}

		size_t length() const
		{
			return strlength(_buffer);
		}

		char_t* data(xpath_allocator* alloc)
		{
			// make private heap copy
			if (!_uses_heap)
			{
				_buffer = duplicate_string(_buffer, alloc);
				_uses_heap = true;
			}

			return const_cast<char_t*>(_buffer);
		}

		bool empty() const
		{
			return *_buffer == 0;
		}

		bool operator==(const xpath_string& o) const
		{
			return strequal(_buffer, o._buffer);
		}

		bool operator!=(const xpath_string& o) const
		{
			return !strequal(_buffer, o._buffer);
		}

		bool uses_heap() const
		{
			return _uses_heap;
		}
	};

	xpath_string xpath_string_const(const char_t* str)
	{
		return xpath_string(str, false);
	}
}

namespace
{
	bool starts_with(const char_t* string, const char_t* pattern)
	{
		while (*pattern && *string == *pattern)
		{
			string++;
			pattern++;
		}

		return *pattern == 0;
	}

	const char_t* find_char(const char_t* s, char_t c)
	{
	#ifdef PUGIHTML_WCHAR_MODE
		return wcschr(s, c);
	#else
		return strchr(s, c);
	#endif
	}

	const char_t* find_substring(const char_t* s, const char_t* p)
	{
	#ifdef PUGIHTML_WCHAR_MODE
		// MSVC6 wcsstr bug workaround (if s is empty it always returns 0)
		return (*p == 0) ? s : wcsstr(s, p);
	#else
		return strstr(s, p);
	#endif
	}

	// Converts symbol to lower case, if it is an ASCII one
	char_t tolower_ascii(char_t ch)
	{
		return static_cast<unsigned int>(ch - 'A') < 26 ? static_cast<char_t>(ch | ' ') : ch;
	}

	xpath_string string_value(const xpath_node& na, xpath_allocator* alloc)
	{
		if (na.attribute())
			return xpath_string_const(na.attribute().value());
		else
		{
			const node& n = na.node();

			switch (n.type())
			{
			case node_pcdata:
			case node_cdata:
			case node_comment:
			case node_pi:
				return xpath_string_const(n.value());

			case node_document:
			case node_element:
			{
				xpath_string result;

				node cur = n.first_child();

				while (cur && cur != n)
				{
					if (cur.type() == node_pcdata || cur.type() == node_cdata)
						result.append(xpath_string_const(cur.value()), alloc);

					if (cur.first_child())
						cur = cur.first_child();
					else if (cur.next_sibling())
						cur = cur.next_sibling();
					else
					{
						while (!cur.next_sibling() && cur != n)
							cur = cur.parent();

						if (cur != n) cur = cur.next_sibling();
					}
				}

				return result;
			}

			default:
				return xpath_string();
			}
		}
	}

	unsigned int node_height(node n)
	{
		unsigned int result = 0;

		while (n)
		{
			++result;
			n = n.parent();
		}

		return result;
	}

	bool node_is_before(node ln, unsigned int lh, node rn, unsigned int rh)
	{
		// normalize heights
		for (unsigned int i = rh; i < lh; i++) ln = ln.parent();
		for (unsigned int j = lh; j < rh; j++) rn = rn.parent();

		// one node is the ancestor of the other
		if (ln == rn) return lh < rh;

		// find common ancestor
		while (ln.parent() != rn.parent())
		{
			ln = ln.parent();
			rn = rn.parent();
		}

		// there is no common ancestor (the shared parent is null), nodes are from different documents
		if (!ln.parent()) return ln < rn;

		// determine sibling order
		for (; ln; ln = ln.next_sibling())
			if (ln == rn)
				return true;

		return false;
	}

	bool node_is_ancestor(node parent, node node)
	{
		while (node && node != parent) node = node.parent();

		return parent && node == parent;
	}

	const void* document_order(const xpath_node& xnode)
	{
		node_struct* node = xnode.node().internal_object();

		if (node)
		{
			if (node->name && (node->header & html_memory_page_name_allocated_mask) == 0) return node->name;
			if (node->value && (node->header & html_memory_page_value_allocated_mask) == 0) return node->value;
			return 0;
		}

		attribute_struct* attr = xnode.attribute().internal_object();

		if (attr)
		{
			if ((attr->header & html_memory_page_name_allocated_mask) == 0) return attr->name;
			if ((attr->header & html_memory_page_value_allocated_mask) == 0) return attr->value;
			return 0;
		}

		return 0;
	}

	struct document_order_comparator
	{
		bool operator()(const xpath_node& lhs, const xpath_node& rhs) const
		{
			// optimized document order based check
			const void* lo = document_order(lhs);
			const void* ro = document_order(rhs);

			if (lo && ro) return lo < ro;

			// slow comparison
			node ln = lhs.node(), rn = rhs.node();

			// compare attributes
			if (lhs.attribute() && rhs.attribute())
			{
				// shared parent
				if (lhs.parent() == rhs.parent())
				{
					// determine sibling order
					for (attribute a = lhs.attribute(); a; a = a.next_attribute())
						if (a == rhs.attribute())
							return true;

					return false;
				}

				// compare attribute parents
				ln = lhs.parent();
				rn = rhs.parent();
			}
			else if (lhs.attribute())
			{
				// attributes go after the parent element
				if (lhs.parent() == rhs.node()) return false;

				ln = lhs.parent();
			}
			else if (rhs.attribute())
			{
				// attributes go after the parent element
				if (rhs.parent() == lhs.node()) return true;

				rn = rhs.parent();
			}

			if (ln == rn) return false;

			unsigned int lh = node_height(ln);
			unsigned int rh = node_height(rn);

			return node_is_before(ln, lh, rn, rh);
		}
	};

	struct duplicate_comparator
	{
		bool operator()(const xpath_node& lhs, const xpath_node& rhs) const
		{
			if (lhs.attribute()) return rhs.attribute() ? lhs.attribute() < rhs.attribute() : true;
			else return rhs.attribute() ? false : lhs.node() < rhs.node();
		}
	};

	double gen_nan()
	{
	#if defined(__STDC_IEC_559__) || ((FLT_RADIX - 0 == 2) && (FLT_MAX_EXP - 0 == 128) && (FLT_MANT_DIG - 0 == 24))
		union { float f; int32_t i; } u[sizeof(float) == sizeof(int32_t) ? 1 : -1];
		u[0].i = 0x7fc00000;
		return u[0].f;
	#else
		// fallback
		const volatile double zero = 0.0;
		return zero / zero;
	#endif
	}

	bool is_nan(double value)
	{
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		return !!_isnan(value);
	#elif defined(fpclassify) && defined(FP_NAN)
		return fpclassify(value) == FP_NAN;
	#else
		// fallback
		const volatile double v = value;
		return v != v;
	#endif
	}

	const char_t* convert_number_to_string_special(double value)
	{
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		if (_finite(value)) return (value == 0) ? PUGIHTML_TEXT("0") : 0;
		if (_isnan(value)) return PUGIHTML_TEXT("NaN");
		return PUGIHTML_TEXT("-Infinity") + (value > 0);
	#elif defined(fpclassify) && defined(FP_NAN) && defined(FP_INFINITE) && defined(FP_ZERO)
		switch (fpclassify(value))
		{
		case FP_NAN:
			return PUGIHTML_TEXT("NaN");

		case FP_INFINITE:
			return PUGIHTML_TEXT("-Infinity") + (value > 0);

		case FP_ZERO:
			return PUGIHTML_TEXT("0");

		default:
			return 0;
		}
	#else
		// fallback
		const volatile double v = value;

		if (v == 0) return PUGIHTML_TEXT("0");
		if (v != v) return PUGIHTML_TEXT("NaN");
		if (v * 2 == v) return PUGIHTML_TEXT("-Infinity") + (value > 0);
		return 0;
	#endif
	}

	bool convert_number_to_boolean(double value)
	{
		return (value != 0 && !is_nan(value));
	}

	void truncate_zeros(char* begin, char* end)
	{
		while (begin != end && end[-1] == '0') end--;

		*end = 0;
	}

	// gets mantissa digits in the form of 0.xxxxx with 0. implied and the exponent
#if defined(_MSC_VER) && _MSC_VER >= 1400
	void convert_number_to_mantissa_exponent(double value, char* buffer, size_t buffer_size, char** out_mantissa, int* out_exponent)
	{
		// get base values
		int sign, exponent;
		_ecvt_s(buffer, buffer_size, value, DBL_DIG + 1, &exponent, &sign);

		// truncate redundant zeros
		truncate_zeros(buffer, buffer + strlen(buffer));

		// fill results
		*out_mantissa = buffer;
		*out_exponent = exponent;
	}
#else
	void convert_number_to_mantissa_exponent(double value, char* buffer, size_t buffer_size, char** out_mantissa, int* out_exponent)
	{
		// get a scientific notation value with IEEE DBL_DIG decimals
		sprintf(buffer, "%.*e", DBL_DIG, value);
		assert(strlen(buffer) < buffer_size);
		(void)!buffer_size;

		// get the exponent (possibly negative)
		char* exponent_string = strchr(buffer, 'e');
		assert(exponent_string);

		int exponent = atoi(exponent_string + 1);

		// extract mantissa string: skip sign
		char* mantissa = buffer[0] == '-' ? buffer + 1 : buffer;
		assert(mantissa[0] != '0' && mantissa[1] == '.');

		// divide mantissa by 10 to eliminate integer part
		mantissa[1] = mantissa[0];
		mantissa++;
		exponent++;

		// remove extra mantissa digits and zero-terminate mantissa
		truncate_zeros(mantissa, exponent_string);

		// fill results
		*out_mantissa = mantissa;
		*out_exponent = exponent;
	}
#endif

	xpath_string convert_number_to_string(double value, xpath_allocator* alloc)
	{
		// try special number conversion
		const char_t* special = convert_number_to_string_special(value);
		if (special) return xpath_string_const(special);

		// get mantissa + exponent form
		char mantissa_buffer[64];

		char* mantissa;
		int exponent;
		convert_number_to_mantissa_exponent(value, mantissa_buffer, sizeof(mantissa_buffer), &mantissa, &exponent);

		// make the number!
		char_t result[512];
		char_t* s = result;

		// sign
		if (value < 0) *s++ = '-';

		// integer part
		if (exponent <= 0)
		{
			*s++ = '0';
		}
		else
		{
			while (exponent > 0)
			{
				assert(*mantissa == 0 || (unsigned)(*mantissa - '0') <= 9);
				*s++ = *mantissa ? *mantissa++ : '0';
				exponent--;
			}
		}

		// fractional part
		if (*mantissa)
		{
			// decimal point
			*s++ = '.';

			// extra zeroes from negative exponent
			while (exponent < 0)
			{
				*s++ = '0';
				exponent++;
			}

			// extra mantissa digits
			while (*mantissa)
			{
				assert((unsigned)(*mantissa - '0') <= 9);
				*s++ = *mantissa++;
			}
		}

		// zero-terminate
		assert(s < result + sizeof(result) / sizeof(result[0]));
		*s = 0;

		return xpath_string(result, alloc);
	}

	bool check_string_to_number_format(const char_t* string)
	{
		// parse leading whitespace
		while (IS_CHARTYPE(*string, ct_space)) ++string;

		// parse sign
		if (*string == '-') ++string;

		if (!*string) return false;

		// if there is no integer part, there should be a decimal part with at least one digit
		if (!IS_CHARTYPEX(string[0], ctx_digit) && (string[0] != '.' || !IS_CHARTYPEX(string[1], ctx_digit))) return false;

		// parse integer part
		while (IS_CHARTYPEX(*string, ctx_digit)) ++string;

		// parse decimal part
		if (*string == '.')
		{
			++string;

			while (IS_CHARTYPEX(*string, ctx_digit)) ++string;
		}

		// parse trailing whitespace
		while (IS_CHARTYPE(*string, ct_space)) ++string;

		return *string == 0;
	}

	double convert_string_to_number(const char_t* string)
	{
		// check string format
		if (!check_string_to_number_format(string)) return gen_nan();

		// parse string
	#ifdef PUGIHTML_WCHAR_MODE
		return wcstod(string, 0);
	#else
		return atof(string);
	#endif
	}

	bool convert_string_to_number(const char_t* begin, const char_t* end, double* out_result)
	{
		char_t buffer[32];

		size_t length = static_cast<size_t>(end - begin);
		char_t* scratch = buffer;

		if (length >= sizeof(buffer) / sizeof(buffer[0]))
		{
			// need to make dummy on-heap copy
			scratch = static_cast<char_t*>(global_allocate((length + 1) * sizeof(char_t)));
			if (!scratch) return false;
		}

		// copy string to zero-terminated buffer and perform conversion
		memcpy(scratch, begin, length * sizeof(char_t));
		scratch[length] = 0;

		*out_result = convert_string_to_number(scratch);

		// free dummy buffer
		if (scratch != buffer) global_deallocate(scratch);

		return true;
	}

	double round_nearest(double value)
	{
		return floor(value + 0.5);
	}

	double round_nearest_nzero(double value)
	{
		// same as round_nearest, but returns -0 for [-0.5, -0]
		// ceil is used to differentiate between +0 and -0 (we return -0 for [-0.5, -0] and +0 for +0)
		return (value >= -0.5 && value <= 0) ? ceil(value) : floor(value + 0.5);
	}

	const char_t* qualified_name(const xpath_node& node)
	{
		return node.attribute() ? node.attribute().name() : node.node().name();
	}

	const char_t* local_name(const xpath_node& node)
	{
		const char_t* name = qualified_name(node);
		const char_t* p = find_char(name, ':');

		return p ? p + 1 : name;
	}

	struct namespace_uri_predicate
	{
		const char_t* prefix;
		size_t prefix_length;

		namespace_uri_predicate(const char_t* name)
		{
			const char_t* pos = find_char(name, ':');

			prefix = pos ? name : 0;
			prefix_length = pos ? static_cast<size_t>(pos - name) : 0;
		}

		bool operator()(const attribute& a) const
		{
			const char_t* name = a.name();

			if (!starts_with(name, PUGIHTML_TEXT("htmlns"))) return false;

			return prefix ? name[5] == ':' && strequalrange(name + 6, prefix, prefix_length) : name[5] == 0;
		}
	};

	const char_t* namespace_uri(const node& node)
	{
		namespace_uri_predicate pred = node.name();

		node p = node;

		while (p)
		{
			attribute a = p.find_attribute(pred);

			if (a) return a.value();

			p = p.parent();
		}

		return PUGIHTML_TEXT("");
	}

	const char_t* namespace_uri(const attribute& attr, const node& parent)
	{
		namespace_uri_predicate pred = attr.name();

		// Default namespace does not apply to attributes
		if (!pred.prefix) return PUGIHTML_TEXT("");

		node p = parent;

		while (p)
		{
			attribute a = p.find_attribute(pred);

			if (a) return a.value();

			p = p.parent();
		}

		return PUGIHTML_TEXT("");
	}

	const char_t* namespace_uri(const xpath_node& node)
	{
		return node.attribute() ? namespace_uri(node.attribute(), node.parent()) : namespace_uri(node.node());
	}

	void normalize_space(char_t* buffer)
	{
		char_t* write = buffer;

		for (char_t* it = buffer; *it; )
		{
			char_t ch = *it++;

			if (IS_CHARTYPE(ch, ct_space))
			{
				// replace whitespace sequence with single space
				while (IS_CHARTYPE(*it, ct_space)) it++;

				// avoid leading spaces
				if (write != buffer) *write++ = ' ';
			}
			else *write++ = ch;
		}

		// remove trailing space
		if (write != buffer && IS_CHARTYPE(write[-1], ct_space)) write--;

		// zero-terminate
		*write = 0;
	}

	void translate(char_t* buffer, const char_t* from, const char_t* to)
	{
		size_t to_length = strlength(to);

		char_t* write = buffer;

		while (*buffer)
		{
			DMC_VOLATILE char_t ch = *buffer++;

			const char_t* pos = find_char(from, ch);

			if (!pos)
				*write++ = ch; // do not process
			else if (static_cast<size_t>(pos - from) < to_length)
				*write++ = to[pos - from]; // replace
		}

		// zero-terminate
		*write = 0;
	}

	struct xpath_variable_boolean: xpath_variable
	{
		xpath_variable_boolean(): value(false)
		{
		}

		bool value;
		char_t name[1];
	};

	struct xpath_variable_number: xpath_variable
	{
		xpath_variable_number(): value(0)
		{
		}

		double value;
		char_t name[1];
	};

	struct xpath_variable_string: xpath_variable
	{
		xpath_variable_string(): value(0)
		{
		}

		~xpath_variable_string()
		{
			if (value) global_deallocate(value);
		}

		char_t* value;
		char_t name[1];
	};

	struct xpath_variable_node_set: xpath_variable
	{
		xpath_node_set value;
		char_t name[1];
	};

	const xpath_node_set dummy_node_set;

	unsigned int hash_string(const char_t* str)
	{
		// Jenkins one-at-a-time hash (http://en.wikipedia.org/wiki/Jenkins_hash_function#one-at-a-time)
		unsigned int result = 0;

		while (*str)
		{
			result += static_cast<unsigned int>(*str++);
			result += result << 10;
			result ^= result >> 6;
		}

		result += result << 3;
		result ^= result >> 11;
		result += result << 15;

		return result;
	}

	template <typename T> T* new_xpath_variable(const char_t* name)
	{
		size_t length = strlength(name);
		if (length == 0) return 0; // empty variable names are invalid

		// $$ we can't use offsetof(T, name) because T is non-POD, so we just allocate additional length characters
		void* memory = global_allocate(sizeof(T) + length * sizeof(char_t));
		if (!memory) return 0;

		T* result = new (memory) T();

		memcpy(result->name, name, (length + 1) * sizeof(char_t));

		return result;
	}

	xpath_variable* new_xpath_variable(xpath_value_type type, const char_t* name)
	{
		switch (type)
		{
		case xpath_type_node_set:
			return new_xpath_variable<xpath_variable_node_set>(name);

		case xpath_type_number:
			return new_xpath_variable<xpath_variable_number>(name);

		case xpath_type_string:
			return new_xpath_variable<xpath_variable_string>(name);

		case xpath_type_boolean:
			return new_xpath_variable<xpath_variable_boolean>(name);

		default:
			return 0;
		}
	}

	template <typename T> void delete_xpath_variable(T* var)
	{
		var->~T();
		global_deallocate(var);
	}

	void delete_xpath_variable(xpath_value_type type, xpath_variable* var)
	{
		switch (type)
		{
		case xpath_type_node_set:
			delete_xpath_variable(static_cast<xpath_variable_node_set*>(var));
			break;

		case xpath_type_number:
			delete_xpath_variable(static_cast<xpath_variable_number*>(var));
			break;

		case xpath_type_string:
			delete_xpath_variable(static_cast<xpath_variable_string*>(var));
			break;

		case xpath_type_boolean:
			delete_xpath_variable(static_cast<xpath_variable_boolean*>(var));
			break;

		default:
			assert(!"Invalid variable type");
		}
	}

	xpath_variable* get_variable(xpath_variable_set* set, const char_t* begin, const char_t* end)
	{
		char_t buffer[32];

		size_t length = static_cast<size_t>(end - begin);
		char_t* scratch = buffer;

		if (length >= sizeof(buffer) / sizeof(buffer[0]))
		{
			// need to make dummy on-heap copy
			scratch = static_cast<char_t*>(global_allocate((length + 1) * sizeof(char_t)));
			if (!scratch) return 0;
		}

		// copy string to zero-terminated buffer and perform lookup
		memcpy(scratch, begin, length * sizeof(char_t));
		scratch[length] = 0;

		xpath_variable* result = set->get(scratch);

		// free dummy buffer
		if (scratch != buffer) global_deallocate(scratch);

		return result;
	}
}

// Internal node set class
namespace
{
	xpath_node_set::type_t xpath_sort(xpath_node* begin, xpath_node* end, xpath_node_set::type_t type, bool rev)
	{
		xpath_node_set::type_t order = rev ? xpath_node_set::type_sorted_reverse : xpath_node_set::type_sorted;

		if (type == xpath_node_set::type_unsorted)
		{
			sort(begin, end, document_order_comparator());

			type = xpath_node_set::type_sorted;
		}

		if (type != order) reverse(begin, end);

		return order;
	}

	xpath_node xpath_first(const xpath_node* begin, const xpath_node* end, xpath_node_set::type_t type)
	{
		if (begin == end) return xpath_node();

		switch (type)
		{
		case xpath_node_set::type_sorted:
			return *begin;

		case xpath_node_set::type_sorted_reverse:
			return *(end - 1);

		case xpath_node_set::type_unsorted:
			return *min_element(begin, end, document_order_comparator());

		default:
			assert(!"Invalid node set type");
			return xpath_node();
		}
	}
	class xpath_node_set_raw
	{
		xpath_node_set::type_t _type;

		xpath_node* _begin;
		xpath_node* _end;
		xpath_node* _eos;

	public:
		xpath_node_set_raw(): _type(xpath_node_set::type_unsorted), _begin(0), _end(0), _eos(0)
		{
		}

		xpath_node* begin() const
		{
			return _begin;
		}

		xpath_node* end() const
		{
			return _end;
		}

		bool empty() const
		{
			return _begin == _end;
		}

		size_t size() const
		{
			return static_cast<size_t>(_end - _begin);
		}

		xpath_node first() const
		{
			return xpath_first(_begin, _end, _type);
		}

		void push_back(const xpath_node& node, xpath_allocator* alloc)
		{
			if (_end == _eos)
			{
				size_t capacity = static_cast<size_t>(_eos - _begin);

				// get new capacity (1.5x rule)
				size_t new_capacity = capacity + capacity / 2 + 1;

				// reallocate the old array or allocate a new one
				xpath_node* data = static_cast<xpath_node*>(alloc->reallocate(_begin, capacity * sizeof(xpath_node), new_capacity * sizeof(xpath_node)));
				assert(data);

				// finalize
				_begin = data;
				_end = data + capacity;
				_eos = data + new_capacity;
			}

			*_end++ = node;
		}

		void append(const xpath_node* begin, const xpath_node* end, xpath_allocator* alloc)
		{
			size_t size = static_cast<size_t>(_end - _begin);
			size_t capacity = static_cast<size_t>(_eos - _begin);
			size_t count = static_cast<size_t>(end - begin);

			if (size + count > capacity)
			{
				// reallocate the old array or allocate a new one
				xpath_node* data = static_cast<xpath_node*>(alloc->reallocate(_begin, capacity * sizeof(xpath_node), (size + count) * sizeof(xpath_node)));
				assert(data);

				// finalize
				_begin = data;
				_end = data + size;
				_eos = data + size + count;
			}

			memcpy(_end, begin, count * sizeof(xpath_node));
			_end += count;
		}

		void sort_do()
		{
			_type = xpath_sort(_begin, _end, _type, false);
		}

		void truncate(xpath_node* pos)
		{
			assert(_begin <= pos && pos <= _end);

			_end = pos;
		}

		void remove_duplicates()
		{
			if (_type == xpath_node_set::type_unsorted)
				sort(_begin, _end, duplicate_comparator());

			_end = unique(_begin, _end);
		}

		xpath_node_set::type_t type() const
		{
			return _type;
		}

		void set_type(xpath_node_set::type_t type)
		{
			_type = type;
		}
	};
}

namespace
{
	struct xpath_context
	{
		xpath_node n;
		size_t position, size;

		xpath_context(const xpath_node& n, size_t position, size_t size): n(n), position(position), size(size)
		{
		}
	};

	enum lexeme_t
	{
		lex_none = 0,
		lex_equal,
		lex_not_equal,
		lex_less,
		lex_greater,
		lex_less_or_equal,
		lex_greater_or_equal,
		lex_plus,
		lex_minus,
		lex_multiply,
		lex_union,
		lex_var_ref,
		lex_open_brace,
		lex_close_brace,
		lex_quoted_string,
		lex_number,
		lex_slash,
		lex_double_slash,
		lex_open_square_brace,
		lex_close_square_brace,
		lex_string,
		lex_comma,
		lex_axis_attribute,
		lex_dot,
		lex_double_dot,
		lex_double_colon,
		lex_eof
	};

	struct xpath_lexer_string
	{
		const char_t* begin;
		const char_t* end;

		xpath_lexer_string(): begin(0), end(0)
		{
		}

		bool operator==(const char_t* other) const
		{
			size_t length = static_cast<size_t>(end - begin);

			return strequalrange(other, begin, length);
		}
	};

	class xpath_lexer
	{
		const char_t* _cur;
		const char_t* _cur_lexeme_pos;
		xpath_lexer_string _cur_lexeme_contents;

		lexeme_t _cur_lexeme;

	public:
		explicit xpath_lexer(const char_t* query): _cur(query)
		{
			next();
		}

		const char_t* state() const
		{
			return _cur;
		}

		void next()
		{
			const char_t* cur = _cur;

			while (IS_CHARTYPE(*cur, ct_space)) ++cur;

			// save lexeme position for error reporting
			_cur_lexeme_pos = cur;

			switch (*cur)
			{
			case 0:
				_cur_lexeme = lex_eof;
				break;

			case '>':
				if (*(cur+1) == '=')
				{
					cur += 2;
					_cur_lexeme = lex_greater_or_equal;
				}
				else
				{
					cur += 1;
					_cur_lexeme = lex_greater;
				}
				break;

			case '<':
				if (*(cur+1) == '=')
				{
					cur += 2;
					_cur_lexeme = lex_less_or_equal;
				}
				else
				{
					cur += 1;
					_cur_lexeme = lex_less;
				}
				break;

			case '!':
				if (*(cur+1) == '=')
				{
					cur += 2;
					_cur_lexeme = lex_not_equal;
				}
				else
				{
					_cur_lexeme = lex_none;
				}
				break;

			case '=':
				cur += 1;
				_cur_lexeme = lex_equal;

				break;

			case '+':
				cur += 1;
				_cur_lexeme = lex_plus;

				break;

			case '-':
				cur += 1;
				_cur_lexeme = lex_minus;

				break;

			case '*':
				cur += 1;
				_cur_lexeme = lex_multiply;

				break;

			case '|':
				cur += 1;
				_cur_lexeme = lex_union;

				break;

			case '$':
				cur += 1;

				if (IS_CHARTYPEX(*cur, ctx_start_symbol))
				{
					_cur_lexeme_contents.begin = cur;

					while (IS_CHARTYPEX(*cur, ctx_symbol)) cur++;

					if (cur[0] == ':' && IS_CHARTYPEX(cur[1], ctx_symbol)) // qname
					{
						cur++; // :

						while (IS_CHARTYPEX(*cur, ctx_symbol)) cur++;
					}

					_cur_lexeme_contents.end = cur;

					_cur_lexeme = lex_var_ref;
				}
				else
				{
					_cur_lexeme = lex_none;
				}

				break;

			case '(':
				cur += 1;
				_cur_lexeme = lex_open_brace;

				break;

			case ')':
				cur += 1;
				_cur_lexeme = lex_close_brace;

				break;

			case '[':
				cur += 1;
				_cur_lexeme = lex_open_square_brace;

				break;

			case ']':
				cur += 1;
				_cur_lexeme = lex_close_square_brace;

				break;

			case ',':
				cur += 1;
				_cur_lexeme = lex_comma;

				break;

			case '/':
				if (*(cur+1) == '/')
				{
					cur += 2;
					_cur_lexeme = lex_double_slash;
				}
				else
				{
					cur += 1;
					_cur_lexeme = lex_slash;
				}
				break;

			case '.':
				if (*(cur+1) == '.')
				{
					cur += 2;
					_cur_lexeme = lex_double_dot;
				}
				else if (IS_CHARTYPEX(*(cur+1), ctx_digit))
				{
					_cur_lexeme_contents.begin = cur; // .

					++cur;

					while (IS_CHARTYPEX(*cur, ctx_digit)) cur++;

					_cur_lexeme_contents.end = cur;

					_cur_lexeme = lex_number;
				}
				else
				{
					cur += 1;
					_cur_lexeme = lex_dot;
				}
				break;

			case '@':
				cur += 1;
				_cur_lexeme = lex_axis_attribute;

				break;

			case '"':
			case '\'':
			{
				char_t terminator = *cur;

				++cur;

				_cur_lexeme_contents.begin = cur;
				while (*cur && *cur != terminator) cur++;
				_cur_lexeme_contents.end = cur;

				if (!*cur)
					_cur_lexeme = lex_none;
				else
				{
					cur += 1;
					_cur_lexeme = lex_quoted_string;
				}

				break;
			}

			case ':':
				if (*(cur+1) == ':')
				{
					cur += 2;
					_cur_lexeme = lex_double_colon;
				}
				else
				{
					_cur_lexeme = lex_none;
				}
				break;

			default:
				if (IS_CHARTYPEX(*cur, ctx_digit))
				{
					_cur_lexeme_contents.begin = cur;

					while (IS_CHARTYPEX(*cur, ctx_digit)) cur++;

					if (*cur == '.')
					{
						cur++;

						while (IS_CHARTYPEX(*cur, ctx_digit)) cur++;
					}

					_cur_lexeme_contents.end = cur;

					_cur_lexeme = lex_number;
				}
				else if (IS_CHARTYPEX(*cur, ctx_start_symbol))
				{
					_cur_lexeme_contents.begin = cur;

					while (IS_CHARTYPEX(*cur, ctx_symbol)) cur++;

					if (cur[0] == ':')
					{
						if (cur[1] == '*') // namespace test ncname:*
						{
							cur += 2; // :*
						}
						else if (IS_CHARTYPEX(cur[1], ctx_symbol)) // namespace test qname
						{
							cur++; // :

							while (IS_CHARTYPEX(*cur, ctx_symbol)) cur++;
						}
					}

					_cur_lexeme_contents.end = cur;

					_cur_lexeme = lex_string;
				}
				else
				{
					_cur_lexeme = lex_none;
				}
			}

			_cur = cur;
		}

		lexeme_t current() const
		{
			return _cur_lexeme;
		}

		const char_t* current_pos() const
		{
			return _cur_lexeme_pos;
		}

		const xpath_lexer_string& contents() const
		{
			assert(_cur_lexeme == lex_var_ref || _cur_lexeme == lex_number || _cur_lexeme == lex_string || _cur_lexeme == lex_quoted_string);

			return _cur_lexeme_contents;
		}
	};

	enum ast_type_t
	{
		ast_op_or,						// left or right
		ast_op_and,						// left and right
		ast_op_equal,					// left = right
		ast_op_not_equal,				// left != right
		ast_op_less,					// left < right
		ast_op_greater,					// left > right
		ast_op_less_or_equal,			// left <= right
		ast_op_greater_or_equal,		// left >= right
		ast_op_add,						// left + right
		ast_op_subtract,				// left - right
		ast_op_multiply,				// left * right
		ast_op_divide,					// left / right
		ast_op_mod,						// left % right
		ast_op_negate,					// left - right
		ast_op_union,					// left | right
		ast_predicate,					// apply predicate to set; next points to next predicate
		ast_filter,						// select * from left where right
		ast_filter_posinv,				// select * from left where right; proximity position invariant
		ast_string_constant,			// string constant
		ast_number_constant,			// number constant
		ast_variable,					// variable
		ast_func_last,					// last()
		ast_func_position,				// position()
		ast_func_count,					// count(left)
		ast_func_id,					// id(left)
		ast_func_local_name_0,			// local-name()
		ast_func_local_name_1,			// local-name(left)
		ast_func_namespace_uri_0,		// namespace-uri()
		ast_func_namespace_uri_1,		// namespace-uri(left)
		ast_func_name_0,				// name()
		ast_func_name_1,				// name(left)
		ast_func_string_0,				// string()
		ast_func_string_1,				// string(left)
		ast_func_concat,				// concat(left, right, siblings)
		ast_func_starts_with,			// starts_with(left, right)
		ast_func_contains,				// contains(left, right)
		ast_func_substring_before,		// substring-before(left, right)
		ast_func_substring_after,		// substring-after(left, right)
		ast_func_substring_2,			// substring(left, right)
		ast_func_substring_3,			// substring(left, right, third)
		ast_func_string_length_0,		// string-length()
		ast_func_string_length_1,		// string-length(left)
		ast_func_normalize_space_0,		// normalize-space()
		ast_func_normalize_space_1,		// normalize-space(left)
		ast_func_translate,				// translate(left, right, third)
		ast_func_boolean,				// boolean(left)
		ast_func_not,					// not(left)
		ast_func_true,					// true()
		ast_func_false,					// false()
		ast_func_lang,					// lang(left)
		ast_func_number_0,				// number()
		ast_func_number_1,				// number(left)
		ast_func_sum,					// sum(left)
		ast_func_floor,					// floor(left)
		ast_func_ceiling,				// ceiling(left)
		ast_func_round,					// round(left)
		ast_step,						// process set left with step
		ast_step_root					// select root node
	};

	enum axis_t
	{
		axis_ancestor,
		axis_ancestor_or_self,
		axis_attribute,
		axis_child,
		axis_descendant,
		axis_descendant_or_self,
		axis_following,
		axis_following_sibling,
		axis_namespace,
		axis_parent,
		axis_preceding,
		axis_preceding_sibling,
		axis_self
	};

	enum nodetest_t
	{
		nodetest_none,
		nodetest_name,
		nodetest_type_node,
		nodetest_type_comment,
		nodetest_type_pi,
		nodetest_type_text,
		nodetest_pi,
		nodetest_all,
		nodetest_all_in_namespace
	};

	template <axis_t N> struct axis_to_type
	{
		static const axis_t axis;
	};

	template <axis_t N> const axis_t axis_to_type<N>::axis = N;

	class xpath_ast_node
	{
	private:
		// node type
		char _type;
		char _rettype;

		// for ast_step / ast_predicate
		char _axis;
		char _test;

		// tree node structure
		xpath_ast_node* _left;
		xpath_ast_node* _right;
		xpath_ast_node* _next;

		union
		{
			// value for ast_string_constant
			const char_t* string;
			// value for ast_number_constant
			double number;
			// variable for ast_variable
			xpath_variable* variable;
			// node test for ast_step (node name/namespace/node type/pi target)
			const char_t* nodetest;
		} _data;

		xpath_ast_node(const xpath_ast_node&);
		xpath_ast_node& operator=(const xpath_ast_node&);

		template <class Comp> static bool compare_eq(xpath_ast_node* lhs, xpath_ast_node* rhs, const xpath_context& c, const xpath_stack& stack, const Comp& comp)
		{
			xpath_value_type lt = lhs->rettype(), rt = rhs->rettype();

			if (lt != xpath_type_node_set && rt != xpath_type_node_set)
			{
				if (lt == xpath_type_boolean || rt == xpath_type_boolean)
					return comp(lhs->eval_boolean(c, stack), rhs->eval_boolean(c, stack));
				else if (lt == xpath_type_number || rt == xpath_type_number)
					return comp(lhs->eval_number(c, stack), rhs->eval_number(c, stack));
				else if (lt == xpath_type_string || rt == xpath_type_string)
				{
					xpath_allocator_capture cr(stack.result);

					xpath_string ls = lhs->eval_string(c, stack);
					xpath_string rs = rhs->eval_string(c, stack);

					return comp(ls, rs);
				}
			}
			else if (lt == xpath_type_node_set && rt == xpath_type_node_set)
			{
				xpath_allocator_capture cr(stack.result);

				xpath_node_set_raw ls = lhs->eval_node_set(c, stack);
				xpath_node_set_raw rs = rhs->eval_node_set(c, stack);

				for (const xpath_node* li = ls.begin(); li != ls.end(); ++li)
					for (const xpath_node* ri = rs.begin(); ri != rs.end(); ++ri)
					{
						xpath_allocator_capture cri(stack.result);

						if (comp(string_value(*li, stack.result), string_value(*ri, stack.result)))
							return true;
					}

				return false;
			}
			else
			{
				if (lt == xpath_type_node_set)
				{
					swap(lhs, rhs);
					swap(lt, rt);
				}

				if (lt == xpath_type_boolean)
					return comp(lhs->eval_boolean(c, stack), rhs->eval_boolean(c, stack));
				else if (lt == xpath_type_number)
				{
					xpath_allocator_capture cr(stack.result);

					double l = lhs->eval_number(c, stack);
					xpath_node_set_raw rs = rhs->eval_node_set(c, stack);

					for (const xpath_node* ri = rs.begin(); ri != rs.end(); ++ri)
					{
						xpath_allocator_capture cri(stack.result);

						if (comp(l, convert_string_to_number(string_value(*ri, stack.result).c_str())))
							return true;
					}

					return false;
				}
				else if (lt == xpath_type_string)
				{
					xpath_allocator_capture cr(stack.result);

					xpath_string l = lhs->eval_string(c, stack);
					xpath_node_set_raw rs = rhs->eval_node_set(c, stack);

					for (const xpath_node* ri = rs.begin(); ri != rs.end(); ++ri)
					{
						xpath_allocator_capture cri(stack.result);

						if (comp(l, string_value(*ri, stack.result)))
							return true;
					}

					return false;
				}
			}

			assert(!"Wrong types");
			return false;
		}

		template <class Comp> static bool compare_rel(xpath_ast_node* lhs, xpath_ast_node* rhs, const xpath_context& c, const xpath_stack& stack, const Comp& comp)
		{
			xpath_value_type lt = lhs->rettype(), rt = rhs->rettype();

			if (lt != xpath_type_node_set && rt != xpath_type_node_set)
				return comp(lhs->eval_number(c, stack), rhs->eval_number(c, stack));
			else if (lt == xpath_type_node_set && rt == xpath_type_node_set)
			{
				xpath_allocator_capture cr(stack.result);

				xpath_node_set_raw ls = lhs->eval_node_set(c, stack);
				xpath_node_set_raw rs = rhs->eval_node_set(c, stack);

				for (const xpath_node* li = ls.begin(); li != ls.end(); ++li)
				{
					xpath_allocator_capture cri(stack.result);

					double l = convert_string_to_number(string_value(*li, stack.result).c_str());

					for (const xpath_node* ri = rs.begin(); ri != rs.end(); ++ri)
					{
						xpath_allocator_capture crii(stack.result);

						if (comp(l, convert_string_to_number(string_value(*ri, stack.result).c_str())))
							return true;
					}
				}

				return false;
			}
			else if (lt != xpath_type_node_set && rt == xpath_type_node_set)
			{
				xpath_allocator_capture cr(stack.result);

				double l = lhs->eval_number(c, stack);
				xpath_node_set_raw rs = rhs->eval_node_set(c, stack);

				for (const xpath_node* ri = rs.begin(); ri != rs.end(); ++ri)
				{
					xpath_allocator_capture cri(stack.result);

					if (comp(l, convert_string_to_number(string_value(*ri, stack.result).c_str())))
						return true;
				}

				return false;
			}
			else if (lt == xpath_type_node_set && rt != xpath_type_node_set)
			{
				xpath_allocator_capture cr(stack.result);

				xpath_node_set_raw ls = lhs->eval_node_set(c, stack);
				double r = rhs->eval_number(c, stack);

				for (const xpath_node* li = ls.begin(); li != ls.end(); ++li)
				{
					xpath_allocator_capture cri(stack.result);

					if (comp(convert_string_to_number(string_value(*li, stack.result).c_str()), r))
						return true;
				}

				return false;
			}
			else
			{
				assert(!"Wrong types");
				return false;
			}
		}

		void apply_predicate(xpath_node_set_raw& ns, size_t first, xpath_ast_node* expr, const xpath_stack& stack)
		{
			assert(ns.size() >= first);

			size_t i = 1;
			size_t size = ns.size() - first;

			xpath_node* last = ns.begin() + first;

			// remove_if... or well, sort of
			for (xpath_node* it = last; it != ns.end(); ++it, ++i)
			{
				xpath_context c(*it, i, size);

				if (expr->rettype() == xpath_type_number)
				{
					if (expr->eval_number(c, stack) == i)
						*last++ = *it;
				}
				else if (expr->eval_boolean(c, stack))
					*last++ = *it;
			}

			ns.truncate(last);
		}

		void apply_predicates(xpath_node_set_raw& ns, size_t first, const xpath_stack& stack)
		{
			if (ns.size() == first) return;

			for (xpath_ast_node* pred = _right; pred; pred = pred->_next)
			{
				apply_predicate(ns, first, pred->_left, stack);
			}
		}

		void step_push(xpath_node_set_raw& ns, const attribute& a, const node& parent, xpath_allocator* alloc)
		{
			if (!a) return;

			const char_t* name = a.name();

			// There are no attribute nodes corresponding to attributes that declare namespaces
			// That is, "htmlns:..." or "htmlns"
			if (starts_with(name, PUGIHTML_TEXT("htmlns")) && (name[5] == 0 || name[5] == ':')) return;

			switch (_test)
			{
			case nodetest_name:
				if (strequal(name, _data.nodetest)) ns.push_back(xpath_node(a, parent), alloc);
				break;

			case nodetest_type_node:
			case nodetest_all:
				ns.push_back(xpath_node(a, parent), alloc);
				break;

			case nodetest_all_in_namespace:
				if (starts_with(name, _data.nodetest))
					ns.push_back(xpath_node(a, parent), alloc);
				break;

			default:
				;
			}
		}

		void step_push(xpath_node_set_raw& ns, const node& n, xpath_allocator* alloc)
		{
			if (!n) return;

			switch (_test)
			{
			case nodetest_name:
				if (n.type() == node_element && strequal(n.name(), _data.nodetest)) ns.push_back(n, alloc);
				break;

			case nodetest_type_node:
				ns.push_back(n, alloc);
				break;

			case nodetest_type_comment:
				if (n.type() == node_comment)
					ns.push_back(n, alloc);
				break;

			case nodetest_type_text:
				if (n.type() == node_pcdata || n.type() == node_cdata)
					ns.push_back(n, alloc);
				break;

			case nodetest_type_pi:
				if (n.type() == node_pi)
					ns.push_back(n, alloc);
				break;

			case nodetest_pi:
				if (n.type() == node_pi && strequal(n.name(), _data.nodetest))
					ns.push_back(n, alloc);
				break;

			case nodetest_all:
				if (n.type() == node_element)
					ns.push_back(n, alloc);
				break;

			case nodetest_all_in_namespace:
				if (n.type() == node_element && starts_with(n.name(), _data.nodetest))
					ns.push_back(n, alloc);
				break;

			default:
				assert(!"Unknown axis");
			}
		}

		template <class T> void step_fill(xpath_node_set_raw& ns, const node& n, xpath_allocator* alloc, T)
		{
			const axis_t axis = T::axis;

			switch (axis)
			{
			case axis_attribute:
			{
				for (attribute a = n.first_attribute(); a; a = a.next_attribute())
					step_push(ns, a, n, alloc);

				break;
			}

			case axis_child:
			{
				for (node c = n.first_child(); c; c = c.next_sibling())
					step_push(ns, c, alloc);

				break;
			}

			case axis_descendant:
			case axis_descendant_or_self:
			{
				if (axis == axis_descendant_or_self)
					step_push(ns, n, alloc);

				node cur = n.first_child();

				while (cur && cur != n)
				{
					step_push(ns, cur, alloc);

					if (cur.first_child())
						cur = cur.first_child();
					else if (cur.next_sibling())
						cur = cur.next_sibling();
					else
					{
						while (!cur.next_sibling() && cur != n)
							cur = cur.parent();

						if (cur != n) cur = cur.next_sibling();
					}
				}

				break;
			}

			case axis_following_sibling:
			{
				for (node c = n.next_sibling(); c; c = c.next_sibling())
					step_push(ns, c, alloc);

				break;
			}

			case axis_preceding_sibling:
			{
				for (node c = n.previous_sibling(); c; c = c.previous_sibling())
					step_push(ns, c, alloc);

				break;
			}

			case axis_following:
			{
				node cur = n;

				// exit from this node so that we don't include descendants
				while (cur && !cur.next_sibling()) cur = cur.parent();
				cur = cur.next_sibling();

				for (;;)
				{
					step_push(ns, cur, alloc);

					if (cur.first_child())
						cur = cur.first_child();
					else if (cur.next_sibling())
						cur = cur.next_sibling();
					else
					{
						while (cur && !cur.next_sibling()) cur = cur.parent();
						cur = cur.next_sibling();

						if (!cur) break;
					}
				}

				break;
			}

			case axis_preceding:
			{
				node cur = n;

				while (cur && !cur.previous_sibling()) cur = cur.parent();
				cur = cur.previous_sibling();

				for (;;)
				{
					if (cur.last_child())
						cur = cur.last_child();
					else
					{
						// leaf node, can't be ancestor
						step_push(ns, cur, alloc);

						if (cur.previous_sibling())
							cur = cur.previous_sibling();
						else
						{
							do
							{
								cur = cur.parent();
								if (!cur) break;

								if (!node_is_ancestor(cur, n)) step_push(ns, cur, alloc);
							}
							while (!cur.previous_sibling());

							cur = cur.previous_sibling();

							if (!cur) break;
						}
					}
				}

				break;
			}

			case axis_ancestor:
			case axis_ancestor_or_self:
			{
				if (axis == axis_ancestor_or_self)
					step_push(ns, n, alloc);

				node cur = n.parent();

				while (cur)
				{
					step_push(ns, cur, alloc);

					cur = cur.parent();
				}

				break;
			}

			case axis_self:
			{
				step_push(ns, n, alloc);

				break;
			}

			case axis_parent:
			{
				if (n.parent()) step_push(ns, n.parent(), alloc);

				break;
			}

			default:
				assert(!"Unimplemented axis");
			}
		}

		template <class T> void step_fill(xpath_node_set_raw& ns, const attribute& a, const node& p, xpath_allocator* alloc, T v)
		{
			const axis_t axis = T::axis;

			switch (axis)
			{
			case axis_ancestor:
			case axis_ancestor_or_self:
			{
				if (axis == axis_ancestor_or_self && _test == nodetest_type_node) // reject attributes based on principal node type test
					step_push(ns, a, p, alloc);

				node cur = p;

				while (cur)
				{
					step_push(ns, cur, alloc);

					cur = cur.parent();
				}

				break;
			}

			case axis_descendant_or_self:
			case axis_self:
			{
				if (_test == nodetest_type_node) // reject attributes based on principal node type test
					step_push(ns, a, p, alloc);

				break;
			}

			case axis_following:
			{
				node cur = p;

				for (;;)
				{
					if (cur.first_child())
						cur = cur.first_child();
					else if (cur.next_sibling())
						cur = cur.next_sibling();
					else
					{
						while (cur && !cur.next_sibling()) cur = cur.parent();
						cur = cur.next_sibling();

						if (!cur) break;
					}

					step_push(ns, cur, alloc);
				}

				break;
			}

			case axis_parent:
			{
				step_push(ns, p, alloc);

				break;
			}

			case axis_preceding:
			{
				// preceding:: axis does not include attribute nodes and attribute ancestors (they are the same as parent's ancestors), so we can reuse node preceding
				step_fill(ns, p, alloc, v);
				break;
			}

			default:
				assert(!"Unimplemented axis");
			}
		}

		template <class T> xpath_node_set_raw step_do(const xpath_context& c, const xpath_stack& stack, T v)
		{
			const axis_t axis = T::axis;
			bool attributes = (axis == axis_ancestor || axis == axis_ancestor_or_self || axis == axis_descendant_or_self || axis == axis_following || axis == axis_parent || axis == axis_preceding || axis == axis_self);

			xpath_node_set_raw ns;
			ns.set_type((axis == axis_ancestor || axis == axis_ancestor_or_self || axis == axis_preceding || axis == axis_preceding_sibling) ? xpath_node_set::type_sorted_reverse : xpath_node_set::type_sorted);

			if (_left)
			{
				xpath_node_set_raw s = _left->eval_node_set(c, stack);

				// self axis preserves the original order
				if (axis == axis_self) ns.set_type(s.type());

				for (const xpath_node* it = s.begin(); it != s.end(); ++it)
				{
					size_t size = ns.size();

					// in general, all axes generate elements in a particular order, but there is no order guarantee if axis is applied to two nodes
					if (axis != axis_self && size != 0) ns.set_type(xpath_node_set::type_unsorted);

					if (it->node())
						step_fill(ns, it->node(), stack.result, v);
					else if (attributes)
						step_fill(ns, it->attribute(), it->parent(), stack.result, v);

					apply_predicates(ns, size, stack);
				}
			}
			else
			{
				if (c.n.node())
					step_fill(ns, c.n.node(), stack.result, v);
				else if (attributes)
					step_fill(ns, c.n.attribute(), c.n.parent(), stack.result, v);

				apply_predicates(ns, 0, stack);
			}

			// child, attribute and self axes always generate unique set of nodes
			// for other axis, if the set stayed sorted, it stayed unique because the traversal algorithms do not visit the same node twice
			if (axis != axis_child && axis != axis_attribute && axis != axis_self && ns.type() == xpath_node_set::type_unsorted)
				ns.remove_duplicates();

			return ns;
		}

	public:
		xpath_ast_node(ast_type_t type, xpath_value_type rettype, const char_t* value):
			_type((char)type), _rettype((char)rettype), _axis(0), _test(0), _left(0), _right(0), _next(0)
		{
			assert(type == ast_string_constant);
			_data.string = value;
		}

		xpath_ast_node(ast_type_t type, xpath_value_type rettype, double value):
			_type((char)type), _rettype((char)rettype), _axis(0), _test(0), _left(0), _right(0), _next(0)
		{
			assert(type == ast_number_constant);
			_data.number = value;
		}

		xpath_ast_node(ast_type_t type, xpath_value_type rettype, xpath_variable* value):
			_type((char)type), _rettype((char)rettype), _axis(0), _test(0), _left(0), _right(0), _next(0)
		{
			assert(type == ast_variable);
			_data.variable = value;
		}

		xpath_ast_node(ast_type_t type, xpath_value_type rettype, xpath_ast_node* left = 0, xpath_ast_node* right = 0):
			_type((char)type), _rettype((char)rettype), _axis(0), _test(0), _left(left), _right(right), _next(0)
		{
		}

		xpath_ast_node(ast_type_t type, xpath_ast_node* left, axis_t axis, nodetest_t test, const char_t* contents):
			_type((char)type), _rettype(xpath_type_node_set), _axis((char)axis), _test((char)test), _left(left), _right(0), _next(0)
		{
			_data.nodetest = contents;
		}

		void set_next(xpath_ast_node* value)
		{
			_next = value;
		}

		void set_right(xpath_ast_node* value)
		{
			_right = value;
		}

		bool eval_boolean(const xpath_context& c, const xpath_stack& stack)
		{
			switch (_type)
			{
			case ast_op_or:
				return _left->eval_boolean(c, stack) || _right->eval_boolean(c, stack);

			case ast_op_and:
				return _left->eval_boolean(c, stack) && _right->eval_boolean(c, stack);

			case ast_op_equal:
				return compare_eq(_left, _right, c, stack, equal_to());

			case ast_op_not_equal:
				return compare_eq(_left, _right, c, stack, not_equal_to());

			case ast_op_less:
				return compare_rel(_left, _right, c, stack, less());

			case ast_op_greater:
				return compare_rel(_right, _left, c, stack, less());

			case ast_op_less_or_equal:
				return compare_rel(_left, _right, c, stack, less_equal());

			case ast_op_greater_or_equal:
				return compare_rel(_right, _left, c, stack, less_equal());

			case ast_func_starts_with:
			{
				xpath_allocator_capture cr(stack.result);

				xpath_string lr = _left->eval_string(c, stack);
				xpath_string rr = _right->eval_string(c, stack);

				return starts_with(lr.c_str(), rr.c_str());
			}

			case ast_func_contains:
			{
				xpath_allocator_capture cr(stack.result);

				xpath_string lr = _left->eval_string(c, stack);
				xpath_string rr = _right->eval_string(c, stack);

				return find_substring(lr.c_str(), rr.c_str()) != 0;
			}

			case ast_func_boolean:
				return _left->eval_boolean(c, stack);

			case ast_func_not:
				return !_left->eval_boolean(c, stack);

			case ast_func_true:
				return true;

			case ast_func_false:
				return false;

			case ast_func_lang:
			{
				if (c.n.attribute()) return false;

				xpath_allocator_capture cr(stack.result);

				xpath_string lang = _left->eval_string(c, stack);

				for (node n = c.n.node(); n; n = n.parent())
				{
					attribute a = n.attribute(PUGIHTML_TEXT("html:lang"));

					if (a)
					{
						const char_t* value = a.value();

						// strnicmp / strncasecmp is not portable
						for (const char_t* lit = lang.c_str(); *lit; ++lit)
						{
							if (tolower_ascii(*lit) != tolower_ascii(*value)) return false;
							++value;
						}

						return *value == 0 || *value == '-';
					}
				}

				return false;
			}

			case ast_variable:
			{
				assert(_rettype == _data.variable->type());

				if (_rettype == xpath_type_boolean)
					return _data.variable->get_boolean();

				// fallthrough to type conversion
			}

			default:
			{
				switch (_rettype)
				{
				case xpath_type_number:
					return convert_number_to_boolean(eval_number(c, stack));

				case xpath_type_string:
				{
					xpath_allocator_capture cr(stack.result);

					return !eval_string(c, stack).empty();
				}

				case xpath_type_node_set:
				{
					xpath_allocator_capture cr(stack.result);

					return !eval_node_set(c, stack).empty();
				}

				default:
					assert(!"Wrong expression for return type boolean");
					return false;
				}
			}
			}
		}

		double eval_number(const xpath_context& c, const xpath_stack& stack)
		{
			switch (_type)
			{
			case ast_op_add:
				return _left->eval_number(c, stack) + _right->eval_number(c, stack);

			case ast_op_subtract:
				return _left->eval_number(c, stack) - _right->eval_number(c, stack);

			case ast_op_multiply:
				return _left->eval_number(c, stack) * _right->eval_number(c, stack);

			case ast_op_divide:
				return _left->eval_number(c, stack) / _right->eval_number(c, stack);

			case ast_op_mod:
				return fmod(_left->eval_number(c, stack), _right->eval_number(c, stack));

			case ast_op_negate:
				return -_left->eval_number(c, stack);

			case ast_number_constant:
				return _data.number;

			case ast_func_last:
				return (double)c.size;

			case ast_func_position:
				return (double)c.position;

			case ast_func_count:
			{
				xpath_allocator_capture cr(stack.result);

				return (double)_left->eval_node_set(c, stack).size();
			}

			case ast_func_string_length_0:
			{
				xpath_allocator_capture cr(stack.result);

				return (double)string_value(c.n, stack.result).length();
			}

			case ast_func_string_length_1:
			{
				xpath_allocator_capture cr(stack.result);

				return (double)_left->eval_string(c, stack).length();
			}

			case ast_func_number_0:
			{
				xpath_allocator_capture cr(stack.result);

				return convert_string_to_number(string_value(c.n, stack.result).c_str());
			}

			case ast_func_number_1:
				return _left->eval_number(c, stack);

			case ast_func_sum:
			{
				xpath_allocator_capture cr(stack.result);

				double r = 0;

				xpath_node_set_raw ns = _left->eval_node_set(c, stack);

				for (const xpath_node* it = ns.begin(); it != ns.end(); ++it)
				{
					xpath_allocator_capture cri(stack.result);

					r += convert_string_to_number(string_value(*it, stack.result).c_str());
				}

				return r;
			}

			case ast_func_floor:
			{
				double r = _left->eval_number(c, stack);

				return r == r ? floor(r) : r;
			}

			case ast_func_ceiling:
			{
				double r = _left->eval_number(c, stack);

				return r == r ? ceil(r) : r;
			}

			case ast_func_round:
				return round_nearest_nzero(_left->eval_number(c, stack));

			case ast_variable:
			{
				assert(_rettype == _data.variable->type());

				if (_rettype == xpath_type_number)
					return _data.variable->get_number();

				// fallthrough to type conversion
			}

			default:
			{
				switch (_rettype)
				{
				case xpath_type_boolean:
					return eval_boolean(c, stack) ? 1 : 0;

				case xpath_type_string:
				{
					xpath_allocator_capture cr(stack.result);

					return convert_string_to_number(eval_string(c, stack).c_str());
				}

				case xpath_type_node_set:
				{
					xpath_allocator_capture cr(stack.result);

					return convert_string_to_number(eval_string(c, stack).c_str());
				}

				default:
					assert(!"Wrong expression for return type number");
					return 0;
				}

			}
			}
		}

		xpath_string eval_string_concat(const xpath_context& c, const xpath_stack& stack)
		{
			assert(_type == ast_func_concat);

			xpath_allocator_capture ct(stack.temp);

			// count the string number
			size_t count = 1;
			for (xpath_ast_node* nc = _right; nc; nc = nc->_next) count++;

			// gather all strings
			xpath_string static_buffer[4];
			xpath_string* buffer = static_buffer;

			// allocate on-heap for large concats
			if (count > sizeof(static_buffer) / sizeof(static_buffer[0]))
			{
				buffer = static_cast<xpath_string*>(stack.temp->allocate(count * sizeof(xpath_string)));
				assert(buffer);
			}

			// evaluate all strings to temporary stack
			xpath_stack swapped_stack = {stack.temp, stack.result};

			buffer[0] = _left->eval_string(c, swapped_stack);

			size_t pos = 1;
			for (xpath_ast_node* n = _right; n; n = n->_next, ++pos) buffer[pos] = n->eval_string(c, swapped_stack);
			assert(pos == count);

			// get total length
			size_t length = 0;
			for (size_t i = 0; i < count; ++i) length += buffer[i].length();

			// create final string
			char_t* result = static_cast<char_t*>(stack.result->allocate((length + 1) * sizeof(char_t)));
			assert(result);

			char_t* ri = result;

			for (size_t j = 0; j < count; ++j)
				for (const char_t* bi = buffer[j].c_str(); *bi; ++bi)
					*ri++ = *bi;

			*ri = 0;

			return xpath_string(result, true);
		}

		xpath_string eval_string(const xpath_context& c, const xpath_stack& stack)
		{
			switch (_type)
			{
			case ast_string_constant:
				return xpath_string_const(_data.string);

			case ast_func_local_name_0:
			{
				xpath_node na = c.n;

				return xpath_string_const(local_name(na));
			}

			case ast_func_local_name_1:
			{
				xpath_allocator_capture cr(stack.result);

				xpath_node_set_raw ns = _left->eval_node_set(c, stack);
				xpath_node na = ns.first();

				return xpath_string_const(local_name(na));
			}

			case ast_func_name_0:
			{
				xpath_node na = c.n;

				return xpath_string_const(qualified_name(na));
			}

			case ast_func_name_1:
			{
				xpath_allocator_capture cr(stack.result);

				xpath_node_set_raw ns = _left->eval_node_set(c, stack);
				xpath_node na = ns.first();

				return xpath_string_const(qualified_name(na));
			}

			case ast_func_namespace_uri_0:
			{
				xpath_node na = c.n;

				return xpath_string_const(namespace_uri(na));
			}

			case ast_func_namespace_uri_1:
			{
				xpath_allocator_capture cr(stack.result);

				xpath_node_set_raw ns = _left->eval_node_set(c, stack);
				xpath_node na = ns.first();

				return xpath_string_const(namespace_uri(na));
			}

			case ast_func_string_0:
				return string_value(c.n, stack.result);

			case ast_func_string_1:
				return _left->eval_string(c, stack);

			case ast_func_concat:
				return eval_string_concat(c, stack);

			case ast_func_substring_before:
			{
				xpath_allocator_capture cr(stack.temp);

				xpath_stack swapped_stack = {stack.temp, stack.result};

				xpath_string s = _left->eval_string(c, swapped_stack);
				xpath_string p = _right->eval_string(c, swapped_stack);

				const char_t* pos = find_substring(s.c_str(), p.c_str());

				return pos ? xpath_string(s.c_str(), pos, stack.result) : xpath_string();
			}

			case ast_func_substring_after:
			{
				xpath_allocator_capture cr(stack.temp);

				xpath_stack swapped_stack = {stack.temp, stack.result};

				xpath_string s = _left->eval_string(c, swapped_stack);
				xpath_string p = _right->eval_string(c, swapped_stack);

				const char_t* pos = find_substring(s.c_str(), p.c_str());
				if (!pos) return xpath_string();

				const char_t* result = pos + p.length();

				return s.uses_heap() ? xpath_string(result, stack.result) : xpath_string_const(result);
			}

			case ast_func_substring_2:
			{
				xpath_allocator_capture cr(stack.temp);

				xpath_stack swapped_stack = {stack.temp, stack.result};

				xpath_string s = _left->eval_string(c, swapped_stack);
				size_t s_length = s.length();

				double first = round_nearest(_right->eval_number(c, stack));

				if (is_nan(first)) return xpath_string(); // NaN
				else if (first >= s_length + 1) return xpath_string();

				size_t pos = first < 1 ? 1 : (size_t)first;
				assert(1 <= pos && pos <= s_length + 1);

				const char_t* rbegin = s.c_str() + (pos - 1);

				return s.uses_heap() ? xpath_string(rbegin, stack.result) : xpath_string_const(rbegin);
			}

			case ast_func_substring_3:
			{
				xpath_allocator_capture cr(stack.temp);

				xpath_stack swapped_stack = {stack.temp, stack.result};

				xpath_string s = _left->eval_string(c, swapped_stack);
				size_t s_length = s.length();

				double first = round_nearest(_right->eval_number(c, stack));
				double last = first + round_nearest(_right->_next->eval_number(c, stack));

				if (is_nan(first) || is_nan(last)) return xpath_string();
				else if (first >= s_length + 1) return xpath_string();
				else if (first >= last) return xpath_string();
				else if (last < 1) return xpath_string();

				size_t pos = first < 1 ? 1 : (size_t)first;
				size_t end = last >= s_length + 1 ? s_length + 1 : (size_t)last;

				assert(1 <= pos && pos <= end && end <= s_length + 1);
				const char_t* rbegin = s.c_str() + (pos - 1);
				const char_t* rend = s.c_str() + (end - 1);

				return (end == s_length + 1 && !s.uses_heap()) ? xpath_string_const(rbegin) : xpath_string(rbegin, rend, stack.result);
			}

			case ast_func_normalize_space_0:
			{
				xpath_string s = string_value(c.n, stack.result);

				normalize_space(s.data(stack.result));

				return s;
			}

			case ast_func_normalize_space_1:
			{
				xpath_string s = _left->eval_string(c, stack);

				normalize_space(s.data(stack.result));

				return s;
			}

			case ast_func_translate:
			{
				xpath_allocator_capture cr(stack.temp);

				xpath_stack swapped_stack = {stack.temp, stack.result};

				xpath_string s = _left->eval_string(c, stack);
				xpath_string from = _right->eval_string(c, swapped_stack);
				xpath_string to = _right->_next->eval_string(c, swapped_stack);

				translate(s.data(stack.result), from.c_str(), to.c_str());

				return s;
			}

			case ast_variable:
			{
				assert(_rettype == _data.variable->type());

				if (_rettype == xpath_type_string)
					return xpath_string_const(_data.variable->get_string());

				// fallthrough to type conversion
			}

			default:
			{
				switch (_rettype)
				{
				case xpath_type_boolean:
					return xpath_string_const(eval_boolean(c, stack) ? PUGIHTML_TEXT("true") : PUGIHTML_TEXT("false"));

				case xpath_type_number:
					return convert_number_to_string(eval_number(c, stack), stack.result);

				case xpath_type_node_set:
				{
					xpath_allocator_capture cr(stack.temp);

					xpath_stack swapped_stack = {stack.temp, stack.result};

					xpath_node_set_raw ns = eval_node_set(c, swapped_stack);
					return ns.empty() ? xpath_string() : string_value(ns.first(), stack.result);
				}

				default:
					assert(!"Wrong expression for return type string");
					return xpath_string();
				}
			}
			}
		}

		xpath_node_set_raw eval_node_set(const xpath_context& c, const xpath_stack& stack)
		{
			switch (_type)
			{
			case ast_op_union:
			{
				xpath_allocator_capture cr(stack.temp);

				xpath_stack swapped_stack = {stack.temp, stack.result};

				xpath_node_set_raw ls = _left->eval_node_set(c, swapped_stack);
				xpath_node_set_raw rs = _right->eval_node_set(c, stack);

				// we can optimize merging two sorted sets, but this is a very rare operation, so don't bother
				rs.set_type(xpath_node_set::type_unsorted);

				rs.append(ls.begin(), ls.end(), stack.result);
				rs.remove_duplicates();

				return rs;
			}

			case ast_filter:
			case ast_filter_posinv:
			{
				xpath_node_set_raw set = _left->eval_node_set(c, stack);

				// either expression is a number or it contains position() call; sort by document order
				if (_type == ast_filter) set.sort_do();

				apply_predicate(set, 0, _right, stack);

				return set;
			}

			case ast_func_id:
				return xpath_node_set_raw();

			case ast_step:
			{
				switch (_axis)
				{
				case axis_ancestor:
					return step_do(c, stack, axis_to_type<axis_ancestor>());

				case axis_ancestor_or_self:
					return step_do(c, stack, axis_to_type<axis_ancestor_or_self>());

				case axis_attribute:
					return step_do(c, stack, axis_to_type<axis_attribute>());

				case axis_child:
					return step_do(c, stack, axis_to_type<axis_child>());

				case axis_descendant:
					return step_do(c, stack, axis_to_type<axis_descendant>());

				case axis_descendant_or_self:
					return step_do(c, stack, axis_to_type<axis_descendant_or_self>());

				case axis_following:
					return step_do(c, stack, axis_to_type<axis_following>());

				case axis_following_sibling:
					return step_do(c, stack, axis_to_type<axis_following_sibling>());

				case axis_namespace:
					// namespaced axis is not supported
					return xpath_node_set_raw();

				case axis_parent:
					return step_do(c, stack, axis_to_type<axis_parent>());

				case axis_preceding:
					return step_do(c, stack, axis_to_type<axis_preceding>());

				case axis_preceding_sibling:
					return step_do(c, stack, axis_to_type<axis_preceding_sibling>());

				case axis_self:
					return step_do(c, stack, axis_to_type<axis_self>());
				}
			}

			case ast_step_root:
			{
				assert(!_right); // root step can't have any predicates

				xpath_node_set_raw ns;

				ns.set_type(xpath_node_set::type_sorted);

				if (c.n.node()) ns.push_back(c.n.node().root(), stack.result);
				else if (c.n.attribute()) ns.push_back(c.n.parent().root(), stack.result);

				return ns;
			}

			case ast_variable:
			{
				assert(_rettype == _data.variable->type());

				if (_rettype == xpath_type_node_set)
				{
					const xpath_node_set& s = _data.variable->get_node_set();

					xpath_node_set_raw ns;

					ns.set_type(s.type());
					ns.append(s.begin(), s.end(), stack.result);

					return ns;
				}

				// fallthrough to type conversion
			}

			default:
				assert(!"Wrong expression for return type node set");
				return xpath_node_set_raw();
			}
		}

		bool is_posinv()
		{
			switch (_type)
			{
			case ast_func_position:
				return false;

			case ast_string_constant:
			case ast_number_constant:
			case ast_variable:
				return true;

			case ast_step:
			case ast_step_root:
				return true;

			case ast_predicate:
			case ast_filter:
			case ast_filter_posinv:
				return true;

			default:
				if (_left && !_left->is_posinv()) return false;

				for (xpath_ast_node* n = _right; n; n = n->_next)
					if (!n->is_posinv()) return false;

				return true;
			}
		}

		xpath_value_type rettype() const
		{
			return static_cast<xpath_value_type>(_rettype);
		}
	};

	struct xpath_parser
	{
		xpath_allocator* _alloc;
		xpath_lexer _lexer;

		const char_t* _query;
		xpath_variable_set* _variables;

		xpath_parse_result* _result;

	#ifdef PUGIHTML_NO_EXCEPTIONS
		jmp_buf _error_handler;
	#endif

		void throw_error(const char* message)
		{
			_result->error = message;
			_result->offset = _lexer.current_pos() - _query;

		#ifdef PUGIHTML_NO_EXCEPTIONS
			longjmp(_error_handler, 1);
		#else
			throw xpath_exception(*_result);
		#endif
		}

		void throw_error_oom()
		{
		#ifdef PUGIHTML_NO_EXCEPTIONS
			throw_error("Out of memory");
		#else
			throw std::bad_alloc();
		#endif
		}

		void* alloc_node()
		{
			void* result = _alloc->allocate_nothrow(sizeof(xpath_ast_node));

			if (!result) throw_error_oom();

			return result;
		}

		const char_t* alloc_string(const xpath_lexer_string& value)
		{
			if (value.begin)
			{
				size_t length = static_cast<size_t>(value.end - value.begin);

				char_t* c = static_cast<char_t*>(_alloc->allocate_nothrow((length + 1) * sizeof(char_t)));
				if (!c) throw_error_oom();

				memcpy(c, value.begin, length * sizeof(char_t));
				c[length] = 0;

				return c;
			}
			else return 0;
		}

		xpath_ast_node* parse_function_helper(ast_type_t type0, ast_type_t type1, size_t argc, xpath_ast_node* args[2])
		{
			assert(argc <= 1);

			if (argc == 1 && args[0]->rettype() != xpath_type_node_set) throw_error("Function has to be applied to node set");

			return new (alloc_node()) xpath_ast_node(argc == 0 ? type0 : type1, xpath_type_string, args[0]);
		}

		xpath_ast_node* parse_function(const xpath_lexer_string& name, size_t argc, xpath_ast_node* args[2])
		{
			switch (name.begin[0])
			{
			case 'b':
				if (name == PUGIHTML_TEXT("boolean") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_boolean, xpath_type_boolean, args[0]);

				break;

			case 'c':
				if (name == PUGIHTML_TEXT("count") && argc == 1)
				{
					if (args[0]->rettype() != xpath_type_node_set) throw_error("Function has to be applied to node set");
					return new (alloc_node()) xpath_ast_node(ast_func_count, xpath_type_number, args[0]);
				}
				else if (name == PUGIHTML_TEXT("contains") && argc == 2)
					return new (alloc_node()) xpath_ast_node(ast_func_contains, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("concat") && argc >= 2)
					return new (alloc_node()) xpath_ast_node(ast_func_concat, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("ceiling") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_ceiling, xpath_type_number, args[0]);

				break;

			case 'f':
				if (name == PUGIHTML_TEXT("false") && argc == 0)
					return new (alloc_node()) xpath_ast_node(ast_func_false, xpath_type_boolean);
				else if (name == PUGIHTML_TEXT("floor") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_floor, xpath_type_number, args[0]);

				break;

			case 'i':
				if (name == PUGIHTML_TEXT("id") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_id, xpath_type_node_set, args[0]);

				break;

			case 'l':
				if (name == PUGIHTML_TEXT("last") && argc == 0)
					return new (alloc_node()) xpath_ast_node(ast_func_last, xpath_type_number);
				else if (name == PUGIHTML_TEXT("lang") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_lang, xpath_type_boolean, args[0]);
				else if (name == PUGIHTML_TEXT("local-name") && argc <= 1)
					return parse_function_helper(ast_func_local_name_0, ast_func_local_name_1, argc, args);

				break;

			case 'n':
				if (name == PUGIHTML_TEXT("name") && argc <= 1)
					return parse_function_helper(ast_func_name_0, ast_func_name_1, argc, args);
				else if (name == PUGIHTML_TEXT("namespace-uri") && argc <= 1)
					return parse_function_helper(ast_func_namespace_uri_0, ast_func_namespace_uri_1, argc, args);
				else if (name == PUGIHTML_TEXT("normalize-space") && argc <= 1)
					return new (alloc_node()) xpath_ast_node(argc == 0 ? ast_func_normalize_space_0 : ast_func_normalize_space_1, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("not") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_not, xpath_type_boolean, args[0]);
				else if (name == PUGIHTML_TEXT("number") && argc <= 1)
					return new (alloc_node()) xpath_ast_node(argc == 0 ? ast_func_number_0 : ast_func_number_1, xpath_type_number, args[0]);

				break;

			case 'p':
				if (name == PUGIHTML_TEXT("position") && argc == 0)
					return new (alloc_node()) xpath_ast_node(ast_func_position, xpath_type_number);

				break;

			case 'r':
				if (name == PUGIHTML_TEXT("round") && argc == 1)
					return new (alloc_node()) xpath_ast_node(ast_func_round, xpath_type_number, args[0]);

				break;

			case 's':
				if (name == PUGIHTML_TEXT("string") && argc <= 1)
					return new (alloc_node()) xpath_ast_node(argc == 0 ? ast_func_string_0 : ast_func_string_1, xpath_type_string, args[0]);
				else if (name == PUGIHTML_TEXT("string-length") && argc <= 1)
					return new (alloc_node()) xpath_ast_node(argc == 0 ? ast_func_string_length_0 : ast_func_string_length_1, xpath_type_string, args[0]);
				else if (name == PUGIHTML_TEXT("starts-with") && argc == 2)
					return new (alloc_node()) xpath_ast_node(ast_func_starts_with, xpath_type_boolean, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("substring-before") && argc == 2)
					return new (alloc_node()) xpath_ast_node(ast_func_substring_before, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("substring-after") && argc == 2)
					return new (alloc_node()) xpath_ast_node(ast_func_substring_after, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("substring") && (argc == 2 || argc == 3))
					return new (alloc_node()) xpath_ast_node(argc == 2 ? ast_func_substring_2 : ast_func_substring_3, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("sum") && argc == 1)
				{
					if (args[0]->rettype() != xpath_type_node_set) throw_error("Function has to be applied to node set");
					return new (alloc_node()) xpath_ast_node(ast_func_sum, xpath_type_number, args[0]);
				}

				break;

			case 't':
				if (name == PUGIHTML_TEXT("translate") && argc == 3)
					return new (alloc_node()) xpath_ast_node(ast_func_translate, xpath_type_string, args[0], args[1]);
				else if (name == PUGIHTML_TEXT("true") && argc == 0)
					return new (alloc_node()) xpath_ast_node(ast_func_true, xpath_type_boolean);

				break;
			}

			throw_error("Unrecognized function or wrong parameter count");

			return 0;
		}

		axis_t parse_axis_name(const xpath_lexer_string& name, bool& specified)
		{
			specified = true;

			switch (name.begin[0])
			{
			case 'a':
				if (name == PUGIHTML_TEXT("ancestor"))
					return axis_ancestor;
				else if (name == PUGIHTML_TEXT("ancestor-or-self"))
					return axis_ancestor_or_self;
				else if (name == PUGIHTML_TEXT("attribute"))
					return axis_attribute;

				break;

			case 'c':
				if (name == PUGIHTML_TEXT("child"))
					return axis_child;

				break;

			case 'd':
				if (name == PUGIHTML_TEXT("descendant"))
					return axis_descendant;
				else if (name == PUGIHTML_TEXT("descendant-or-self"))
					return axis_descendant_or_self;

				break;

			case 'f':
				if (name == PUGIHTML_TEXT("following"))
					return axis_following;
				else if (name == PUGIHTML_TEXT("following-sibling"))
					return axis_following_sibling;

				break;

			case 'n':
				if (name == PUGIHTML_TEXT("namespace"))
					return axis_namespace;

				break;

			case 'p':
				if (name == PUGIHTML_TEXT("parent"))
					return axis_parent;
				else if (name == PUGIHTML_TEXT("preceding"))
					return axis_preceding;
				else if (name == PUGIHTML_TEXT("preceding-sibling"))
					return axis_preceding_sibling;

				break;

			case 's':
				if (name == PUGIHTML_TEXT("self"))
					return axis_self;

				break;
			}

			specified = false;
			return axis_child;
		}

		nodetest_t parse_node_test_type(const xpath_lexer_string& name)
		{
			switch (name.begin[0])
			{
			case 'c':
				if (name == PUGIHTML_TEXT("comment"))
					return nodetest_type_comment;

				break;

			case 'n':
				if (name == PUGIHTML_TEXT("node"))
					return nodetest_type_node;

				break;

			case 'p':
				if (name == PUGIHTML_TEXT("processing-instruction"))
					return nodetest_type_pi;

				break;

			case 't':
				if (name == PUGIHTML_TEXT("text"))
					return nodetest_type_text;

				break;
			}

			return nodetest_none;
		}

		// PrimaryExpr ::= VariableReference | '(' Expr ')' | Literal | Number | FunctionCall
		xpath_ast_node* parse_primary_expression()
		{
			switch (_lexer.current())
			{
			case lex_var_ref:
			{
				xpath_lexer_string name = _lexer.contents();

				if (!_variables)
					throw_error("Unknown variable: variable set is not provided");

				xpath_variable* var = get_variable(_variables, name.begin, name.end);

				if (!var)
					throw_error("Unknown variable: variable set does not contain the given name");

				_lexer.next();

				return new (alloc_node()) xpath_ast_node(ast_variable, var->type(), var);
			}

			case lex_open_brace:
			{
				_lexer.next();

				xpath_ast_node* n = parse_expression();

				if (_lexer.current() != lex_close_brace)
					throw_error("Unmatched braces");

				_lexer.next();

				return n;
			}

			case lex_quoted_string:
			{
				const char_t* value = alloc_string(_lexer.contents());

				xpath_ast_node* n = new (alloc_node()) xpath_ast_node(ast_string_constant, xpath_type_string, value);
				_lexer.next();

				return n;
			}

			case lex_number:
			{
				double value = 0;

				if (!convert_string_to_number(_lexer.contents().begin, _lexer.contents().end, &value))
					throw_error_oom();

				xpath_ast_node* n = new (alloc_node()) xpath_ast_node(ast_number_constant, xpath_type_number, value);
				_lexer.next();

				return n;
			}

			case lex_string:
			{
				xpath_ast_node* args[2] = {0};
				size_t argc = 0;

				xpath_lexer_string function = _lexer.contents();
				_lexer.next();

				xpath_ast_node* last_arg = 0;

				if (_lexer.current() != lex_open_brace)
					throw_error("Unrecognized function call");
				_lexer.next();

				if (_lexer.current() != lex_close_brace)
					args[argc++] = parse_expression();

				while (_lexer.current() != lex_close_brace)
				{
					if (_lexer.current() != lex_comma)
						throw_error("No comma between function arguments");
					_lexer.next();

					xpath_ast_node* n = parse_expression();

					if (argc < 2) args[argc] = n;
					else last_arg->set_next(n);

					argc++;
					last_arg = n;
				}

				_lexer.next();

				return parse_function(function, argc, args);
			}

			default:
				throw_error("Unrecognizable primary expression");

				return 0;
			}
		}

		// FilterExpr ::= PrimaryExpr | FilterExpr Predicate
		// Predicate ::= '[' PredicateExpr ']'
		// PredicateExpr ::= Expr
		xpath_ast_node* parse_filter_expression()
		{
			xpath_ast_node* n = parse_primary_expression();

			while (_lexer.current() == lex_open_square_brace)
			{
				_lexer.next();

				xpath_ast_node* expr = parse_expression();

				if (n->rettype() != xpath_type_node_set) throw_error("Predicate has to be applied to node set");

				bool posinv = expr->rettype() != xpath_type_number && expr->is_posinv();

				n = new (alloc_node()) xpath_ast_node(posinv ? ast_filter_posinv : ast_filter, xpath_type_node_set, n, expr);

				if (_lexer.current() != lex_close_square_brace)
					throw_error("Unmatched square brace");

				_lexer.next();
			}

			return n;
		}

		// Step ::= AxisSpecifier NodeTest Predicate* | AbbreviatedStep
		// AxisSpecifier ::= AxisName '::' | '@'?
		// NodeTest ::= NameTest | NodeType '(' ')' | 'processing-instruction' '(' Literal ')'
		// NameTest ::= '*' | NCName ':' '*' | QName
		// AbbreviatedStep ::= '.' | '..'
		xpath_ast_node* parse_step(xpath_ast_node* set)
		{
			if (set && set->rettype() != xpath_type_node_set)
				throw_error("Step has to be applied to node set");

			bool axis_specified = false;
			axis_t axis = axis_child; // implied child axis

			if (_lexer.current() == lex_axis_attribute)
			{
				axis = axis_attribute;
				axis_specified = true;

				_lexer.next();
			}
			else if (_lexer.current() == lex_dot)
			{
				_lexer.next();

				return new (alloc_node()) xpath_ast_node(ast_step, set, axis_self, nodetest_type_node, 0);
			}
			else if (_lexer.current() == lex_double_dot)
			{
				_lexer.next();

				return new (alloc_node()) xpath_ast_node(ast_step, set, axis_parent, nodetest_type_node, 0);
			}

			nodetest_t nt_type = nodetest_none;
			xpath_lexer_string nt_name;

			if (_lexer.current() == lex_string)
			{
				// node name test
				nt_name = _lexer.contents();
				_lexer.next();

				// was it an axis name?
				if (_lexer.current() == lex_double_colon)
				{
					// parse axis name
					if (axis_specified) throw_error("Two axis specifiers in one step");

					axis = parse_axis_name(nt_name, axis_specified);

					if (!axis_specified) throw_error("Unknown axis");

					// read actual node test
					_lexer.next();

					if (_lexer.current() == lex_multiply)
					{
						nt_type = nodetest_all;
						nt_name = xpath_lexer_string();
						_lexer.next();
					}
					else if (_lexer.current() == lex_string)
					{
						nt_name = _lexer.contents();
						_lexer.next();
					}
					else throw_error("Unrecognized node test");
				}

				if (nt_type == nodetest_none)
				{
					// node type test or processing-instruction
					if (_lexer.current() == lex_open_brace)
					{
						_lexer.next();

						if (_lexer.current() == lex_close_brace)
						{
							_lexer.next();

							nt_type = parse_node_test_type(nt_name);

							if (nt_type == nodetest_none) throw_error("Unrecognized node type");

							nt_name = xpath_lexer_string();
						}
						else if (nt_name == PUGIHTML_TEXT("processing-instruction"))
						{
							if (_lexer.current() != lex_quoted_string)
								throw_error("Only literals are allowed as arguments to processing-instruction()");

							nt_type = nodetest_pi;
							nt_name = _lexer.contents();
							_lexer.next();

							if (_lexer.current() != lex_close_brace)
								throw_error("Unmatched brace near processing-instruction()");
							_lexer.next();
						}
						else
							throw_error("Unmatched brace near node type test");

					}
					// QName or NCName:*
					else
					{
						if (nt_name.end - nt_name.begin > 2 && nt_name.end[-2] == ':' && nt_name.end[-1] == '*') // NCName:*
						{
							nt_name.end--; // erase *

							nt_type = nodetest_all_in_namespace;
						}
						else nt_type = nodetest_name;
					}
				}
			}
			else if (_lexer.current() == lex_multiply)
			{
				nt_type = nodetest_all;
				_lexer.next();
			}
			else throw_error("Unrecognized node test");

			xpath_ast_node* n = new (alloc_node()) xpath_ast_node(ast_step, set, axis, nt_type, alloc_string(nt_name));

			xpath_ast_node* last = 0;

			while (_lexer.current() == lex_open_square_brace)
			{
				_lexer.next();

				xpath_ast_node* expr = parse_expression();

				xpath_ast_node* pred = new (alloc_node()) xpath_ast_node(ast_predicate, xpath_type_node_set, expr);

				if (_lexer.current() != lex_close_square_brace)
					throw_error("Unmatched square brace");
				_lexer.next();

				if (last) last->set_next(pred);
				else n->set_right(pred);

				last = pred;
			}

			return n;
		}

		// RelativeLocationPath ::= Step | RelativeLocationPath '/' Step | RelativeLocationPath '//' Step
		xpath_ast_node* parse_relative_location_path(xpath_ast_node* set)
		{
			xpath_ast_node* n = parse_step(set);

			while (_lexer.current() == lex_slash || _lexer.current() == lex_double_slash)
			{
				lexeme_t l = _lexer.current();
				_lexer.next();

				if (l == lex_double_slash)
					n = new (alloc_node()) xpath_ast_node(ast_step, n, axis_descendant_or_self, nodetest_type_node, 0);

				n = parse_step(n);
			}

			return n;
		}

		// LocationPath ::= RelativeLocationPath | AbsoluteLocationPath
		// AbsoluteLocationPath ::= '/' RelativeLocationPath? | '//' RelativeLocationPath
		xpath_ast_node* parse_location_path()
		{
			if (_lexer.current() == lex_slash)
			{
				_lexer.next();

				xpath_ast_node* n = new (alloc_node()) xpath_ast_node(ast_step_root, xpath_type_node_set);

				// relative location path can start from axis_attribute, dot, double_dot, multiply and string lexemes; any other lexeme means standalone root path
				lexeme_t l = _lexer.current();

				if (l == lex_string || l == lex_axis_attribute || l == lex_dot || l == lex_double_dot || l == lex_multiply)
					return parse_relative_location_path(n);
				else
					return n;
			}
			else if (_lexer.current() == lex_double_slash)
			{
				_lexer.next();

				xpath_ast_node* n = new (alloc_node()) xpath_ast_node(ast_step_root, xpath_type_node_set);
				n = new (alloc_node()) xpath_ast_node(ast_step, n, axis_descendant_or_self, nodetest_type_node, 0);

				return parse_relative_location_path(n);
			}

			// else clause moved outside of if because of bogus warning 'control may reach end of non-void function being inlined' in gcc 4.0.1
			return parse_relative_location_path(0);
		}

		// PathExpr ::= LocationPath
		//				| FilterExpr
		//				| FilterExpr '/' RelativeLocationPath
		//				| FilterExpr '//' RelativeLocationPath
		xpath_ast_node* parse_path_expression()
		{
			// Clarification.
			// PathExpr begins with either LocationPath or FilterExpr.
			// FilterExpr begins with PrimaryExpr
			// PrimaryExpr begins with '$' in case of it being a variable reference,
			// '(' in case of it being an expression, string literal, number constant or
			// function call.

			if (_lexer.current() == lex_var_ref || _lexer.current() == lex_open_brace ||
				_lexer.current() == lex_quoted_string || _lexer.current() == lex_number ||
				_lexer.current() == lex_string)
			{
				if (_lexer.current() == lex_string)
				{
					// This is either a function call, or not - if not, we shall proceed with location path
					const char_t* state = _lexer.state();

					while (IS_CHARTYPE(*state, ct_space)) ++state;

					if (*state != '(') return parse_location_path();

					// This looks like a function call; however this still can be a node-test. Check it.
					if (parse_node_test_type(_lexer.contents()) != nodetest_none) return parse_location_path();
				}

				xpath_ast_node* n = parse_filter_expression();

				if (_lexer.current() == lex_slash || _lexer.current() == lex_double_slash)
				{
					lexeme_t l = _lexer.current();
					_lexer.next();

					if (l == lex_double_slash)
					{
						if (n->rettype() != xpath_type_node_set) throw_error("Step has to be applied to node set");

						n = new (alloc_node()) xpath_ast_node(ast_step, n, axis_descendant_or_self, nodetest_type_node, 0);
					}

					// select from location path
					return parse_relative_location_path(n);
				}

				return n;
			}
			else return parse_location_path();
		}

		// UnionExpr ::= PathExpr | UnionExpr '|' PathExpr
		xpath_ast_node* parse_union_expression()
		{
			xpath_ast_node* n = parse_path_expression();

			while (_lexer.current() == lex_union)
			{
				_lexer.next();

				xpath_ast_node* expr = parse_union_expression();

				if (n->rettype() != xpath_type_node_set || expr->rettype() != xpath_type_node_set)
					throw_error("Union operator has to be applied to node sets");

				n = new (alloc_node()) xpath_ast_node(ast_op_union, xpath_type_node_set, n, expr);
			}

			return n;
		}

		// UnaryExpr ::= UnionExpr | '-' UnaryExpr
		xpath_ast_node* parse_unary_expression()
		{
			if (_lexer.current() == lex_minus)
			{
				_lexer.next();

				xpath_ast_node* expr = parse_unary_expression();

				return new (alloc_node()) xpath_ast_node(ast_op_negate, xpath_type_number, expr);
			}
			else return parse_union_expression();
		}

		// MultiplicativeExpr ::= UnaryExpr
		//						  | MultiplicativeExpr '*' UnaryExpr
		//						  | MultiplicativeExpr 'div' UnaryExpr
		//						  | MultiplicativeExpr 'mod' UnaryExpr
		xpath_ast_node* parse_multiplicative_expression()
		{
			xpath_ast_node* n = parse_unary_expression();

			while (_lexer.current() == lex_multiply || (_lexer.current() == lex_string &&
				   (_lexer.contents() == PUGIHTML_TEXT("mod") || _lexer.contents() == PUGIHTML_TEXT("div"))))
			{
				ast_type_t op = _lexer.current() == lex_multiply ? ast_op_multiply :
					_lexer.contents().begin[0] == 'd' ? ast_op_divide : ast_op_mod;
				_lexer.next();

				xpath_ast_node* expr = parse_unary_expression();

				n = new (alloc_node()) xpath_ast_node(op, xpath_type_number, n, expr);
			}

			return n;
		}

		// AdditiveExpr ::= MultiplicativeExpr
		//					| AdditiveExpr '+' MultiplicativeExpr
		//					| AdditiveExpr '-' MultiplicativeExpr
		xpath_ast_node* parse_additive_expression()
		{
			xpath_ast_node* n = parse_multiplicative_expression();

			while (_lexer.current() == lex_plus || _lexer.current() == lex_minus)
			{
				lexeme_t l = _lexer.current();

				_lexer.next();

				xpath_ast_node* expr = parse_multiplicative_expression();

				n = new (alloc_node()) xpath_ast_node(l == lex_plus ? ast_op_add : ast_op_subtract, xpath_type_number, n, expr);
			}

			return n;
		}

		// RelationalExpr ::= AdditiveExpr
		//					  | RelationalExpr '<' AdditiveExpr
		//					  | RelationalExpr '>' AdditiveExpr
		//					  | RelationalExpr '<=' AdditiveExpr
		//					  | RelationalExpr '>=' AdditiveExpr
		xpath_ast_node* parse_relational_expression()
		{
			xpath_ast_node* n = parse_additive_expression();

			while (_lexer.current() == lex_less || _lexer.current() == lex_less_or_equal ||
				   _lexer.current() == lex_greater || _lexer.current() == lex_greater_or_equal)
			{
				lexeme_t l = _lexer.current();
				_lexer.next();

				xpath_ast_node* expr = parse_additive_expression();

				n = new (alloc_node()) xpath_ast_node(l == lex_less ? ast_op_less : l == lex_greater ? ast_op_greater :
								l == lex_less_or_equal ? ast_op_less_or_equal : ast_op_greater_or_equal, xpath_type_boolean, n, expr);
			}

			return n;
		}

		// EqualityExpr ::= RelationalExpr
		//					| EqualityExpr '=' RelationalExpr
		//					| EqualityExpr '!=' RelationalExpr
		xpath_ast_node* parse_equality_expression()
		{
			xpath_ast_node* n = parse_relational_expression();

			while (_lexer.current() == lex_equal || _lexer.current() == lex_not_equal)
			{
				lexeme_t l = _lexer.current();

				_lexer.next();

				xpath_ast_node* expr = parse_relational_expression();

				n = new (alloc_node()) xpath_ast_node(l == lex_equal ? ast_op_equal : ast_op_not_equal, xpath_type_boolean, n, expr);
			}

			return n;
		}

		// AndExpr ::= EqualityExpr | AndExpr 'and' EqualityExpr
		xpath_ast_node* parse_and_expression()
		{
			xpath_ast_node* n = parse_equality_expression();

			while (_lexer.current() == lex_string && _lexer.contents() == PUGIHTML_TEXT("and"))
			{
				_lexer.next();

				xpath_ast_node* expr = parse_equality_expression();

				n = new (alloc_node()) xpath_ast_node(ast_op_and, xpath_type_boolean, n, expr);
			}

			return n;
		}

		// OrExpr ::= AndExpr | OrExpr 'or' AndExpr
		xpath_ast_node* parse_or_expression()
		{
			xpath_ast_node* n = parse_and_expression();

			while (_lexer.current() == lex_string && _lexer.contents() == PUGIHTML_TEXT("or"))
			{
				_lexer.next();

				xpath_ast_node* expr = parse_and_expression();

				n = new (alloc_node()) xpath_ast_node(ast_op_or, xpath_type_boolean, n, expr);
			}

			return n;
		}

		// Expr ::= OrExpr
		xpath_ast_node* parse_expression()
		{
			return parse_or_expression();
		}

		xpath_parser(const char_t* query, xpath_variable_set* variables, xpath_allocator* alloc, xpath_parse_result* result): _alloc(alloc), _lexer(query), _query(query), _variables(variables), _result(result)
		{
		}

		xpath_ast_node* parse()
		{
			xpath_ast_node* result = parse_expression();

			if (_lexer.current() != lex_eof)
			{
				// there are still unparsed tokens left, error
				throw_error("Incorrect query");
			}

			return result;
		}

		static xpath_ast_node* parse(const char_t* query, xpath_variable_set* variables, xpath_allocator* alloc, xpath_parse_result* result)
		{
			xpath_parser parser(query, variables, alloc, result);

		#ifdef PUGIHTML_NO_EXCEPTIONS
			int error = setjmp(parser._error_handler);

			return (error == 0) ? parser.parse() : 0;
		#else
			return parser.parse();
		#endif
		}
	};

	struct xpath_query_impl
	{
		static xpath_query_impl* create()
		{
			void* memory = global_allocate(sizeof(xpath_query_impl));

			return new (memory) xpath_query_impl();
		}

		static void destroy(void* ptr)
		{
			if (!ptr) return;

			// free all allocated pages
			static_cast<xpath_query_impl*>(ptr)->alloc.release();

			// free allocator memory (with the first page)
			global_deallocate(ptr);
		}

		xpath_query_impl(): root(0), alloc(&block)
		{
			block.next = 0;
		}

		xpath_ast_node* root;
		xpath_allocator alloc;
		xpath_memory_block block;
	};

	xpath_string evaluate_string_impl(xpath_query_impl* impl, const xpath_node& n, xpath_stack_data& sd)
	{
		if (!impl) return xpath_string();

	#ifdef PUGIHTML_NO_EXCEPTIONS
		if (setjmp(sd.error_handler)) return xpath_string();
	#endif

		xpath_context c(n, 1, 1);

		return impl->root->eval_string(c, sd.stack);
	}
}

namespace pugihtml
{
#ifndef PUGIHTML_NO_EXCEPTIONS
	xpath_exception::xpath_exception(const xpath_parse_result& result): _result(result)
	{
		assert(result.error);
	}

	const char* xpath_exception::what() const throw()
	{
		return _result.error;
	}

	const xpath_parse_result& xpath_exception::result() const
	{
		return _result;
	}
#endif

	xpath_node::xpath_node()
	{
	}

	xpath_node::xpath_node(const node& node): _node(node)
	{
	}

	xpath_node::xpath_node(const attribute& attribute, const node& parent): _node(attribute ? parent : node()), _attribute(attribute)
	{
	}

	node xpath_node::node() const
	{
		return _attribute ? node() : _node;
	}

	attribute xpath_node::attribute() const
	{
		return _attribute;
	}

	node xpath_node::parent() const
	{
		return _attribute ? _node : _node.parent();
	}

	xpath_node::operator xpath_node::unspecified_bool_type() const
	{
		return (_node || _attribute) ? &xpath_node::_node : 0;
	}

	bool xpath_node::operator!() const
	{
		return !(_node || _attribute);
	}

	bool xpath_node::operator==(const xpath_node& n) const
	{
		return _node == n._node && _attribute == n._attribute;
	}

	bool xpath_node::operator!=(const xpath_node& n) const
	{
		return _node != n._node || _attribute != n._attribute;
	}

#ifdef __BORLANDC__
	bool operator&&(const xpath_node& lhs, bool rhs)
	{
		return (bool)lhs && rhs;
	}

	bool operator||(const xpath_node& lhs, bool rhs)
	{
		return (bool)lhs || rhs;
	}
#endif

	void xpath_node_set::_assign(const_iterator begin, const_iterator end)
	{
		assert(begin <= end);

		size_t size = static_cast<size_t>(end - begin);

		if (size <= 1)
		{
			// deallocate old buffer
			if (_begin != &_storage) global_deallocate(_begin);

			// use internal buffer
			if (begin != end) _storage = *begin;

			_begin = &_storage;
			_end = &_storage + size;
		}
		else
		{
			// make heap copy
			xpath_node* storage = static_cast<xpath_node*>(global_allocate(size * sizeof(xpath_node)));

			if (!storage)
			{
			#ifdef PUGIHTML_NO_EXCEPTIONS
				return;
			#else
				throw std::bad_alloc();
			#endif
			}

			memcpy(storage, begin, size * sizeof(xpath_node));

			// deallocate old buffer
			if (_begin != &_storage) global_deallocate(_begin);

			// finalize
			_begin = storage;
			_end = storage + size;
		}
	}

	xpath_node_set::xpath_node_set(): _type(type_unsorted), _begin(&_storage), _end(&_storage)
	{
	}

	xpath_node_set::xpath_node_set(const_iterator begin, const_iterator end, type_t type): _type(type), _begin(&_storage), _end(&_storage)
	{
		_assign(begin, end);
	}

	xpath_node_set::~xpath_node_set()
	{
		if (_begin != &_storage) global_deallocate(_begin);
	}

	xpath_node_set::xpath_node_set(const xpath_node_set& ns): _type(ns._type), _begin(&_storage), _end(&_storage)
	{
		_assign(ns._begin, ns._end);
	}

	xpath_node_set& xpath_node_set::operator=(const xpath_node_set& ns)
	{
		if (this == &ns) return *this;

		_type = ns._type;
		_assign(ns._begin, ns._end);

		return *this;
	}

	xpath_node_set::type_t xpath_node_set::type() const
	{
		return _type;
	}

	size_t xpath_node_set::size() const
	{
		return _end - _begin;
	}

	bool xpath_node_set::empty() const
	{
		return _begin == _end;
	}

	const xpath_node& xpath_node_set::operator[](size_t index) const
	{
		assert(index < size());
		return _begin[index];
	}

	xpath_node_set::const_iterator xpath_node_set::begin() const
	{
		return _begin;
	}

	xpath_node_set::const_iterator xpath_node_set::end() const
	{
		return _end;
	}

	void xpath_node_set::sort(bool reverse)
	{
		_type = xpath_sort(_begin, _end, _type, reverse);
	}

	xpath_node xpath_node_set::first() const
	{
		return xpath_first(_begin, _end, _type);
	}

	xpath_parse_result::xpath_parse_result(): error("Internal error"), offset(0)
	{
	}

	xpath_parse_result::operator bool() const
	{
		return error == 0;
	}
	const char* xpath_parse_result::description() const
	{
		return error ? error : "No error";
	}

	xpath_variable::xpath_variable()
	{
	}

	const char_t* xpath_variable::name() const
	{
		switch (_type)
		{
		case xpath_type_node_set:
			return static_cast<const xpath_variable_node_set*>(this)->name;

		case xpath_type_number:
			return static_cast<const xpath_variable_number*>(this)->name;

		case xpath_type_string:
			return static_cast<const xpath_variable_string*>(this)->name;

		case xpath_type_boolean:
			return static_cast<const xpath_variable_boolean*>(this)->name;

		default:
			assert(!"Invalid variable type");
			return 0;
		}
	}

	xpath_value_type xpath_variable::type() const
	{
		return _type;
	}

	bool xpath_variable::get_boolean() const
	{
		return (_type == xpath_type_boolean) ? static_cast<const xpath_variable_boolean*>(this)->value : false;
	}

	double xpath_variable::get_number() const
	{
		return (_type == xpath_type_number) ? static_cast<const xpath_variable_number*>(this)->value : gen_nan();
	}

	const char_t* xpath_variable::get_string() const
	{
		const char_t* value = (_type == xpath_type_string) ? static_cast<const xpath_variable_string*>(this)->value : 0;
		return value ? value : PUGIHTML_TEXT("");
	}

	const xpath_node_set& xpath_variable::get_node_set() const
	{
		return (_type == xpath_type_node_set) ? static_cast<const xpath_variable_node_set*>(this)->value : dummy_node_set;
	}

	bool xpath_variable::set(bool value)
	{
		if (_type != xpath_type_boolean) return false;

		static_cast<xpath_variable_boolean*>(this)->value = value;
		return true;
	}

	bool xpath_variable::set(double value)
	{
		if (_type != xpath_type_number) return false;

		static_cast<xpath_variable_number*>(this)->value = value;
		return true;
	}

	bool xpath_variable::set(const char_t* value)
	{
		if (_type != xpath_type_string) return false;

		xpath_variable_string* var = static_cast<xpath_variable_string*>(this);

		// duplicate string
		size_t size = (strlength(value) + 1) * sizeof(char_t);

		char_t* copy = static_cast<char_t*>(global_allocate(size));
		if (!copy) return false;

		memcpy(copy, value, size);

		// replace old string
		if (var->value) global_deallocate(var->value);
		var->value = copy;

		return true;
	}

	bool xpath_variable::set(const xpath_node_set& value)
	{
		if (_type != xpath_type_node_set) return false;

		static_cast<xpath_variable_node_set*>(this)->value = value;
		return true;
	}

	xpath_variable_set::xpath_variable_set()
	{
		for (size_t i = 0; i < sizeof(_data) / sizeof(_data[0]); ++i) _data[i] = 0;
	}

	xpath_variable_set::~xpath_variable_set()
	{
		for (size_t i = 0; i < sizeof(_data) / sizeof(_data[0]); ++i)
		{
			xpath_variable* var = _data[i];

			while (var)
			{
				xpath_variable* next = var->_next;

				delete_xpath_variable(var->_type, var);

				var = next;
			}
		}
	}

	xpath_variable* xpath_variable_set::find(const char_t* name) const
	{
		const size_t hash_size = sizeof(_data) / sizeof(_data[0]);
		size_t hash = hash_string(name) % hash_size;

		// look for existing variable
		for (xpath_variable* var = _data[hash]; var; var = var->_next)
			if (strequal(var->name(), name))
				return var;

		return 0;
	}

	xpath_variable* xpath_variable_set::add(const char_t* name, xpath_value_type type)
	{
		const size_t hash_size = sizeof(_data) / sizeof(_data[0]);
		size_t hash = hash_string(name) % hash_size;

		// look for existing variable
		for (xpath_variable* var = _data[hash]; var; var = var->_next)
			if (strequal(var->name(), name))
				return var->type() == type ? var : 0;

		// add new variable
		xpath_variable* result = new_xpath_variable(type, name);

		if (result)
		{
			result->_type = type;
			result->_next = _data[hash];

			_data[hash] = result;
		}

		return result;
	}

	bool xpath_variable_set::set(const char_t* name, bool value)
	{
		xpath_variable* var = add(name, xpath_type_boolean);
		return var ? var->set(value) : false;
	}

	bool xpath_variable_set::set(const char_t* name, double value)
	{
		xpath_variable* var = add(name, xpath_type_number);
		return var ? var->set(value) : false;
	}

	bool xpath_variable_set::set(const char_t* name, const char_t* value)
	{
		xpath_variable* var = add(name, xpath_type_string);
		return var ? var->set(value) : false;
	}

	bool xpath_variable_set::set(const char_t* name, const xpath_node_set& value)
	{
		xpath_variable* var = add(name, xpath_type_node_set);
		return var ? var->set(value) : false;
	}

	xpath_variable* xpath_variable_set::get(const char_t* name)
	{
		return find(name);
	}

	const xpath_variable* xpath_variable_set::get(const char_t* name) const
	{
		return find(name);
	}

	xpath_query::xpath_query(const char_t* query, xpath_variable_set* variables): _impl(0)
	{
		xpath_query_impl* impl = xpath_query_impl::create();

		if (!impl)
		{
		#ifdef PUGIHTML_NO_EXCEPTIONS
			_result.error = "Out of memory";
		#else
			throw std::bad_alloc();
		#endif
		}
		else
		{
			buffer_holder impl_holder(impl, xpath_query_impl::destroy);

			impl->root = xpath_parser::parse(query, variables, &impl->alloc, &_result);

			if (impl->root)
			{
				_impl = static_cast<xpath_query_impl*>(impl_holder.release());
				_result.error = 0;
			}
		}
	}

	xpath_query::~xpath_query()
	{
		xpath_query_impl::destroy(_impl);
	}

	xpath_value_type xpath_query::return_type() const
	{
		if (!_impl) return xpath_type_none;

		return static_cast<xpath_query_impl*>(_impl)->root->rettype();
	}

	bool xpath_query::evaluate_boolean(const xpath_node& n) const
	{
		if (!_impl) return false;

		xpath_context c(n, 1, 1);
		xpath_stack_data sd;

	#ifdef PUGIHTML_NO_EXCEPTIONS
		if (setjmp(sd.error_handler)) return false;
	#endif

		return static_cast<xpath_query_impl*>(_impl)->root->eval_boolean(c, sd.stack);
	}

	double xpath_query::evaluate_number(const xpath_node& n) const
	{
		if (!_impl) return gen_nan();

		xpath_context c(n, 1, 1);
		xpath_stack_data sd;

	#ifdef PUGIHTML_NO_EXCEPTIONS
		if (setjmp(sd.error_handler)) return gen_nan();
	#endif

		return static_cast<xpath_query_impl*>(_impl)->root->eval_number(c, sd.stack);
	}

#ifndef PUGIHTML_NO_STL
	string_t xpath_query::evaluate_string(const xpath_node& n) const
	{
		xpath_stack_data sd;

		return evaluate_string_impl(static_cast<xpath_query_impl*>(_impl), n, sd).c_str();
	}
#endif

	size_t xpath_query::evaluate_string(char_t* buffer, size_t capacity, const xpath_node& n) const
	{
		xpath_stack_data sd;

		xpath_string r = evaluate_string_impl(static_cast<xpath_query_impl*>(_impl), n, sd);

		size_t full_size = r.length() + 1;

		if (capacity > 0)
		{
			size_t size = (full_size < capacity) ? full_size : capacity;
			assert(size > 0);

			memcpy(buffer, r.c_str(), (size - 1) * sizeof(char_t));
			buffer[size - 1] = 0;
		}

		return full_size;
	}

	xpath_node_set xpath_query::evaluate_node_set(const xpath_node& n) const
	{
		if (!_impl) return xpath_node_set();

		xpath_ast_node* root = static_cast<xpath_query_impl*>(_impl)->root;

		if (root->rettype() != xpath_type_node_set)
		{
		#ifdef PUGIHTML_NO_EXCEPTIONS
			return xpath_node_set();
		#else
			xpath_parse_result result;
			result.error = "Expression does not evaluate to node set";

			throw xpath_exception(result);
		#endif
		}

		xpath_context c(n, 1, 1);
		xpath_stack_data sd;

	#ifdef PUGIHTML_NO_EXCEPTIONS
		if (setjmp(sd.error_handler)) return xpath_node_set();
	#endif

		xpath_node_set_raw r = root->eval_node_set(c, sd.stack);

		return xpath_node_set(r.begin(), r.end(), r.type());
	}

	const xpath_parse_result& xpath_query::result() const
	{
		return _result;
	}

	xpath_query::operator xpath_query::unspecified_bool_type() const
	{
		return _impl ? &xpath_query::_impl : 0;
	}

	bool xpath_query::operator!() const
	{
		return !_impl;
	}

	xpath_node node::select_single_node(const char_t* query, xpath_variable_set* variables) const
	{
		xpath_query q(query, variables);
		return select_single_node(q);
	}

	xpath_node node::select_single_node(const xpath_query& query) const
	{
		xpath_node_set s = query.evaluate_node_set(*this);
		return s.empty() ? xpath_node() : s.first();
	}

	xpath_node_set node::select_nodes(const char_t* query, xpath_variable_set* variables) const
	{
		xpath_query q(query, variables);
		return select_nodes(q);
	}

	xpath_node_set node::select_nodes(const xpath_query& query) const
	{
		return query.evaluate_node_set(*this);
	}
}

#endif
