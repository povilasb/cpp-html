#include <cstddef>
#include <cassert>
#include <cstring>

#include "pugiutil.hpp"
#include "common.hpp"
#include "html_parser.hpp"


using namespace pugihtml;


gap::gap(): end(nullptr), size(0)
{
}

void
gap::push(char_t*& s, size_t count)
{
	// there was a gap already; collapse it
	if (end)  {
		// Move [old_gap_end, new_gap_start) to [old_gap_start, ...)
		assert(s >= end);
		memmove(end - size, end, reinterpret_cast<char*>(s)
			- reinterpret_cast<char*>(end));
	}

	s += count; // end of current gap

	// "merge" two gaps
	end = s;
	size += count;
}


char_t*
gap::flush(char_t* s)
{
	if (end) {
		// Move [old_gap_end, current_pos) to [old_gap_start, ...)
		assert(s >= end);
		memmove(end - size, end, reinterpret_cast<char*>(s)
			- reinterpret_cast<char*>(end));

		return s - size;
	}
	else {
		return s;
	}
}


// Utility macro for last character handling
#define ENDSWITH(c, e) ((c) == (e) || ((c) == 0 && endch == (e)))

char_t*
pugihtml::strconv_comment(char_t* s, char_t endch)
{
	gap g;

	while (true) {
		while (!is_chartype(*s, ct_parse_comment)) ++s;

		// Either a single 0x0d or 0x0d 0x0a pair
		if (*s == '\r')  {
			*s++ = '\n'; // replace first one with 0x0a

			if (*s == '\n') g.push(s, 1);
		}
		// comment ends here
		else if (s[0] == '-' && s[1] == '-' && ENDSWITH(s[2], '>')) {
			*g.flush(s) = 0;

			return s + (s[2] == '>' ? 3 : 2);
		}
		else if (*s == 0) {
			return 0;
		}
		else {
			++s;
		}
	}
}


char_t*
pugihtml::strconv_cdata(char_t* s, char_t endch)
{
	gap g;

	while (true) {
		while (!is_chartype(*s, ct_parse_cdata)) {
			++s;
		}

		// Either a single 0x0d or 0x0d 0x0a pair
		if (*s == '\r') {
			*s++ = '\n'; // replace first one with 0x0a

			if (*s == '\n') {
				g.push(s, 1);
			}
		}
		// CDATA ends here
		else if (s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>')) {
			*g.flush(s) = 0;

			return s + 1;
		}
		else if (*s == 0) {
			return 0;
		}
		else {
			++s;
		}
	}
}


struct utf8_writer {
	typedef uint8_t* value_type;

	static value_type low(value_type result, uint32_t ch)
	{
		// U+0000..U+007F
		if (ch < 0x80)
		{
			*result = static_cast<uint8_t>(ch);
			return result + 1;
		}
		// U+0080..U+07FF
		else if (ch < 0x800)
		{
			result[0] = static_cast<uint8_t>(0xC0 | (ch >> 6));
			result[1] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 2;
		}
		// U+0800..U+FFFF
		else
		{
			result[0] = static_cast<uint8_t>(0xE0 | (ch >> 12));
			result[1] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
			result[2] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 3;
		}
	}

	static value_type high(value_type result, uint32_t ch)
	{
		// U+10000..U+10FFFF
		result[0] = static_cast<uint8_t>(0xF0 | (ch >> 18));
		result[1] = static_cast<uint8_t>(0x80 | ((ch >> 12) & 0x3F));
		result[2] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
		result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
		return result + 4;
	}

	static value_type any(value_type result, uint32_t ch)
	{
		return (ch < 0x10000) ? low(result, ch) : high(result, ch);
	}
};


char_t*
pugihtml::strconv_escape(char_t* s, gap& g)
{
	char_t* stre = s + 1;

	switch (*stre)
	{
		case '#':	// &#...
		{
			unsigned int ucsc = 0;

			if (stre[1] == 'x') // &#x... (hex code)
			{
				stre += 2;

				char_t ch = *stre;

				if (ch == ';') return stre;

				for (;;)
				{
					if (static_cast<unsigned int>(ch - '0') <= 9)
						ucsc = 16 * ucsc + (ch - '0');
					else if (static_cast<unsigned int>((ch | ' ') - 'a') <= 5)
						ucsc = 16 * ucsc + ((ch | ' ') - 'a' + 10);
					else if (ch == ';')
						break;
					else // cancel
						return stre;

					ch = *++stre;
				}

				++stre;
			}
			else	// &#... (dec code)
			{
				char_t ch = *++stre;

				if (ch == ';') return stre;

				for (;;)
				{
					if (static_cast<unsigned int>(ch - '0') <= 9)
						ucsc = 10 * ucsc + (ch - '0');
					else if (ch == ';')
						break;
					else // cancel
						return stre;

					ch = *++stre;
				}

				++stre;
			}

		#ifdef PUGIHTML_WCHAR_MODE
			s = reinterpret_cast<char_t*>(wchar_writer::any(reinterpret_cast<wchar_writer::value_type>(s), ucsc));
		#else
			s = reinterpret_cast<char_t*>(utf8_writer::any(reinterpret_cast<uint8_t*>(s), ucsc));
		#endif

			g.push(s, stre - s);
			return stre;
		}
		case 'a':	// &a
		{
			++stre;

			if (*stre == 'm') // &am
			{
				if (*++stre == 'p' && *++stre == ';') // &amp;
				{
					*s++ = '&';
					++stre;

					g.push(s, stre - s);
					return stre;
				}
			}
			else if (*stre == 'p') // &ap
			{
				if (*++stre == 'o' && *++stre == 's' && *++stre == ';') // &apos;
				{
					*s++ = '\'';
					++stre;

					g.push(s, stre - s);
					return stre;
				}
			}
			break;
		}
		case 'g': // &g
		{
			if (*++stre == 't' && *++stre == ';') // &gt;
			{
				*s++ = '>';
				++stre;

				g.push(s, stre - s);
				return stre;
			}
			break;
		}
		case 'l': // &l
		{
			if (*++stre == 't' && *++stre == ';') // &lt;
			{
				*s++ = '<';
				++stre;

				g.push(s, stre - s);
				return stre;
			}
			break;
		}
		case 'q': // &q
		{
			if (*++stre == 'u' && *++stre == 'o' && *++stre == 't' && *++stre == ';') // &quot;
			{
				*s++ = '"';
				++stre;

				g.push(s, stre - s);
				return stre;
			}
			break;
		}
	}

	return stre;
}


/**
 * This table maps ASCII symbols with their possible types in enum chartype_t.
 */
const unsigned char chartype_table[256] =
{
	55,  0,   0,   0,   0,   0,   0,   0,      0,   12,  12,  0,   0,   63,  0,   0,   // 0-15
	0,   0,   0,   0,   0,   0,   0,   0,      0,   0,   0,   0,   0,   0,   0,   0,   // 16-31
	8,   0,   6,   0,   0,   0,   7,   6,      0,   0,   0,   0,   0,   96,  64,  0,   // 32-47
	64,  64,  64,  64,  64,  64,  64,  64,     64,  64,  192, 0,   1,   0,   48,  0,   // 48-63
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


bool
pugihtml::is_chartype(char_t ch, enum chartype_t char_type)
{
#ifdef PUGIHTML_WCHAR_MODE
	return (static_cast<unsigned int>(ch) < 128
		? chartype_table[static_cast<unsigned int>(ch)]
		: chartype_table[128]) & (char_type);
#else
	return chartype_table[static_cast<unsigned char>(ch)] & (char_type);
#endif
}


inline bool
strcpy_insitu_allow(size_t length, uintptr_t allocated, char_t* target)
{
	assert(target);
	size_t target_length = strlength(target);

	// always reuse document buffer memory if possible
	if (!allocated) return target_length >= length;

	// reuse heap memory if waste is not too great
	const size_t reuse_threshold = 32;

	return target_length >= length && (target_length < reuse_threshold
		|| target_length - length < target_length / 2);
}

bool
pugihtml::strcpy_insitu(char_t*& dest, uintptr_t& header, uintptr_t header_mask,
	const char_t* source)
{
	size_t source_length = strlength(source);

	if (source_length == 0) {
		// empty string and null pointer are equivalent, so just deallocate old memory
		html_allocator* alloc = reinterpret_cast<html_memory_page*>(
			header & html_memory_page_pointer_mask)->allocator;

		if (header & header_mask) alloc->deallocate_string(dest);

		// mark the string as not allocated
		dest = 0;
		header &= ~header_mask;

		return true;
	}
	else if (dest && strcpy_insitu_allow(source_length, header
		& header_mask, dest)) {
		// we can reuse old buffer, so just copy the new data (including zero terminator)
		memcpy(dest, source, (source_length + 1) * sizeof(char_t));

		return true;
	}
	else {
		html_allocator* alloc = reinterpret_cast<html_memory_page*>(
			header & html_memory_page_pointer_mask)->allocator;

		// allocate new buffer
		char_t* buf = alloc->allocate_string(source_length + 1);
		if (!buf) return false;

		// copy the string (including zero terminator)
		memcpy(buf, source, (source_length + 1) * sizeof(char_t));

		// deallocate old buffer (*after* the above to protect
		// against overlapping memory and/or allocation failures)
		if (header & header_mask) alloc->deallocate_string(dest);

		// the string is now allocated, so set the flag
		dest = buf;
		header |= header_mask;

		return true;
	}
}


// Get string length
size_t
pugihtml::strlength(const char_t* s)
{
	assert(s);

#ifdef PUGIHTML_WCHAR_MODE
	return wcslen(s);
#else
	return strlen(s);
#endif
}
