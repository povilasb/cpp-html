#ifndef PUGIHTML_HTML_HPP
#define PUGIHTML_HTML_HPP 1


namespace pugihtml
{

// These flags determine the encoding of input data for HTML document
enum html_encoding {
	encoding_auto,      // Auto-detect input encoding using BOM or < / <? detection; use UTF8 if BOM is not found
	encoding_utf8,      // UTF8 encoding
	encoding_utf16_le,  // Little-endian UTF16
	encoding_utf16_be,  // Big-endian UTF16
	encoding_utf16,     // UTF16 with native endianness
	encoding_utf32_le,  // Little-endian UTF32
	encoding_utf32_be,  // Big-endian UTF32
	encoding_utf32,     // UTF32 with native endianness
	encoding_wchar      // The same encoding wchar_t has (either UTF16 or UTF32)
};

} // pugihtml.

#endif /* PUGIHTML_HTML_HPP */
