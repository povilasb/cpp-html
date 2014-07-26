=====
About
=====

pugihtml HTML processing library based on pugixml

This is the distribution of pugihtml, which is a C++ HTML processing library,
consisting of a DOM-like interface with rich traversal/modification
capabilities, an extremely fast HTML parser which constructs the DOM tree from
an HTML file/buffer, and an XPath 1.0 implementation for complex data-driven
tree queries. Full Unicode support is also available, with Unicode interface
variants and conversions between different Unicode encodings (which happen
automatically during parsing/saving).

The distribution contains the following files and folders:

	contrib/ - various contributions to pugihtml

	docs/ - documentation
		docs/samples - pugixml usage examples
		docs/quickstart.html - quick start guide
		docs/manual.html - complete manual

	scripts/ - project files for IDE/build systems

	src/ - header and source files

        LICENSE

	README - this file.


Dependencies
============

None.


Test
----

* googletest. Download latest version from
  http://code.google.com/p/googletest/downloads/list. These tests were written
  with 1.7.0. Unzip to './lib'.
