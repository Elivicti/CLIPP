#include "../include/CLI++/StringHelper.hpp"

#include <regex>

__CLIPP_begin

std::vector<std::string> str_to_argv(const std::string& cmd)
{
	std::vector<std::string> argv;
	
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

__CLIPP_end