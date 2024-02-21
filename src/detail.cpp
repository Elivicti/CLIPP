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
CharT handle_escape(const CharT*& src_p)
{
	CharT ch = *(src_p++);
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
CliSyntaxError copy_raw_string(CharT*& dest_p, const CharT*& src_p)
{
	while(true) {
		CharT ch = *(src_p++);

		switch (ch) {
		case STR_TERMINATE: return {CliSyntaxError::UNBALANCED_QUOTE, '\''};
		case '\'':
			*dest_p = STR_TERMINATE;
			return CliSyntaxError::OK;

		case '\\':
			ch = *(src_p++);
			switch (ch)
			{
			case STR_TERMINATE:
				return {CliSyntaxError::UNBALANCED_QUOTE, '\''};

			default:
				// unknown/invalid escape. Copy escape character.
				*(dest_p++) = '\\';
				break;

			case '\\':
			case '\'':
				break;
			}

		default:
			*(dest_p++) = ch;
			break;
		}
	}
}
template<typename CharT>
CliSyntaxError copy_cooked_string(CharT*& dest_p, const CharT*& src_p)
{
	while(true) {
		CharT ch = *(src_p++);
		switch (ch) {
		case STR_TERMINATE:
			return {CliSyntaxError::UNBALANCED_QUOTE, '"'};
		case '"':
			*dest_p = STR_TERMINATE;
			return CliSyntaxError::OK;

		case '\\':
			ch = handle_escape(src_p);
			if (ch == STR_TERMINATE)
				return {CliSyntaxError::UNBALANCED_QUOTE, '"'};

		default:
			*(dest_p++) = ch;
			break;
		}
	}
}
template<typename CharT>
void handle_operator(CharT*& dest, const CharT*& src)
{
	CharT ch = *src;
	switch (ch)
	{
	case '&':
	case '|':
		*(dest++) = ch;
		src++;
		break;
	default:
		break;
	}
}

std::vector<String> split_token(StringView str, CliSyntaxError* _err)
{
	using Char = String::value_type;
	std::vector<String> ret;
	ret.reserve(10);

	CliSyntaxError err = CliSyntaxError::OK;

	std::unique_ptr<Char, void (*)(void*)> buffer(strdup(str.data()), std::free);
	const Char* scan = str.data();
	Char* dest = buffer.get();
	Char* token = dest;

	while (*scan != STR_TERMINATE && (err == CliSyntaxError::OK))
	{
		while (std::isspace(*scan)) scan++;
		if (*scan == STR_TERMINATE) break;
		dest = token;

		bool token_done = false;
		while (!token_done && (err == CliSyntaxError::OK))
		{
			Char ch = *(scan++);
			switch (ch)
			{
			case STR_TERMINATE:
				scan--;
				token_done = true;
				break;
			// case '\\':
			// 	if (*scan == STR_TERMINATE)	// ignore last invalid escape
			// 		token_done = true;
			// 	break;

			case '\'':
				err = copy_raw_string(dest, scan);
				break;

			case '"':
				err = copy_cooked_string(dest, scan);
				break;

			case '&':
			case '|':
			// case '(':
			// case ')':
				if (token != dest)
					ret.emplace_back(token, dest);
				*(dest = token) = ch;
				handle_operator(++dest, scan);

			case '(':
			case ')':
				// don't handle '(' and ')' currently, treat them as token end
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
		if (token != dest)
			ret.emplace_back(token, dest);
	}

	if (_err != nullptr) *_err = err;

	ret.shrink_to_fit();
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