#include "../include/CLI++/CLI++.hpp"
#include <filesystem>

SET_CLIPP_ALIAS(CLI);

int main(int argc, const char** argv)
{
	CLI::String name{"USER"};
	CLI::String dir{"~/CLI++"};
	CLI::String colored_prompt = fmt::format(
		"{}:{}$ "
		, CLI::styled(name, fmt::fg(fmt::rgb(0x16c60c)))
		, CLI::styled(dir,  fmt::fg(fmt::rgb(0x3b78ff)))
	);
	CLI::CLI app{colored_prompt};
	app.insertCommand("pwd", [](CLI::CLI*, const CLI::ArgList& args) {
		fmt::print("pwd: {}\n", std::filesystem::current_path().string());
	}, "print working directory");

	int ret = app.exec();
	fmt::print("CLI returned with code: {}\n", ret);
	return ret;
}