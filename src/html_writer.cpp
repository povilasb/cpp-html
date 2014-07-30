#include <cstdio>
#include <cassert>
#include <ios>
#include <ostream>

#include "html_writer.hpp"

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
