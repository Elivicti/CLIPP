#ifndef __CLIPP_ARGPARSER_HEADER__
#define __CLIPP_ARGPARSER_HEADER__

#include "defines.hpp"
#include <string>
#include <vector>

__CLIPP_begin

class ArgParser
{
public:
	void parse(int argc, char** argv);
	void parse(const std::vector<std::string>& args);

private:

};

__CLIPP_end
#endif //! __CLIPP_ARGPARSER_HEADER__