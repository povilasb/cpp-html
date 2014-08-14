#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <list>

#include <pugihtml/attribute.hpp>
#include <pugihtml/node.hpp>
#include <pugihtml/document.hpp>
#include <pugihtml/parser.hpp>


namespace pugihtml
{

// Parser utilities.
// TODO(povilas): replace with inline functions or completely remove some of them.
#define SKIPWS() { while (is_chartype(*s, ct_space)) ++s; }
#define OPTSET(OPT) ( optmsk & OPT )

#define THROW_ERROR(err, m) (void)m, throw parse_error(err)

#define PUSHNODE(TYPE) { cursor = append_node(cursor, alloc, TYPE); if (!cursor) THROW_ERROR(status_out_of_memory, s); }
#define POPNODE()			{ cursor = cursor->parent; }
#define SCANFOR(X)			{ while (*s != 0 && !(X)) ++s; }
#define SCANWHILE(X)		{ while ((X)) ++s; }
#define ENDSEG()			{ ch = *s; *s = 0; ++s; }
#define CHECK_ERROR(err, m)	{ if (*s == 0) THROW_ERROR(err, m); }

// Utility macro for last character handling
#define ENDSWITH(c, e) ((c) == (e) || ((c) == 0 && endch == (e)))


std::list<string_type> html_void_elements = {"AREA", "BASE", "BR",
	"COL", "EMBED", "HR", "IMG", "INPUT", "KEYGEN", "LINK", "MENUITEM",
	"META", "PARAM", "SOURCE", "TRACK", "WBR"};


const char_type*
parser::advance_doctype_primitive(const char_type* s)
{
	// Quoted string.
	if (*s == '"' || *s == '\'') {
		char_type ch = *s++;
		SCANFOR(*s == ch);
		if (!*s) THROW_ERROR(status_bad_doctype, s);

		++s;
	}
	// <? ... ?>
	else if (s[0] == '<' && s[1] == '?') {
		s += 2;
		// no need for ENDSWITH because ?> can't terminate proper doctype
		SCANFOR(s[0] == '?' && s[1] == '>');
		if (!*s) THROW_ERROR(status_bad_doctype, s);

		s += 2;
	}
	// <-- ... -->
	else if (s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-') {
		s += 4;
		// no need for ENDSWITH because --> can't terminate proper doctype
		SCANFOR(s[0] == '-' && s[1] == '-' && s[2] == '>');
		if (!*s) THROW_ERROR(status_bad_doctype, s);

		s += 4;
	}
	else {
		THROW_ERROR(status_bad_doctype, s);
	}

	return s;
}


const char_type*
parser::advance_doctype_ignore(const char_type* s)
{
	assert(s[0] == '<' && s[1] == '!' && s[2] == '[');
	++s;

	while (*s) {
		if (s[0] == '<' && s[1] == '!' && s[2] == '[') {
			// Nested ignore section.
			s = advance_doctype_ignore(s);
		}
		else if (s[0] == ']' && s[1] == ']' && s[2] == '>') {
			// Ignore section end.
			return s + 3;
		}
		else {
			++s;
		}
	}

	THROW_ERROR(status_bad_doctype, s);

	return s;
}


const char_type*
parser::advance_doctype_group(const char_type* s, char_type endch,
	bool top_level)
{
	assert(s[0] == '<' && s[1] == '!');
	++s;

	while (*s) {
		if (s[0] == '<' && s[1] == '!') {
			if (s[2] == '[') {
				// Ignore.
				s = advance_doctype_ignore(s);
			}
			else {
				// Some control group.
				s = advance_doctype_group(s, endch, false);
			}
		}
		else if (s[0] == '<' || s[0] == '"' || s[0] == '\'') {
			// unknown tag (forbidden), or some primitive group
			s = advance_doctype_primitive(s);
		}
		else if (*s == '>') {
			return ++s;
		}
		else {
			++s;
		}
	}

	if (!top_level || endch != '>') {
		THROW_ERROR(status_bad_doctype, s);
	}

	return s;
}


parser::parser(unsigned int options) : options_(options),
	document_(document::create())
{
}


/*
const char_type*
parser::parse_exclamation(const char_type* s, char_type endch)
{
	++s;

	// '<!-...'
	if (*s == '-') {
		++s;
		if (*s == '-') { // '<!--...'
			++s;

			if (this->option_set(parse_comments)) {
				auto node = node::create(node_comment);
				this->document_->append_child(node);
				//cursor->value = s; // Save the offset.
			}

			if (this->option_set(parse_eol) &&
				this->option_set(parse_comments)) {
				s = strconv_comment(s, endch);

				if (!s) {
					THROW_ERROR(status_bad_comment,
						cursor->value);
				}
			}
			else {
				// Scan for terminating '-->'.
				SCANFOR(s[0] == '-' && s[1] == '-'
					&& ENDSWITH(s[2], '>'));
				CHECK_ERROR(status_bad_comment, s);

				if (OPTSET(parse_comments)) {
					*s = 0; // Zero-terminate this segment at the first terminating '-'.
				}

				s += (s[2] == '>' ? 3 : 2); // Step over the '\0->'.
			}
		}
		else {
			THROW_ERROR(status_bad_comment, s);
		}
	}
	else if (*s == '[') {
		// '<![CDATA[...'
		if (*++s=='C' && *++s=='D' && *++s=='A' && *++s=='T'
			&& *++s=='A' && *++s == '[') {
			++s;

			if (OPTSET(parse_cdata)) {
				PUSHNODE(node_cdata); // Append a new node on the tree.
				cursor->value = s; // Save the offset.

				if (OPTSET(parse_eol)) {
					s = strconv_cdata(s, endch);

					if (!s) THROW_ERROR(status_bad_cdata, cursor->value);
				}
				else
				{
					// Scan for terminating ']]>'.
					SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
					CHECK_ERROR(status_bad_cdata, s);

					*s++ = 0; // Zero-terminate this segment.
				}
			}
			else // Flagged for discard, but we still have to scan for the terminator.
			{
				// Scan for terminating ']]>'.
				SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
				CHECK_ERROR(status_bad_cdata, s);

				++s;
			}

			s += (s[1] == '>' ? 2 : 1); // Step over the last ']>'.
		}
		else THROW_ERROR(status_bad_cdata, s);
	}
	else if (s[0] == 'D' && s[1] == 'O' && s[2] == 'C' && s[3] == 'T'
		&& s[4] == 'Y' && s[5] == 'P' && ENDSWITH(s[6], 'E')) {
		s -= 2;

		if (cursor->parent) THROW_ERROR(status_bad_doctype, s);

		char_type* mark = s + 9;

		s = parse_doctype_group(s, endch);

		if (OPTSET(parse_doctype)) {
			while (is_chartype(*mark, ct_space)) ++mark;

			PUSHNODE(node_doctype);

			cursor->value = mark;

			assert((s[0] == 0 && endch == '>') || s[-1] == '>');
			s[*s == 0 ? 0 : -1] = 0;

			POPNODE();
		}
	}
	else if (*s == 0 && endch == '-') THROW_ERROR(status_bad_comment, s);
	else if (*s == 0 && endch == '[') THROW_ERROR(status_bad_cdata, s);
	else THROW_ERROR(status_unrecognized_tag, s);

	return s;
}


/*
char_type*
parser::parse_question(char_type* s, node_struct*& ref_cursor,
	unsigned int optmsk, char_type endch)
{
	// TODO(povilas): optmsk unused?
	// load into registers
	node_struct* cursor = ref_cursor;
	char_type ch = 0;

	// parse node contents, starting with question mark
	++s;

	// read PI target
	char_type* target = s;

	if (!is_chartype(*s, ct_start_symbol)) THROW_ERROR(status_bad_pi, s);

	SCANWHILE(is_chartype(*s, ct_symbol));
	CHECK_ERROR(status_bad_pi, s);

	// determine node type; stricmp / strcasecmp is not portable
	bool declaration = (target[0] | ' ') == 'x'
		&& (target[1] | ' ') == 'm' && (target[2] | ' ') == 'l'
		&& target + 3 == s;

	if (declaration ? OPTSET(parse_declaration) : OPTSET(parse_pi)) {
		if (declaration) {
			// disallow non top-level declarations
			if (cursor->parent) THROW_ERROR(status_bad_pi, s);

			PUSHNODE(node_declaration);
		}
		else {
			PUSHNODE(node_pi);
		}

		cursor->name = target;

		ENDSEG();

		// parse value/attributes
		if (ch == '?') {
			// empty node
			if (!ENDSWITH(*s, '>')) THROW_ERROR(status_bad_pi, s);
			s += (*s == '>');

			if(cursor->parent) {
				POPNODE(); // Pop.
			}
			else {
				// We have reached the root node so do not
				// attempt to pop anymore nodes
				return s;
			}
		}
		else if (is_chartype(ch, ct_space)) {
			SKIPWS();

			// scan for tag end
			char_type* value = s;

			SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
			CHECK_ERROR(status_bad_pi, s);

			if (declaration) {
				// replace ending ? with / so that 'element' terminates properly
				*s = '/';

				// we exit from this function with cursor at node_declaration, which is a signal to parse() to go to LOC_ATTRIBUTES
				s = value;
			}
			else {
				// store value and step over >
				cursor->value = value;

				if(cursor->parent) {
					POPNODE(); // Pop.
				}
				else {
					// We have reached the root node so do not
					// attempt to pop anymore nodes
					return s;
				}
				ENDSEG();

				s += (*s == '>');
			}
		}
		else THROW_ERROR(status_bad_pi, s);
	}
	else {
		// scan for tag end
		SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
		CHECK_ERROR(status_bad_pi, s);

		s += (s[1] == '>' ? 2 : 1);
	}

	// store from registers
	ref_cursor = cursor;

	return s;
}


typedef char_type* (*strconv_attribute_t)(char_type*, char_type);

template <typename opt_escape>
struct strconv_attribute_impl {
	static char_type* parse_wnorm(char_type* s, char_type end_quote)
	{
		gap g;

		// trim leading whitespaces
		if (is_chartype(*s, ct_space)) {
			char_type* str = s;

			do ++str;
			while (is_chartype(*str, ct_space));

			g.push(s, str - s);
		}

		while (true) {
			chartype_t skip_chars =
				static_cast<enum chartype_t>(ct_parse_attr_ws
					| ct_space);
			while (!is_chartype(*s, skip_chars)) {
				++s;
			}

			if (*s == end_quote)
			{
				char_type* str = g.flush(s);

				do *str-- = 0;
				while (is_chartype(*str, ct_space));

				return s + 1;
			}
			else if (is_chartype(*s, ct_space))
			{
				*s++ = ' ';

				if (is_chartype(*s, ct_space))
				{
					char_type* str = s + 1;
					while (is_chartype(*str, ct_space)) ++str;

					g.push(s, str - s);
				}
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}

	static char_type* parse_wconv(char_type* s, char_type end_quote)
	{
		gap g;

		while (true)
		{
			while (!is_chartype(*s, ct_parse_attr_ws)) ++s;

			if (*s == end_quote)
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (is_chartype(*s, ct_space))
			{
				if (*s == '\r')
				{
					*s++ = ' ';

					if (*s == '\n') g.push(s, 1);
				}
				else *s++ = ' ';
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}

	static char_type* parse_eol(char_type* s, char_type end_quote)
	{
		gap g;

		while (true)
		{
			while (!is_chartype(*s, ct_parse_attr)) ++s;

			if (*s == end_quote)
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (*s == '\r')
			{
				*s++ = '\n';

				if (*s == '\n') g.push(s, 1);
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}

	static char_type* parse_simple(char_type* s, char_type end_quote)
	{
		gap g;

		while (true)
		{
			while (!is_chartype(*s, ct_parse_attr)) ++s;

			if (*s == end_quote)
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}
};


strconv_attribute_t
get_strconv_attribute(unsigned int optmask)
{
	STATIC_ASSERT(parser::parse_escapes == 0x10
		&& parser::parse_eol == 0x20
		&& parser::parse_wconv_attribute == 0x40
		&& parser::parse_wnorm_attribute == 0x80);

	switch ((optmask >> 4) & 15) // get bitmask for flags (wconv wnorm eol escapes)
	{
	case 0:  return strconv_attribute_impl<opt_false>::parse_simple;
	case 1:  return strconv_attribute_impl<opt_true>::parse_simple;
	case 2:  return strconv_attribute_impl<opt_false>::parse_eol;
	case 3:  return strconv_attribute_impl<opt_true>::parse_eol;
	case 4:  return strconv_attribute_impl<opt_false>::parse_wconv;
	case 5:  return strconv_attribute_impl<opt_true>::parse_wconv;
	case 6:  return strconv_attribute_impl<opt_false>::parse_wconv;
	case 7:  return strconv_attribute_impl<opt_true>::parse_wconv;
	case 8:  return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 9:  return strconv_attribute_impl<opt_true>::parse_wnorm;
	case 10: return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 11: return strconv_attribute_impl<opt_true>::parse_wnorm;
	case 12: return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 13: return strconv_attribute_impl<opt_true>::parse_wnorm;
	case 14: return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 15: return strconv_attribute_impl<opt_true>::parse_wnorm;
	// TODO(povilas): throw exception if it does.
	default: return 0; // should not get here
	}
}


typedef char_type* (*strconv_pcdata_t)(char_type*);

template <typename opt_eol, typename opt_escape>
struct strconv_pcdata_impl {
	static char_type* parse(char_type* s)
	{
		gap g;

		while (true) {
			while (!is_chartype(*s, ct_parse_pcdata)) ++s;

			if (*s == '<') { // PCDATA ends here
				*g.flush(s) = 0;

				return s + 1;
			}
			// Either a single 0x0d or 0x0d 0x0a pair
			else if (opt_eol::value && *s == '\r') {
				*s++ = '\n'; // replace first one with 0x0a

				if (*s == '\n') g.push(s, 1);
			}
			else if (opt_escape::value && *s == '&') {
				s = strconv_escape(s, g);
			}
			else if (*s == 0) {
				return s;
			}
			else {
				++s;
			}
		}
	}
};


strconv_pcdata_t
get_strconv_pcdata(unsigned int optmask)
{
	STATIC_ASSERT(parser::parse_escapes == 0x10
		&& parser::parse_eol == 0x20);

	switch ((optmask >> 4) & 3) // get bitmask for flags (eol escapes)
	{
	case 0: return strconv_pcdata_impl<opt_false, opt_false>::parse;
	case 1: return strconv_pcdata_impl<opt_false, opt_true>::parse;
	case 2: return strconv_pcdata_impl<opt_true, opt_false>::parse;
	case 3: return strconv_pcdata_impl<opt_true, opt_true>::parse;
	// TODO(povilas): throw exception if it does?
	default: return 0; // should not get here
	}
}


std::shared_ptr<document>
parse(const string_type& str_html, unsigned int optmsk)
{
	this->status_ = status_ok;

	auto doc = html::document::create();
	if (str_html.size() == 0) {
		return doc;
	}

	strconv_attribute_t strconv_attribute = get_strconv_attribute(optmsk);
	strconv_pcdata_t strconv_pcdata = get_strconv_pcdata(optmsk);

	char_type ch = 0;

	// Point the cursor to the html document node
	node_struct* cursor = htmldoc;

	// Set the marker
	char_type* mark = s;

	// It's necessary to keep another mark when we have to roll
	// back the name and the mark at the same time.
	char_type* sMark = s;
	char_type* nameMark = s;

	// Parse while the current character is not '\0'
	while (*s != 0) {
		// Check if the current character is the start tag character
		if (*s == '<') {
			// Move to the next character
			++s;

			// Check if the current character is a start symbol
		LOC_TAG:
			if (is_chartype(*s, ct_start_symbol)) // '<#...'
			{
				// Append a new node to the tree.
				PUSHNODE(node_element);

				// Set the current element's name
				cursor->name = s;

				// Scan while the current character is a symbol belonging
				// to the set of symbols acceptable within a tag. In other
				// words, scan until the termination symbol is discovered.
				SCANWHILE(is_chartype(*s, ct_symbol));

				// Save char in 'ch', terminate & step over.
				ENDSEG();

				// Capitalize the tag name
				to_upper(cursor->name);

				// End of tag
				if (ch == '>') {
					auto it = std::find(
						html_void_elements.cbegin(),
						html_void_elements.cend(),
						cursor->name);
					if (it != html_void_elements.cend()) {
						if (cursor->parent) {
							POPNODE();
						}
					}
				}
				else if (is_chartype(ch, ct_space)) {
				LOC_ATTRIBUTES:
					while (true)
					{
						SKIPWS(); // Eat any whitespace.

						if (is_chartype(*s, ct_start_symbol)) // <... #...
						{
							attribute_struct* a = append_attribute_ll(cursor, alloc); // Make space for this attribute.
							if (!a) THROW_ERROR(status_out_of_memory, s);

							a->name = s; // Save the offset.

							SCANWHILE(is_chartype(*s, ct_symbol)); // Scan for a terminator.
							CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

							ENDSEG(); // Save char in 'ch', terminate & step over.

							// Capitalize the attribute name
							to_upper(a->name);

							//$ redundant, left for performance
							CHECK_ERROR(status_bad_attribute, s);

							if (is_chartype(ch, ct_space))
							{
								SKIPWS(); // Eat any whitespace.
								CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

								ch = *s;
								++s;
							}

							if (ch == '=') // '<... #=...'
							{
								SKIPWS(); // Eat any whitespace.

								if (*s == '"' || *s == '\'') // '<... #="...'
								{
									ch = *s; // Save quote char to avoid breaking on "''" -or- '""'.
									++s; // Step over the quote.
									a->value = s; // Save the offset.

									s = strconv_attribute(s, ch);

									if (!s) THROW_ERROR(status_bad_attribute, a->value);

									// After this line the loop continues from the start;
									// Whitespaces, / and > are ok, symbols and EOF are wrong,
									// everything else will be detected
									if (is_chartype(*s, ct_start_symbol)) THROW_ERROR(status_bad_attribute, s);
								}
								else THROW_ERROR(status_bad_attribute, s);
							}
							else {//hot fix:try to fix that attribute=value situation;
								ch='"';
								a->value=s;
								char_type* dp=s;
								char_type temp;
								//find the end of attribute,200 length is enough,it must return a position;
								temp=*dp;//backup the former char
								*dp='"';//hack at the end position;
								s=strconv_attribute(s,ch);
								*dp=temp;//restore it;
								s--;// back it because we come to the " but it is > indeed;
								if (is_chartype(*s, ct_start_symbol)) THROW_ERROR(status_bad_attribute, s);

							}
							//THROW_ERROR(status_bad_attribute, s);
						}
						else if (*s == '/')
						{
							++s;

							if (*s == '>')
							{
								if(cursor->parent)
								{
									POPNODE(); // Pop.
								}
								else
								{
									// We have reached the root node so do not
									// attempt to pop anymore nodes
									break;
								}
								s++;
								break;
							}
							else if (*s == 0 && endch == '>')
							{
								if(cursor->parent)
								{
									POPNODE(); // Pop.
								}
								else
								{
									// We have reached the root node so do not
									// attempt to pop anymore nodes
									break;
								}
								break;
							}
							else THROW_ERROR(status_bad_start_element, s);
						}
						else if (*s == '>')
						{
							++s;

							break;
						}
						else if (*s == 0 && endch == '>')
						{
							break;
						}
						else THROW_ERROR(status_bad_start_element, s);
					}// while

					// !!!
				}
				// Void HTML element.
				else if (ch == '/') {
					if (!ENDSWITH(*s, '>')) THROW_ERROR(status_bad_start_element, s);

					if(cursor->parent) {
						POPNODE(); // Pop.
					}
					else
					{
						// We have reached the root node so do not
						// attempt to pop anymore nodes
						break;
					}

					s += (*s == '>');
				}
				else if (ch == 0)
				{
					// we stepped over null terminator, backtrack & handle closing tag
					--s;

					if (endch != '>') THROW_ERROR(status_bad_start_element, s);
				}
				else THROW_ERROR(status_bad_start_element, s);
			}
			else if (*s == '/')
			{
				++s;

				char_type* name = cursor->name;
				if (!name)
				{
					// TODO ignore exception
					//THROW_ERROR(status_end_element_mismatch, s);
				}
				else
				{
					sMark = s;
					nameMark = name;
					// Read the name while the character is a symbol
					while (is_chartype(*s, ct_symbol))
					{
						// Check if we're closing the correct tag name:
						// if the cursor tag does not match the current
						// closing tag then throw an exception.
						if (*s++ != *name++)
						{
							// TODO POPNODE or ignore exception
							//THROW_ERROR(status_end_element_mismatch, s);

							// Return to the last position where we started
							// reading the expected closing tag name.
							s = sMark;
							name = nameMark;
							break;
						}
					}
					// Check if the end element is valid
					if (*name)
					{
						if (*s == 0 && name[0] == endch && name[1] == 0)
						{
							THROW_ERROR(status_bad_end_element, s);
						}
						else
						{
							// TODO POPNODE or ignore exception
							//THROW_ERROR(status_end_element_mismatch, s);
						}
					}
				}

				// The tag was closed so we have to pop the
				// node off the "stack".
				if(cursor->parent)
				{
					POPNODE(); // Pop.
				}
				else
				{
					// We have reached the root node so do not
					// attempt to pop anymore nodes
					break;
				}

				SKIPWS();

				// If there end of the string is reached.
				if (*s == 0)
				{
					// Check if the end character specified is the
					// same as the closing tag.
					if (endch != '>')
					{
						THROW_ERROR(status_bad_end_element, s);
					}
				}
				else
				{
					// Skip the end character becaue the tag
					// was closed and the node was popped off
					// the "stack".
					if (*s != '>')
					{
						// Continue parsing
						continue;
						// TODO ignore the exception
						// THROW_ERROR(status_bad_end_element, s);
					}
					++s;
				}
			}
			else if (*s == '?') // '<?...'
			{
				s = parse_question(s, cursor, optmsk, endch);

				assert(cursor);
				if ((cursor->header & html_memory_page_type_mask) + 1 == node_declaration)
				{
					goto LOC_ATTRIBUTES;
				}
			}
			else if (*s == '!') // '<!...'
			{
				s = parse_exclamation(s, cursor, optmsk, endch);
			}
			else if (*s == 0 && endch == '?')
			{
				THROW_ERROR(status_bad_pi, s);
			}
			else
			{
				THROW_ERROR(status_unrecognized_tag, s);
			}
		}
		else
		{
			mark = s; // Save this offset while searching for a terminator.

			SKIPWS(); // Eat whitespace if no genuine PCDATA here.

			if ((!OPTSET(parse_ws_pcdata) || mark == s) && (*s == '<' || !*s))
			{
				continue;
			}

			s = mark;

			if (cursor->parent)
			{
				PUSHNODE(node_pcdata); // Append a new node on the tree.
				cursor->value = s; // Save the offset.

				s = strconv_pcdata(s);

				POPNODE(); // Pop since this is a standalone.

				if (!*s) break;
			}
			else
			{
				SCANFOR(*s == '<'); // '...<'
				if (!*s) break;

				++s;
			}

			// We're after '<'
			goto LOC_TAG;
		}
	}

	// Check that last tag is closed
	if (cursor != htmldoc)
	{
		// TODO POPNODE or ignore exception
		// THROW_ERROR(status_end_element_mismatch, s);
		cursor = htmldoc;
	}
}
*/


string_type
parser::status_description() const
{
	return parser::status_description(this->status_);
}


string_type
parser::status_description(parse_status status)
{
	switch (status)
	{
	case status_ok: return "No error";

	case status_file_not_found: return "File was not found";
	case status_io_error: return "Error reading from file/stream";
	case status_out_of_memory: return "Could not allocate memory";
	case status_internal_error: return "Internal error occurred";

	case status_unrecognized_tag: return "Could not determine tag type";

	case status_bad_pi: return "Error parsing document declaration/processing instruction";
	case status_bad_comment: return "Error parsing comment";
	case status_bad_cdata: return "Error parsing CDATA section";
	case status_bad_doctype: return "Error parsing document type declaration";
	case status_bad_pcdata: return "Error parsing PCDATA section";
	case status_bad_start_element: return "Error parsing start element tag";
	case status_bad_attribute: return "Error parsing element attribute";
	case status_bad_end_element: return "Error parsing end element tag";
	case status_end_element_mismatch: return "Start-end tags mismatch";

	default: return "Unknown error";
	}
}


bool
parser::option_set(unsigned int opt)
{
	return this->options_ & opt;
}

} // pugihtml.
