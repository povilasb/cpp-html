=====
CDATA
=====

The term CDATA is used about text data that should not be parsed by the XML
parser. Characters like "<" and "&" are illegal in XML elements.
"<" will generate an error because the parser interprets it as the start of a
new element.
"&" will generate an error because the parser interprets it as the start of an
character entity.
Some text, like JavaScript code, contains a lot of "<" or "&" characters.
To avoid errors script code can be defined as CDATA.
Everything inside a CDATA section is ignored by the parser.

A CDATA section starts with "<![CDATA[" and ends with "]]>"


In HTML
=======

CDATA has no meaning at all in HTML.

CDATA is an XML construct which sets a tag's contents that is normally
#PCDATA - parsed character data, to be instead taken as #CDATA, that is,
non-parsed character data. It is only relevant and valid in XHTML.

It is used in script tags to avoid parsing < and &. In HTML, this is not needed,
because in HTML, script is already #CDATA.
