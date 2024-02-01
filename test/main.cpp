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
	app.insertCommand("pwd", [](CLI::CLI&, const CLI::ArgList& args) {
		fmt::print("pwd: {}\n", std::filesystem::current_path().string());
		return 0;
	}, "print working directory");

	app.insertCommand("test0", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("this is a line\n");
		return 0;
	} , "test 0");
	app.insertCommand("test", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("----------------\n");
		std::string str;
		int a = 0;
		for (int i = 0; i < 2; i++)
		{
			cli.get(str);
			cli.print("{}\n", str);
			a++;
		}
		cli.print("total: {}\n", a);
		return 0;
	} , "test");

	app.insertCommand("test1", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("-test1-----------\n");
		std::string str;
		int a = 0;
		while (cli.get() >> str)
		{
			cli.print("{:?}\n", str);
			a++;
		}
		cli.print("total: {}\n", a);
		return 0;
	} , "test 1");

	app.insertCommand("ret0", [](CLI::CLI&, const CLI::ArgList& args) {
		fmt::print("return 0;\n");
		return 0;
	} , "return 0");
	app.insertCommand("ret1", [](CLI::CLI&, const CLI::ArgList& args) {
		fmt::print("return 1;\n");
		return 1;
	} , "return 1");

	int ret = app.exec();
	fmt::print("CLI returned with code: {}\n", ret);
	return ret;
}