#include <cstdio>
#include <cassert>
#include <ios>
#include <ostream>
#include <cstring>

#include "html_writer.hpp"
#include "html.hpp"
#include "pugiutil.hpp"
#include "utf_decoder.hpp"

using namespace pugihtml;


html_writer_file::html_writer_file(void* file): file(file)
{
}


void html_writer_file::write(const void* data, size_t size)
{
	fwrite(data, size, 1, static_cast<FILE*>(file));
}


#ifndef PUGIHTML_NO_STL
html_writer_stream::html_writer_stream(std::basic_ostream<char,
	std::char_traits<char> >& stream): narrow_stream(&stream), wide_stream(0)
{
}


html_writer_stream::html_writer_stream(std::basic_ostream<wchar_t,
	std::char_traits<wchar_t> >& stream): narrow_stream(0), wide_stream(&stream)
{
}


void
html_writer_stream::write(const void* data, size_t size)
{
	if (narrow_stream) {
		assert(!wide_stream);
		narrow_stream->write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
	}
	else
	{
		assert(wide_stream);
		assert(size % sizeof(wchar_t) == 0);

		wide_stream->write(reinterpret_cast<const wchar_t*>(data), static_cast<std::streamsize>(size / sizeof(wchar_t)));
	}
}
#endif


html_buffered_writer::html_buffered_writer(html_writer& writer,
	html_encoding user_encoding) : writer(writer), bufsize(0),
	encoding(get_write_encoding(user_encoding))
{
}


html_buffered_writer::~html_buffered_writer()
{
	flush();
}


void
html_buffered_writer::flush()
{
	flush(buffer, bufsize);
	bufsize = 0;
}


#ifdef PUGIHTML_WCHAR_MODE
size_t convert_buffer(char* result, const char_t* data, size_t length, html_encoding encoding)
{
	// only endian-swapping is required
	if (need_endian_swap_utf(encoding, get_wchar_encoding()))
	{
		convert_wchar_endian_swap(reinterpret_cast<char_t*>(result), data, length);

		return length * sizeof(char_t);
	}

	// convert to utf8
	if (encoding == encoding_utf8)
	{
		uint8_t* dest = reinterpret_cast<uint8_t*>(result);

		uint8_t* end = sizeof(wchar_t) == 2 ?
			utf_decoder<utf8_writer>::decode_utf16_block(reinterpret_cast<const uint16_t*>(data), length, dest) :
			utf_decoder<utf8_writer>::decode_utf32_block(reinterpret_cast<const uint32_t*>(data), length, dest);

		return static_cast<size_t>(end - dest);
	}

	// convert to utf16
	if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
	{
		uint16_t* dest = reinterpret_cast<uint16_t*>(result);

		// convert to native utf16
		uint16_t* end = utf_decoder<utf16_writer>::decode_utf32_block(reinterpret_cast<const uint32_t*>(data), length, dest);

		// swap if necessary
		html_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

		if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

		return static_cast<size_t>(end - dest) * sizeof(uint16_t);
	}

	// convert to utf32
	if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
	{
		uint32_t* dest = reinterpret_cast<uint32_t*>(result);

		// convert to native utf32
		uint32_t* end = utf_decoder<utf32_writer>::decode_utf16_block(reinterpret_cast<const uint16_t*>(data), length, dest);

		// swap if necessary
		html_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

		if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

		return static_cast<size_t>(end - dest) * sizeof(uint32_t);
	}

	assert(!"Invalid encoding");
	return 0;
}
#else
size_t
convert_buffer(char* result, const char_t* data, size_t length,
html_encoding encoding)
{
	if (encoding == encoding_utf16_be || encoding == encoding_utf16_le) {
		uint16_t* dest = reinterpret_cast<uint16_t*>(result);

		// convert to native utf16
		uint16_t* end = utf_decoder<utf16_writer>::decode_utf8_block(
			reinterpret_cast<const uint8_t*>(data), length, dest);

		// swap if necessary
		html_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

		if (native_encoding != encoding) convert_utf_endian_swap(dest, dest, static_cast<size_t>(end - dest));

		return static_cast<size_t>(end - dest) * sizeof(uint16_t);
	}

	if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
	{
		uint32_t* dest = reinterpret_cast<uint32_t*>(result);

		// convert to native utf32
		uint32_t* end = utf_decoder<utf32_writer>::decode_utf8_block(reinterpret_cast<const uint8_t*>(data), length, dest);

		// swap if necessary
		html_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

		if (native_encoding != encoding) {
			convert_utf_endian_swap(dest, dest,
				static_cast<size_t>(end - dest));
		}

		return static_cast<size_t>(end - dest) * sizeof(uint32_t);
	}

	assert(!"Invalid encoding");
	return 0;
}
#endif

#ifdef PUGIHTML_WCHAR_MODE
size_t
get_valid_length(const char_t* data, size_t length)
{
	assert(length > 0);

	// discard last character if it's the lead of a surrogate pair
	return (sizeof(wchar_t) == 2
		&& (unsigned)(static_cast<uint16_t>(data[length - 1]) - 0xD800)
		< 0x400) ? length - 1 : length;
}
#else
size_t
get_valid_length(const char_t* data, size_t length)
{
	assert(length > 4);

	for (size_t i = 1; i <= 4; ++i) {
		uint8_t ch = static_cast<uint8_t>(data[length - i]);

		// either a standalone character or a leading one
		if ((ch & 0xc0) != 0x80) return length - i;
	}

	// there are four non-leading characters at the end, sequence tail is
	// broken so might as well process the whole chunk.
	return length;
}
#endif


void
html_buffered_writer::flush(const char_t* data, size_t size)
{
	if (size == 0) return;

	// fast path, just write data
	if (encoding == get_write_native_encoding())
		writer.write(data, size * sizeof(char_t));
	else {
		// convert chunk
		size_t result = convert_buffer(scratch, data, size, encoding);
		assert(result <= sizeof(scratch));

		// write data
		writer.write(scratch, result);
	}
}


void
html_buffered_writer::write(const char_t* data, size_t length)
{
	if (bufsize + length > bufcapacity) {
		// flush the remaining buffer contents
		flush();

		// handle large chunks
		if (length > bufcapacity) {
			if (encoding == get_write_native_encoding())
			{
				// fast path, can just write data chunk
				writer.write(data, length * sizeof(char_t));
				return;
			}

			// need to convert in suitable chunks
			while (length > bufcapacity) {
				// get chunk size by selecting such number of
				// characters that are guaranteed to fit into
				// scratch buffer and form a complete codepoint
				// sequence (i.e. discard start of last
				// codepoint if necessary)
				size_t chunk_size = get_valid_length(data,
					bufcapacity);

				// convert chunk and write
				flush(data, chunk_size);

				// iterate
				data += chunk_size;
				length -= chunk_size;
			}

			// small tail is copied below
			bufsize = 0;
		}
	}

	memcpy(buffer + bufsize, data, length * sizeof(char_t));
	bufsize += length;
}


void
html_buffered_writer::write(const char_t* data)
{
	write(data, strlength(data));
}


void
html_buffered_writer::write(char_t d0)
{
	if (bufsize + 1 > bufcapacity) flush();

	buffer[bufsize + 0] = d0;
	bufsize += 1;
}


void
html_buffered_writer::write(char_t d0, char_t d1)
{
	if (bufsize + 2 > bufcapacity) flush();

	buffer[bufsize + 0] = d0;
	buffer[bufsize + 1] = d1;
	bufsize += 2;
}


void
html_buffered_writer::write(char_t d0, char_t d1, char_t d2)
{
	if (bufsize + 3 > bufcapacity) flush();

	buffer[bufsize + 0] = d0;
	buffer[bufsize + 1] = d1;
	buffer[bufsize + 2] = d2;
	bufsize += 3;
}


void
html_buffered_writer::write(char_t d0, char_t d1, char_t d2, char_t d3)
{
	if (bufsize + 4 > bufcapacity) flush();

	buffer[bufsize + 0] = d0;
	buffer[bufsize + 1] = d1;
	buffer[bufsize + 2] = d2;
	buffer[bufsize + 3] = d3;
	bufsize += 4;
}


void
html_buffered_writer::write(char_t d0, char_t d1, char_t d2, char_t d3, char_t d4)
{
	if (bufsize + 5 > bufcapacity) flush();

	buffer[bufsize + 0] = d0;
	buffer[bufsize + 1] = d1;
	buffer[bufsize + 2] = d2;
	buffer[bufsize + 3] = d3;
	buffer[bufsize + 4] = d4;
	bufsize += 5;
}


void
html_buffered_writer::write(char_t d0, char_t d1, char_t d2, char_t d3,
	char_t d4, char_t d5)
{
	if (bufsize + 6 > bufcapacity) flush();

	buffer[bufsize + 0] = d0;
	buffer[bufsize + 1] = d1;
	buffer[bufsize + 2] = d2;
	buffer[bufsize + 3] = d3;
	buffer[bufsize + 4] = d4;
	buffer[bufsize + 5] = d5;
	bufsize += 6;
}


html_encoding
pugihtml::get_wchar_encoding()
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
