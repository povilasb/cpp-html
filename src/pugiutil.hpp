#ifndef PUGIHTML_UTIL_HPP
#define PUGIHMTL_UTIL_HPP 1

#include <pugihtml/parser.hpp>

#include "common.hpp"


// TODO(povilas): rename to util.hpp.


namespace pugihtml
{

//#define ARRAYSIZE(ar)  (sizeof(ar) / sizeof(ar[0]))
#define TOUPPER(X){ if((X) >= 'a' && (X) <= 'z') {(X) -= ('a' - 'A');} }

// Simple static assertion
#define STATIC_ASSERT(cond) { \
	static const char condition_failed[(cond) ? 1 : -1] = {0}; \
	(void)condition_failed[0]; }


static inline void
to_upper(char_t* str)
{
	while(*str != 0) {
		TOUPPER(*str);
		str++;
	}
}


inline uint16_t
endian_swap(uint16_t value)
{
	return static_cast<uint16_t>(((value & 0xff) << 8) | (value >> 8));
}


inline uint32_t
endian_swap(uint32_t value)
{
	return ((value & 0xff) << 24) | ((value & 0xff00) << 8)
		| ((value & 0xff0000) >> 8) | (value >> 24);
}


template <typename T> inline void
convert_utf_endian_swap(T* result, const T* data, size_t length)
{
	for (size_t i = 0; i < length; ++i) {
		result[i] = endian_swap(data[i]);
	}
}


class gap {
public:
	gap();

	/**
	 * Push new gap, move s count bytes further (skipping the gap).
	 * Collapse previous gap.
	 */
	void push(char_t*& s, size_t count);

	/**
	 * Collapse all gaps, return past-the-end pointer
	 */
	char_t* flush(char_t* s);

private:
	char_t* end;
	size_t size;
};


/**
 * Normalizes EOL \r\n chars to single char \n and returns pointer
 * to the resulting string.
 * NOTE: original string is also altered.
 */
char_t* strconv_comment(char_t* s, char_t endch);

/**
 * Normalizes EOL \r\n chars to single char \n and returns pointer
 * to the resulting string.
 * NOTE: original string is also altered.
 */
char_t* strconv_cdata(char_t* s, char_t endch);

char_t* strconv_escape(char_t* s, gap& g);


/**
 * Checks if the specified char is of the specified char type.
 */
bool is_chartype(char_t ch, enum chartype_t char_type);


// Helper classes for code generation
struct opt_false {
	enum { value = 0 };
};

struct opt_true {
	enum { value = 1 };
};


bool strcpy_insitu(char_t*& dest, uintptr_t& header, uintptr_t header_mask,
	const char_t* source);

/**
 * Get string length.
 */
size_t strlength(const char_t* s);


// Compare two strings
bool strequal(const char_t* src, const char_t* dst);


// Compare lhs with [rhs_begin, rhs_end)
bool strequalrange(const char_t* lhs, const char_t* rhs, size_t count);


#if !defined(PUGIHTML_NO_STL) || !defined(PUGIHTML_NO_XPATH)
// auto_ptr-like buffer holder for exception recovery
struct buffer_holder {
	void* data;
	void (*deleter)(void*);

	buffer_holder(void* data, void (*deleter)(void*)) : data(data),
		deleter(deleter)
	{
	}

	~buffer_holder()
	{
		if (data) {
			deleter(data);
		}
	}

	void* release()
	{
		void* result = data;
		data = 0;
		return result;
	}
};
#endif

parse_status get_file_size(FILE* file, size_t& out_result);


#ifndef PUGIHTML_NO_STL
// Convert wide string to UTF8
std::basic_string<char, std::char_traits<char>, std::allocator<char> > PUGIHTML_FUNCTION as_utf8(const wchar_t* str);
std::basic_string<char, std::char_traits<char>, std::allocator<char> > PUGIHTML_FUNCTION as_utf8(const std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >& str);
#endif

//static char_t* attributes[] = {"ABBR", "ACCEPT", "ACCEPT-CHARSET",
//    "ACCESSKEY", "ACTION", "ALIGN", "ALINK", "ALT", "ARCHIVE",
//    "AXIS", "BACKGROUND", "BGCOLOR", "BORDER", "CELLPADDING",
//    "CELLSPACING", "CHAR", "CHAROFF", "CHARSET", "CHECKED", "CITE",
//    "CLASS", "CLASSID", "CLEAR", "CODE", "CODEBASE", "CODETYPE",
//    "COLOR", "COLS", "COLSPAN", "COMPACT", "CONTENT", "COORDS",
//    "DATA", "DATETIME", "DECLARE", "DEFER", "DIR", "DISABLED",
//    "ENCTYPE", "FACE", "FOR", "FRAME", "FRAMEBORDER", "HEADERS",
//    "HEIGHT", "HREF", "HREFLANG", "HSPACE", "HTTP-EQUIV", "ID",
//    "ISMAP", "LABEL", "LANG", "LANGUAGE", "LINK", "LONGDESC",
//    "MARGINHEIGHT", "MARGINWIDTH", "MAXLENGTH", "MEDIA", "METHOD",
//    "MULTIPLE", "NAME", "NOHREF", "NORESIZE", "NOSHADE", "NOWRAP",
//    "OBJECT", "ONBLUR", "ONCHANGE", "ONCLICK", "ONDBLCLICK",
//    "ONFOCUS", "ONKEYDOWN", "ONKEYPRESS", "ONKEYUP", "ONLOAD",
//    "ONMOUSEDOWN", "ONMOUSEMOVE", "ONMOUSEOUT", "ONMOUSEOVER",
//    "ONMOUSEUP", "ONRESET", "ONSELECT", "ONSUBMIT", "ONUNLOAD",
//    "PROFILE", "PROMPT", "READONLY", "REL", "REV", "ROWS", "ROWSPAN",
//    "RULES", "SCHEME", "SCOPE", "SCROLLING", "SELECTED", "SHAPE",
//    "SIZE", "SPAN", "SRC", "STANDBY", "START", "STYLE", "SUMMARY",
//    "TABINDEX", "TARGET", "TEXT", "TITLE", "TYPE", "USEMAP", "VALIGN",
//    "VALUE", "VALUETYPE", "VERSION", "VLINK", "VSPACE", "WIDTH"};

//static char_t* elements[] = {"!DOCTYPE", "A", "ABBR", "ACRONYM", "ADDRESS",
//    "APPLET", "AREA", "ARTICLE", "ASIDE", "AUDIO", "B", "BASE",
//    "BASEFONT", "BDI", "BDO", "BIG", "BLOCKQUOTE", "BODY", "BR",
//    "BUTTON", "CANVAS", "CAPTION", "CENTER", "CITE", "CODE", "COL",
//    "COLGROUP", "COMMAND", "DATALIST", "DD", "DEL", "DETAILS",
//    "DFN", "DIR", "DIV", "DL", "DT", "EM", "EMBED", "FIELDSET",
//    "FIGCAPTION", "FIGURE", "FONT", "FOOTER", "FORM", "FRAME",
//    "FRAMESET", "H1> TO <H6", "HEAD", "HEADER", "HGROUP", "HR",
//    "HTML", "I", "IFRAME", "IMG", "INPUT", "INS", "KEYGEN", "KBD",
//    "LABEL", "LEGEND", "LI", "LINK", "MAP", "MARK", "MENU", "META",
//    "METER", "NAV", "NOFRAMES", "NOSCRIPT", "OBJECT", "OL", "OPTGROUP",
//    "OPTION", "OUTPUT", "P", "PARAM", "PRE", "PROGRESS", "Q", "RP",
//    "RT", "RUBY", "S", "SAMP", "SCRIPT", "SECTION", "SELECT", "SMALL",
//    "SOURCE", "SPAN", "STRIKE", "STRONG", "STYLE", "SUB", "SUMMARY",
//    "SUP", "TABLE", "TBODY", "TD", "TEXTAREA", "TFOOT", "TH", "THEAD",
//    "TIME", "TITLE", "TR", "TRACK", "TT", "U", "UL", "VAR", "VIDEO", "WBR"};
//
//
///// Comparator optimized for speed
//struct TagSetComparator
//{
//    ///// A specialized comparison of two strings: the first string
//    ///// should always be upper case (since we're using this in the
//    ///// html tag and attribute sets).
//    //static inline int strcasecmp(const char_t *s1, const char_t *s2)
//    //{
//    //    char_t c1, c2;
//    //    do
//    //    {
//    //        c1 = *s1++;
//    //        c2 = *s2++;
//    //        TOUPPER(c1);
//    //        TOUPPER(c2);
//    //    } while((c1 == c2) && (s1 != '\0'));
//    //    return (int) c1-c2;
//    //}

//    inline bool operator()(const char_t *s1, const char_t *s2) const
//    {
//        return strcmp(s1, s2) < 0;
//    }
//};
//
///// HTML5 attributes
//static const std::set<char_t*, TagSetComparator> attributes(attributes, attributes+ARRAYSIZE(attributes));
//
///// HTML5 elements
//static const std::set<char_t*, TagSetComparator> html_elements(elements, elements+ARRAYSIZE(elements));

///// Tag normalization involves the capitalization of the tag if it
///// was found in the given tag set.
/////
///// tag is the tag which should be normalized
/////
///// tmp is a the temporary character array has to be large enough
///// to store valid tags and the number of characters in the tag
///// should not surpass the size of the temporary character array.
/////
///// tagSet is the set of tags which the given tag should be
///// checked against.
//static inline void normalize(char_t* tag, char_t* tmp, const std::set<char_t*, TagSetComparator>& tagSet )
//{
//    strcpy(tmp, tag);
//    to_upper(tmp);
//    if(tagSet.find(tmp) != tagSet.end())
//    {
//        strcpy(tag, tmp);
//    }
//    memset(tmp, 0, strlen(tmp));
//}

} // pugihtml.

#endif /* PUGIHMTL_UTIL_HPP */
