#ifndef PUGIHTML_HTML_WRITER_HPP
#define PUGIHTML_HTML_WRITER_HPP 1

#include "pugiconfig.hpp"
#include "common.hpp"


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


// Writer interface for node printing (see html_node::print)
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

} // pugihtml.

#endif /* PUGIHTML_HTML_WRITER_HPP */
