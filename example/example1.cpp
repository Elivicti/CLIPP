#include "../include/CLI++/CLI++.hpp"
#include <filesystem>

SET_CLIPP_ALIAS(CLI);

int main(int argc, const char** argv)
{
	CLI::String user{"USER"};
	CLI::String dir{std::filesystem::current_path().string()};
	auto get_prompt = [&user](){
		return fmt::format(
			"{}:{}$ "
			, CLI::styled(user, fmt::fg(fmt::rgb(0x16c60c)))
			, CLI::styled(std::filesystem::current_path().string(), fmt::fg(fmt::rgb(0x3b78ff)))
		);
	};
	CLI::CLI app{get_prompt()};
	app.insertCommand("pwd", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("pwd: {}\n", std::filesystem::current_path().string());
		return 0;
	}, "print working directory");
	app.insertCommand("cd", [&user, &dir, &get_prompt](CLI::CLI& cli, const CLI::ArgList& args) {
		if (args.size() < 2)
			return 0;
		if (!std::filesystem::is_directory(args[1]))
		{
			cli.printStderr("cd: {} is not a directory.\n", args[1]);
			return 1;
		}
		std::filesystem::current_path(args[1]);
		cli.setPrompt(get_prompt());
		return 0;
	}, "change working directory");

	app.insertCommand("pipe0", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("-pipe0----------\n");
		cli.print("this is a line\n");
		return 0;
	}, "pipeline test: 0");
	app.insertCommand("pipe1", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("-pipe1----------\n");
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
	}, "pipeline test: 1");

	app.insertCommand("pipe2", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("-pipe2----------\n");
		std::string str;
		int a = 0;
		while (cli.get() >> str)
		{
			cli.print("{:?}\n", str);
			a++;
		}
		cli.print("total: {}\n", a);
		return 0;
	}, "pipeline test: 2");

	app.insertCommand("ret0", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("return 0;\n");
		return 0;
	} , "operator test: return 0");
	app.insertCommand("ret1", [](CLI::CLI& cli, const CLI::ArgList& args) {
		cli.print("return 1;\n");
		return 1;
	} , "operator test: return 1");

	int ret = app.exec();
	fmt::print("CLI returned with code: {}\n", ret);
	return ret;
}