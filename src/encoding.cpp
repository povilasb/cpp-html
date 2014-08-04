#include <cstdint>
#include <cassert>

#include "encoding.hpp"
#include "pugiutil.hpp"
#include "utf_decoder.hpp"

using namespace pugihtml;


html_encoding
guess_buffer_encoding(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
	// look for BOM in first few bytes
	if (d0 == 0 && d1 == 0 && d2 == 0xfe && d3 == 0xff) return encoding_utf32_be;
	if (d0 == 0xff && d1 == 0xfe && d2 == 0 && d3 == 0) return encoding_utf32_le;
	if (d0 == 0xfe && d1 == 0xff) return encoding_utf16_be;
	if (d0 == 0xff && d1 == 0xfe) return encoding_utf16_le;
	if (d0 == 0xef && d1 == 0xbb && d2 == 0xbf) return encoding_utf8;

	// look for <, <? or <?xm in various encodings
	if (d0 == 0 && d1 == 0 && d2 == 0 && d3 == 0x3c) return encoding_utf32_be;
	if (d0 == 0x3c && d1 == 0 && d2 == 0 && d3 == 0) return encoding_utf32_le;
	if (d0 == 0 && d1 == 0x3c && d2 == 0 && d3 == 0x3f) return encoding_utf16_be;
	if (d0 == 0x3c && d1 == 0 && d2 == 0x3f && d3 == 0) return encoding_utf16_le;
	if (d0 == 0x3c && d1 == 0x3f && d2 == 0x78 && d3 == 0x6d) return encoding_utf8;

	// Look for utf16 < followed by node name (this may fail, but is
	// better than utf8 since it's zero terminated so early).
	if (d0 == 0 && d1 == 0x3c) return encoding_utf16_be;
	if (d0 == 0x3c && d1 == 0) return encoding_utf16_le;

	// no known BOM detected, assume utf8
	return encoding_utf8;
}


html_encoding
pugihtml::get_buffer_encoding(html_encoding encoding, const void* contents,
	size_t size)
{
	// replace wchar encoding with utf implementation
	if (encoding == encoding_wchar) return get_wchar_encoding();

	// replace utf16 encoding with utf16 with specific endianness
	if (encoding == encoding_utf16)
		return is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

	// replace utf32 encoding with utf32 with specific endianness
	if (encoding == encoding_utf32)
		return is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

	// only do autodetection if no explicit encoding is requested
	if (encoding != encoding_auto) return encoding;

	// skip encoding autodetection if input buffer is too small
	if (size < 4) return encoding_utf8;

	// try to guess encoding (based on HTML specification, Appendix F.1)
	const uint8_t* data = static_cast<const uint8_t*>(contents);

	DMC_VOLATILE uint8_t d0 = data[0], d1 = data[1], d2 = data[2], d3 = data[3];

	return guess_buffer_encoding(d0, d1, d2, d3);
}


bool
::pugihtml::is_little_endian(void)
{
	unsigned int ui = 1;

	return *reinterpret_cast<unsigned char*>(&ui) == 1;
}


html_encoding
pugihtml::get_wchar_encoding(void)
{
	STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

	if (sizeof(wchar_t) == 2)
		return is_little_endian() ? encoding_utf16_le : encoding_utf16_be;
	else
		return is_little_endian() ? encoding_utf32_le : encoding_utf32_be;
}


html_encoding
pugihtml::get_write_encoding(html_encoding encoding)
{
	// replace wchar encoding with utf implementation
	if (encoding == encoding_wchar) return get_wchar_encoding();

	// replace utf16 encoding with utf16 with specific endianness
	if (encoding == encoding_utf16) {
		return is_little_endian() ? encoding_utf16_le
			: encoding_utf16_be;
	}

	// replace utf32 encoding with utf32 with specific endianness
	if (encoding == encoding_utf32) {
		return is_little_endian() ? encoding_utf32_le
			: encoding_utf32_be;
	}

	// only do autodetection if no explicit encoding is requested
	if (encoding != encoding_auto) {
		return encoding;
	}

	// assume utf8 encoding
	return encoding_utf8;
}


// Output facilities
html_encoding
pugihtml::get_write_native_encoding()
{
#ifdef PUGIHTML_WCHAR_MODE
	return get_wchar_encoding();
#else
	return encoding_utf8;
#endif
}


size_t
pugihtml::as_utf8_begin(const wchar_t* str, size_t length)
{
	STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

	// get length in utf8 characters
	return sizeof(wchar_t) == 2 ?
		utf_decoder<utf8_counter>::decode_utf16_block(reinterpret_cast<const uint16_t*>(str), length, 0) :
		utf_decoder<utf8_counter>::decode_utf32_block(reinterpret_cast<const uint32_t*>(str), length, 0);
}


void
pugihtml::as_utf8_end(char* buffer, size_t size, const wchar_t* str,
	size_t length)
{
	STATIC_ASSERT(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

	// convert to utf8
	uint8_t* begin = reinterpret_cast<uint8_t*>(buffer);
	uint8_t* end = sizeof(wchar_t) == 2 ?
		utf_decoder<utf8_writer>::decode_utf16_block(reinterpret_cast<const uint16_t*>(str), length, begin) :
		utf_decoder<utf8_writer>::decode_utf32_block(reinterpret_cast<const uint32_t*>(str), length, begin);

	assert(begin + size == end);
	(void)!end;

	// zero-terminate
	buffer[size] = 0;
}
