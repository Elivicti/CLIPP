#include "../include/CLI++/CLI++.hpp"
#include "../include/CLI++/StringHelper.hpp"

SET_CLIPP_ALIAS(CLI);

int main()
{
	std::string name{"USER"};
	std::string dir{"~/CLI++"};

	std::string colored_prompt = fmt::format(
		"{}:{}$ "
		, CLI::styled(name, fmt::fg(fmt::rgb(0x16c60c)))
		, CLI::styled(dir,  fmt::fg(fmt::rgb(0x3b78ff)))
	);
	CLI::CLI app{colored_prompt};
	int ret = app.mainloop();
	fmt::print("CLI returned with code: {}\n", ret);
	return ret;
}