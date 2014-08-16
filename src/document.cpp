#include <ios>
#include <istream>
#include <vector>
#include <memory>

#include <pugihtml/document.hpp>
#include <pugihtml/node.hpp>
#include <pugihtml/attribute.hpp>


namespace pugihtml
{


std::shared_ptr<document>
document::create()
{
	return std::shared_ptr<document>(new document());
}


document::document() : node(node_document)
{
}


/*
inline html_parse_result
make_parse_result(html_parse_status status, ptrdiff_t offset = 0)
{
	html_parse_result result;
	result.status = status;
	result.offset = offset;

	return result;
}


#ifndef PUGIHTML_NO_STL
template <typename T> html_parse_result
load_stream_impl(document& doc, std::basic_istream<T>& stream,
	unsigned int options, html_encoding encoding)
{
	// get length of remaining data in stream
	typename std::basic_istream<T>::pos_type pos = stream.tellg();
	stream.seekg(0, std::ios::end);
	std::streamoff length = stream.tellg() - pos;
	stream.seekg(pos);

	if (stream.fail() || pos < 0) return make_parse_result(status_io_error);

	// guard against huge files
	size_t read_length = static_cast<size_t>(length);

	if (static_cast<std::streamsize>(read_length) != length || length < 0) return make_parse_result(status_out_of_memory);

	// read stream data into memory (guard against stream exceptions with buffer holder)
	buffer_holder buffer(global_allocate((read_length > 0 ? read_length : 1) * sizeof(T)), global_deallocate);
	if (!buffer.data) return make_parse_result(status_out_of_memory);

	stream.read(static_cast<T*>(buffer.data), static_cast<std::streamsize>(read_length));

	// read may set failbit | eofbit in case gcount() is less than read_length (i.e. line ending conversion), so check for other I/O errors
	if (stream.bad()) return make_parse_result(status_io_error);

	// load data from buffer
	size_t actual_length = static_cast<size_t>(stream.gcount());
	assert(actual_length <= read_length);

	return doc.load_buffer_inplace_own(buffer.release(), actual_length * sizeof(T), options, encoding);
}


html_parse_result
document::load(std::basic_istream<char, std::char_traits<char> >& stream, unsigned int options, html_encoding encoding)
{
	reset();

	return load_stream_impl(*this, stream, options, encoding);
}

html_parse_result
document::load(std::basic_istream<wchar_t, std::char_traits<wchar_t> >& stream, unsigned int options)
{
	reset();

	return load_stream_impl(*this, stream, options, encoding_wchar);
}
#endif

html_parse_result document::load(const char_t* contents, unsigned int options)
{
	// Force native encoding (skip autodetection)
#ifdef PUGIHTML_WCHAR_MODE
	html_encoding encoding = encoding_wchar;
#else
	html_encoding encoding = encoding_utf8;
#endif

	return load_buffer(contents, strlength(contents) * sizeof(char_t), options, encoding);
}


html_parse_result
load_file_impl(document& doc, FILE* file, unsigned int options,
	html_encoding encoding)
{
	if (!file) return make_parse_result(status_file_not_found);

	// get file size (can result in I/O errors)
	size_t size = 0;
	html_parse_status size_status = get_file_size(file, size);

	if (size_status != status_ok)
	{
		fclose(file);
		return make_parse_result(size_status);
	}

	// allocate buffer for the whole file
	char* contents = static_cast<char*>(global_allocate(size > 0 ? size : 1));

	if (!contents)
	{
		fclose(file);
		return make_parse_result(status_out_of_memory);
	}

	// read file in memory
	size_t read_size = fread(contents, 1, size, file);
	fclose(file);

	if (read_size != size)
	{
		global_deallocate(contents);
		return make_parse_result(status_io_error);
	}

	return doc.load_buffer_inplace_own(contents, size, options, encoding);
}

html_parse_result document::load_file(const char* path, unsigned int options, html_encoding encoding)
{
	reset();

	FILE* file = fopen(path, "rb");

	return load_file_impl(*this, file, options, encoding);
}


#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__)
FILE*
open_file_wide(const wchar_t* path, const wchar_t* mode)
{
	return _wfopen(path, mode);
}
#else


char*
convert_path_heap(const wchar_t* str)
{
	assert(str);

	// first pass: get length in utf8 characters
	size_t length = wcslen(str);
	size_t size = as_utf8_begin(str, length);

	// allocate resulting string
	char* result = static_cast<char*>(global_allocate(size + 1));
	if (!result) return 0;

	// second pass: convert to utf8
	as_utf8_end(result, size, str, length);

	return result;
}


FILE*
open_file_wide(const wchar_t* path, const wchar_t* mode)
{
	// there is no standard function to open wide paths, so our best bet is to try utf8 path
	char* path_utf8 = convert_path_heap(path);
	if (!path_utf8) return 0;

	// convert mode to ASCII (we mirror _wfopen interface)
	char mode_ascii[4] = {0};
	for (size_t i = 0; mode[i]; ++i) mode_ascii[i] = static_cast<char>(mode[i]);

	// try to open the utf8 path
	FILE* result = fopen(path_utf8, mode_ascii);

	// free dummy buffer
	global_deallocate(path_utf8);

	return result;
}
#endif

html_parse_result
document::load_file(const wchar_t* path, unsigned int options,
	html_encoding encoding)
{
	reset();

	FILE* file = open_file_wide(path, L"rb");

	return load_file_impl(*this, file, options, encoding);
}


bool
get_mutable_buffer(char_t*& out_buffer, size_t& out_length,
	const void* contents, size_t size, bool is_mutable)
{
	if (is_mutable) {
		out_buffer = static_cast<char_t*>(const_cast<void*>(contents));
	}
	else {
		void* buffer = global_allocate(size > 0 ? size : 1);
		if (!buffer) return false;

		memcpy(buffer, contents, size);

		out_buffer = static_cast<char_t*>(buffer);
	}

	out_length = size / sizeof(char_t);

	return true;
}


#ifdef PUGIHTML_WCHAR_MODE
inline bool
need_endian_swap_utf(html_encoding le, html_encoding re)
{
	return (le == encoding_utf16_be && re == encoding_utf16_le) || (le == encoding_utf16_le && re == encoding_utf16_be) ||
		   (le == encoding_utf32_be && re == encoding_utf32_le) || (le == encoding_utf32_le && re == encoding_utf32_be);
}


bool
convert_buffer_endian_swap(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, bool is_mutable)
{
	const char_t* data = static_cast<const char_t*>(contents);

	if (is_mutable)
	{
		out_buffer = const_cast<char_t*>(data);
	}
	else
	{
		out_buffer = static_cast<char_t*>(global_allocate(size > 0 ? size : 1));
		if (!out_buffer) return false;
	}

	out_length = size / sizeof(char_t);

	convert_wchar_endian_swap(out_buffer, data, out_length);

	return true;
}


bool
convert_buffer_utf8(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size)
{
	const uint8_t* data = static_cast<const uint8_t*>(contents);

	// first pass: get length in wchar_t units
	out_length = utf_decoder<wchar_counter>::decode_utf8_block(data, size, 0);

	// allocate buffer of suitable length
	out_buffer = static_cast<char_t*>(global_allocate((out_length > 0 ? out_length : 1) * sizeof(char_t)));
	if (!out_buffer) return false;

	// second pass: convert utf8 input to wchar_t
	wchar_writer::value_type out_begin = reinterpret_cast<wchar_writer::value_type>(out_buffer);
	wchar_writer::value_type out_end = utf_decoder<wchar_writer>::decode_utf8_block(data, size, out_begin);

	assert(out_end == out_begin + out_length);
	(void)!out_end;

	return true;
}


template <typename opt_swap> bool
convert_buffer_utf16(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
{
	const uint16_t* data = static_cast<const uint16_t*>(contents);
	size_t length = size / sizeof(uint16_t);

	// first pass: get length in wchar_t units
	out_length = utf_decoder<wchar_counter, opt_swap>::decode_utf16_block(data, length, 0);

	// allocate buffer of suitable length
	out_buffer = static_cast<char_t*>(global_allocate((out_length > 0 ? out_length : 1) * sizeof(char_t)));
	if (!out_buffer) return false;

	// second pass: convert utf16 input to wchar_t
	wchar_writer::value_type out_begin = reinterpret_cast<wchar_writer::value_type>(out_buffer);
	wchar_writer::value_type out_end = utf_decoder<wchar_writer, opt_swap>::decode_utf16_block(data, length, out_begin);

	assert(out_end == out_begin + out_length);
	(void)!out_end;

	return true;
}


template <typename opt_swap> bool
convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
{
	const uint32_t* data = static_cast<const uint32_t*>(contents);
	size_t length = size / sizeof(uint32_t);

	// first pass: get length in wchar_t units
	out_length = utf_decoder<wchar_counter, opt_swap>::decode_utf32_block(data, length, 0);

	// allocate buffer of suitable length
	out_buffer = static_cast<char_t*>(global_allocate((out_length > 0 ? out_length : 1) * sizeof(char_t)));
	if (!out_buffer) return false;

	// second pass: convert utf32 input to wchar_t
	wchar_writer::value_type out_begin = reinterpret_cast<wchar_writer::value_type>(out_buffer);
	wchar_writer::value_type out_end = utf_decoder<wchar_writer, opt_swap>::decode_utf32_block(data, length, out_begin);

	assert(out_end == out_begin + out_length);
	(void)!out_end;

	return true;
}


bool
convert_buffer(char_t*& out_buffer, size_t& out_length, html_encoding encoding,
	const void* contents, size_t size, bool is_mutable)
{
	// get native encoding
	html_encoding wchar_encoding = get_wchar_encoding();

	// fast path: no conversion required
	if (encoding == wchar_encoding) return get_mutable_buffer(out_buffer, out_length, contents, size, is_mutable);

	// only endian-swapping is required
	if (need_endian_swap_utf(encoding, wchar_encoding)) return convert_buffer_endian_swap(out_buffer, out_length, contents, size, is_mutable);

	// source encoding is utf8
	if (encoding == encoding_utf8) return convert_buffer_utf8(out_buffer, out_length, contents, size);

	// source encoding is utf16
	if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
	{
		html_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

		return (native_encoding == encoding) ?
			convert_buffer_utf16(out_buffer, out_length, contents, size, opt_false()) :
			convert_buffer_utf16(out_buffer, out_length, contents, size, opt_true());
	}

	// source encoding is utf32
	if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
	{
		html_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

		return (native_encoding == encoding) ?
			convert_buffer_utf32(out_buffer, out_length, contents, size, opt_false()) :
			convert_buffer_utf32(out_buffer, out_length, contents, size, opt_true());
	}

	assert(!"Invalid encoding");
	return false;
}
#else
template <typename opt_swap> bool
convert_buffer_utf16(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
{
	const uint16_t* data = static_cast<const uint16_t*>(contents);
	size_t length = size / sizeof(uint16_t);

	// first pass: get length in utf8 units
	out_length = utf_decoder<utf8_counter, opt_swap>::decode_utf16_block(data, length, 0);

	// allocate buffer of suitable length
	out_buffer = static_cast<char_t*>(global_allocate((out_length > 0 ? out_length : 1) * sizeof(char_t)));
	if (!out_buffer) return false;

	// second pass: convert utf16 input to utf8
	uint8_t* out_begin = reinterpret_cast<uint8_t*>(out_buffer);
	uint8_t* out_end = utf_decoder<utf8_writer, opt_swap>::decode_utf16_block(data, length, out_begin);

	assert(out_end == out_begin + out_length);
	(void)!out_end;

	return true;
}


template <typename opt_swap> bool
convert_buffer_utf32(char_t*& out_buffer, size_t& out_length, const void* contents, size_t size, opt_swap)
{
	const uint32_t* data = static_cast<const uint32_t*>(contents);
	size_t length = size / sizeof(uint32_t);

	// first pass: get length in utf8 units
	out_length = utf_decoder<utf8_counter, opt_swap>::decode_utf32_block(data, length, 0);

	// allocate buffer of suitable length
	out_buffer = static_cast<char_t*>(global_allocate((out_length > 0 ? out_length : 1) * sizeof(char_t)));
	if (!out_buffer) return false;

	// second pass: convert utf32 input to utf8
	uint8_t* out_begin = reinterpret_cast<uint8_t*>(out_buffer);
	uint8_t* out_end = utf_decoder<utf8_writer, opt_swap>::decode_utf32_block(data, length, out_begin);

	assert(out_end == out_begin + out_length);
	(void)!out_end;

	return true;
}


bool
convert_buffer(char_t*& out_buffer, size_t& out_length, html_encoding encoding, const void* contents, size_t size, bool is_mutable)
{
	// fast path: no conversion required
	if (encoding == encoding_utf8) return get_mutable_buffer(out_buffer, out_length, contents, size, is_mutable);

	// source encoding is utf16
	if (encoding == encoding_utf16_be || encoding == encoding_utf16_le)
	{
		html_encoding native_encoding = is_little_endian() ? encoding_utf16_le : encoding_utf16_be;

		return (native_encoding == encoding) ?
			convert_buffer_utf16(out_buffer, out_length, contents, size, opt_false()) :
			convert_buffer_utf16(out_buffer, out_length, contents, size, opt_true());
	}

	// source encoding is utf32
	if (encoding == encoding_utf32_be || encoding == encoding_utf32_le)
	{
		html_encoding native_encoding = is_little_endian() ? encoding_utf32_le : encoding_utf32_be;

		return (native_encoding == encoding) ?
			convert_buffer_utf32(out_buffer, out_length, contents, size, opt_false()) :
			convert_buffer_utf32(out_buffer, out_length, contents, size, opt_true());
	}

	assert(!"Invalid encoding");
	return false;
}
#endif



html_parse_result
document::load_buffer_impl(void* contents, size_t size, unsigned int options, html_encoding encoding, bool is_mutable, bool own)
{
	reset();

	// check input buffer
	assert(contents || size == 0);

	// get actual encoding
	html_encoding buffer_encoding = get_buffer_encoding(encoding, contents, size);

	// get private buffer
	char_t* buffer = 0;
	size_t length = 0;

	if (!convert_buffer(buffer, length, buffer_encoding, contents, size,
		is_mutable)) {
		return make_parse_result(status_out_of_memory);
	}

	// delete original buffer if we performed a conversion
	if (own && buffer != contents && contents) global_deallocate(contents);

	// parse
	html_parse_result res = parser::parse(buffer, length, _root, options);

	// remember encoding
	res.encoding = buffer_encoding;

	// grab onto buffer if it's our buffer, user is responsible for deallocating contens himself
	if (own || buffer != contents) _buffer = buffer;

	return res;
}


void
write_bom(html_writer& writer, html_encoding encoding)
{
	switch (encoding) {
	case encoding_utf8:
		writer.write("\xef\xbb\xbf", 3);
		break;

	case encoding_utf16_be:
		writer.write("\xfe\xff", 2);
		break;

	case encoding_utf16_le:
		writer.write("\xff\xfe", 2);
		break;

	case encoding_utf32_be:
		writer.write("\x00\x00\xfe\xff", 4);
		break;

	case encoding_utf32_le:
		writer.write("\xff\xfe\x00\x00", 4);
		break;

	default:
		assert(!"Invalid encoding");
	}
}


inline bool
has_declaration(const node& _node)
{
	for (node child = _node.first_child(); child; child = child.next_sibling())
	{
		node_type type = child.type();

		if (type == node_declaration) return true;
		if (type == node_element) return false;
	}

	return false;
}


void
document::save(html_writer& writer, const char_t* indent,
	unsigned int flags, html_encoding encoding) const
{
	if (flags & format_write_bom) write_bom(writer, get_write_encoding(encoding));

	html_buffered_writer buffered_writer(writer, encoding);

	if (!(flags & format_no_declaration) && !has_declaration(*this))
	{
		buffered_writer.write(PUGIHTML_TEXT("<?html version=\"1.0\"?>"));
		if (!(flags & format_raw)) buffered_writer.write('\n');
	}

	node_output(buffered_writer, *this, indent, flags, 0);
}


void
document::save(std::basic_ostream<char_type,
	std::char_traits<char_type> >& stream, const string_type& indent,
	unsigned int flags, html_encoding encoding) const;
{
	html_writer_stream writer(stream);

	save(writer, indent, flags, encoding);
}
*/


class links_walker : public node_walker {
public:
	links_walker(std::vector<std::shared_ptr<node> >& links) : links_(links)
	{
	}

	bool
	for_each(std::shared_ptr<node> node) override
	{
		if (node->name() == "A" ||
			node->name() == "AREA") {
			this->links_.push_back(node);
		}

		return true;
	}

private:
	std::vector<std::shared_ptr<node> >& links_;
};

std::vector<std::shared_ptr<node> >
document::links() const
{
	std::vector<std::shared_ptr<node> > result;

	links_walker html_walker(result);
	const_cast<document*>(this)->traverse(html_walker);

	return result;
}


std::shared_ptr<node>
document::get_element_by_id(const string_type& id) const
{
	return this->find_node([&](const std::shared_ptr<node>& node) {
		auto attr = node->get_attribute("ID");
		return attr ? attr->value() == id : false;
	});
}

} // pugihtml.
