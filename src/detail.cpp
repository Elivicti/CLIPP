#include "../include/CLI++/detail.hpp"
#include <cstring>

#ifdef __REGEX_SPLIT
#include <regex>
#endif

__CLIPP_begin namespace detail {

//////////// String To Argv ////////////
/**
 * this section of code was written by:
 * @author Bruce@stackoverflow.com/users/307917
 * @post   https://stackoverflow.com/a/2568792
 * modifications were made to fit C++ code style and the need of this library.
**/

#ifndef STR_TERMINATE
#define STR_TERMINATE '\0'
#endif
template<typename CharT>
CharT handle_escape(CharT** src_p)
{
	CharT ch = *((*src_p)++);
	// Escape character is always eaten. The next character is sometimes treated specially.
	switch (ch) {
	case 'a': ch = '\a'; break;
	case 'b': ch = '\b'; break;
	case 't': ch = '\t'; break;
	case 'n': ch = '\n'; break;
	case 'v': ch = '\v'; break;
	case 'f': ch = '\f'; break;
	case 'r': ch = '\r'; break;
	}
	return ch;
}
template<typename CharT>
String2ArgvErr copy_raw_string(CharT** dest_p, CharT** src_p)
{
	while(true) {
		CharT ch = *((*src_p)++);

		switch (ch) {
		case STR_TERMINATE: return String2ArgvErr::UNBALANCED_QUOTE;
		case '\'':
			*(*dest_p) = STR_TERMINATE;
			return String2ArgvErr::OK;

		case '\\':
			ch = *((*src_p)++);
			switch (ch)
			{
			case STR_TERMINATE:
				return String2ArgvErr::UNBALANCED_QUOTE;

			default:
				// unknown/invalid escape. Copy escape character.
				*((*dest_p)++) = '\\';
				break;

			case '\\':
			case '\'':
				break;
			}

		default:
			*((*dest_p)++) = ch;
			break;
		}
	}
}
template<typename CharT>
String2ArgvErr copy_cooked_string(CharT** dest_p, CharT** src_p)
{
	while(true) {
		CharT ch = *((*src_p)++);
		switch (ch) {
		case STR_TERMINATE:
			return String2ArgvErr::UNBALANCED_QUOTE;
		case '"':
			*(*dest_p) = STR_TERMINATE;
			return String2ArgvErr::OK;

		case '\\':
			ch = handle_escape(src_p);
			if (ch == STR_TERMINATE)
				return String2ArgvErr::UNBALANCED_QUOTE;

		default:
			*((*dest_p)++) = ch;
			break;
		}
	}
}


std::vector<String> split_token(const String& str, String2ArgvErr* err_p)
{
	String2ArgvErr err = String2ArgvErr::OK;
	using Char = String::value_type;

	std::vector<String> ret;
	ret.reserve(10);

	std::unique_ptr<Char, void (*)(void*)> pstr(strdup(str.data()), std::free);
	Char* scan = pstr.get();

	while (std::isspace((unsigned char)*scan)) scan++;
	Char* dest  = scan;
	Char* token = scan;

	// constexpr Char round_bracket_beg[2] = { '(', '\0' };
	// constexpr Char round_bracket_end[2] = { ')', '\0' };

	bool done = false;
	while(!done && (err == String2ArgvErr::OK))
	{
		while (std::isspace((unsigned char)*scan)) scan++;
		if (*scan == STR_TERMINATE)
			break;

		token = dest = scan;

		bool token_done = false;
		// int round_bracket = 0;
		while(!token_done && (err == String2ArgvErr::OK))
		{
			Char ch = *(scan++);
			switch (ch)
			{
			case STR_TERMINATE:
				done = true;
				token_done = true;
				break;

			case '\\':
				if ((*(dest++) = *(scan++)) == STR_TERMINATE)
				{
					done = true;
					token_done = true;
				}
				break;

			case '\'':
				err = copy_raw_string(&dest, &scan);
				break;

			case '"':
				err = copy_cooked_string(&dest, &scan);
				break;

			case ')':
				// round_bracket++;
				// continue;
			case '(':
				// ret.emplace_back(round_bracket_beg);
				// fallthrough
			case ' ':
			case '\t':
			case '\n':
			case '\f':
			case '\r':
			case '\v':
			case '\b':
				token_done = true;
				break;
			default:
				*(dest++) = ch;
			}
		}
		*dest = STR_TERMINATE;
		if (*token != STR_TERMINATE)
			ret.emplace_back(token);
		// while (round_bracket--)
		// 	ret.emplace_back(round_bracket_end);
	}

	if (err_p != nullptr)
		*err_p = err;

	return ret;
}
//////////// String To Argv ////////////


///////// old one uses regular expression, can't deal with escaped characters
#ifdef __REGEX_SPLIT
std::vector<String> string_to_argv(const String& cmd)
{
	std::vector<String> argv;
	static const std::regex reg(
		R"(([^\s'"]([^\s'"]*(['"])((?!\3)[^]*?)\3)+[^\s'"]*)|[^\s'"]+|(['"])((?!\5)[^]*?)\5)",
		std::regex_constants::icase | std::regex_constants::ECMAScript
	);
	static const int chosen[] = { 1, 6, 0 };	// we are only interested in these parts of the match
	auto it = cmd.begin();
	std::smatch match;
	while (std::regex_search(it, cmd.end(), match, reg))
	{
		it = match[0].second;
		for (int i = 0; i < 3; i++)
		{
			if (match[chosen[i]].length() != 0)
			{
				argv.push_back(match[chosen[i]].str());
				break;
			}
		}
	}
	return argv;
}
#endif //! __REGEX_SPLIT

} __CLIPP_end