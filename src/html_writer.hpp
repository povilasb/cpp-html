#ifndef PUGIHTML_HTML_WRITER_HPP
#define PUGIHTML_HTML_WRITER_HPP 1

#include "pugiconfig.hpp"
#include "common.hpp"
#include "encoding.hpp"


namespace pugihtml
{
// Formatting flags

// Indent the nodes that are written to output stream with as many indentation strings as deep the node is in DOM tree. This flag is on by default.
const unsigned int format_indent = 0x01;

// Write encoding-specific BOM to the output stream. This flag is off by default.
const unsigned int format_write_bom = 0x02;

// Use raw output mode (no indentation and no line breaks are written). This flag is off by default.
const unsigned int format_raw = 0x04;

// Omit default HTML declaration even if there is no declaration in the document. This flag is off by default.
const unsigned int format_no_declaration = 0x08;

// The default set of formatting flags.
// Nodes are indented depending on their depth in DOM tree, a default declaration is output if document has none.
const unsigned int format_default = format_indent;


// Writer interface for node printing (see node::print)
class PUGIHTML_CLASS html_writer {
public:
	virtual ~html_writer() {}

	// Write memory chunk into stream/file/whatever
	virtual void write(const void* data, size_t size) = 0;
};


// TODO(povilas): use c++ ofstream.
// html_writer implementation for FILE*
class PUGIHTML_CLASS html_writer_file: public html_writer {
public:
	// Construct writer from a FILE* object; void* is used to avoid header dependencies on stdio
	html_writer_file(void* file);

	virtual void write(const void* data, size_t size);

private:
	void* file;
};


#ifndef PUGIHTML_NO_STL
// html_writer implementation for streams
class PUGIHTML_CLASS html_writer_stream: public html_writer {
public:
	// Construct writer from an output stream object
	html_writer_stream(std::basic_ostream<char, std::char_traits<char> >& stream);
	html_writer_stream(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream);

	virtual void write(const void* data, size_t size);

private:
	std::basic_ostream<char, std::char_traits<char> >* narrow_stream;
	std::basic_ostream<wchar_t, std::char_traits<wchar_t> >* wide_stream;
};
#endif


class html_buffered_writer {
	html_buffered_writer(const html_buffered_writer&);
	html_buffered_writer& operator=(const html_buffered_writer&);

public:
	html_buffered_writer(html_writer& writer, html_encoding user_encoding);

	~html_buffered_writer();

	void flush();

	void flush(const char_t* data, size_t size);

	void write(const char_t* data, size_t length);

	void write(const char_t* data);

	void write(char_t d0);

	void write(char_t d0, char_t d1);

	void write(char_t d0, char_t d1, char_t d2);

	void write(char_t d0, char_t d1, char_t d2, char_t d3);

	void write(char_t d0, char_t d1, char_t d2, char_t d3, char_t d4);

	void write(char_t d0, char_t d1, char_t d2, char_t d3, char_t d4,
		char_t d5);

	// utf8 maximum expansion: x4 (-> utf32)
	// utf16 maximum expansion: x2 (-> utf32)
	// utf32 maximum expansion: x1
	enum { bufcapacity = 2048 };

	char_t buffer[bufcapacity];
	char scratch[4 * bufcapacity];

	html_writer& writer;
	size_t bufsize;
	html_encoding encoding;
};


struct utf16_writer {
	typedef uint16_t* value_type;

	static value_type
	low(value_type result, uint32_t ch)
	{
		*result = static_cast<uint16_t>(ch);
		return result + 1;
	}


	static value_type
	high(value_type result, uint32_t ch)
	{
		uint32_t msh = (uint32_t)(ch - 0x10000) >> 10;
		uint32_t lsh = (uint32_t)(ch - 0x10000) & 0x3ff;

		result[0] = static_cast<uint16_t>(0xD800 + msh);
		result[1] = static_cast<uint16_t>(0xDC00 + lsh);

		return result + 2;
	}


	static value_type
	any(value_type result, uint32_t ch)
	{
		return (ch < 0x10000) ? low(result, ch) : high(result, ch);
	}
};


struct utf32_writer {
	typedef uint32_t* value_type;

	static value_type
	low(value_type result, uint32_t ch)
	{
		*result = ch;

		return result + 1;
	}


	static value_type
	high(value_type result, uint32_t ch)
	{
		*result = ch;
		return result + 1;
	}


	static value_type
	any(value_type result, uint32_t ch)
	{
		*result = ch;
		return result + 1;
	}
};


} // pugihtml.

#endif /* PUGIHTML_HTML_WRITER_HPP */
