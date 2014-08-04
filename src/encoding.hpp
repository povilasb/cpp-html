#ifndef PUGIHTML_ENCODING_HPP
#define PUGIHTML_ENCODING_HPP 1

#include <cstddef>


namespace pugihtml
{

// These flags determine the encoding of input data for HTML document
enum html_encoding {
	// Auto-detect input encoding using BOM or < / <? detection;
	// use UTF8 if BOM is not found.
	encoding_auto,
	encoding_utf8,      // UTF8 encoding
	encoding_utf16_le,  // Little-endian UTF16
	encoding_utf16_be,  // Big-endian UTF16
	encoding_utf16,     // UTF16 with native endianness
	encoding_utf32_le,  // Little-endian UTF32
	encoding_utf32_be,  // Big-endian UTF32
	encoding_utf32,     // UTF32 with native endianness
	encoding_wchar      // The same encoding wchar_t has (either UTF16 or UTF32)
};

bool is_little_endian(void);

html_encoding get_buffer_encoding(html_encoding encoding, const void* contents,
	size_t size);

html_encoding get_write_encoding(html_encoding encoding);
html_encoding get_wchar_encoding(void);
html_encoding get_write_native_encoding();

size_t as_utf8_begin(const wchar_t* str, size_t length);
void as_utf8_end(char* buffer, size_t size, const wchar_t* str, size_t length);

} // pugihtml.

#endif /* PUGIHTML_ENCODING_HPP */
