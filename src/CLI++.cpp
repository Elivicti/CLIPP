#include "../include/CLI++/CLI++.hpp"
#include "../include/CLI++/StringHelper.hpp"

#include <readline/readline.h>
#include <readline/history.h>

__CLIPP_begin

char* command_generator(const char* text, int state)
{
	static int len = 0;
	static CLI::CmdCallBackMap::iterator begin, end;
	
	/* If this is a new word to complete, initialize now. This includes
	 * saving the length of TEXT for efficiency, and initializing the index
	 * variable to 0. */
	if (state == 0)
	{
		begin = CLI::cli_instance->commands.begin();
		end   = CLI::cli_instance->commands.end();
		len = strlen(text);
	}

	/* Return the next name which partially matches from the command list. */
	while (begin != end)
	{
		auto& [ cmd, func ] = *begin;
		++begin;

		if (strncmp(cmd.name().data(), text, len) == 0)
			return strdup(cmd.name().data());
	}
	return ((char *)NULL);
}
char** command_completion(const char* text, int start, int end)
{
	char** matches = (char**)NULL;
	/* If this word is at the start of the line, then it is a command
	 * to complete. Otherwise it is the name of a file in the current directory. */
	if (start == 0)
		matches = rl_completion_matches(text, command_generator);
	return (matches);
}

class CLIExcepionExit : public CLIExcpetion
{

public:
	CLIExcepionExit(int _code = 0) noexcept
		: CLIExcpetion("This CLIExcpetion is called to exit CLI with return code."), _code(_code) {}
	virtual ~CLIExcepionExit() noexcept = default;

	int code() const { return _code; }
private:
	int _code;
};

CLI* CLI::cli_instance = nullptr;

CLI::CLI(const std::string& prompt, char completion_key)
	: prompt(prompt)
{
	if (cli_instance != nullptr)
		throw CLIExcpetion("CLI can ONLY have one instance.");
	cli_instance = this;

	rl_bind_key(completion_key, rl_complete);
	
	rl_attempted_completion_function = command_completion;

	commands.insert(std::make_pair("cd", [](const std::vector<std::string>&) { fmt::print("chdir!\n"); }));
	commands.insert(std::make_pair("help", [](const std::vector<std::string>&) { fmt::print("help!\n"); }));
	commands.insert(
		std::make_pair("echo",
		[](const std::vector<std::string>& args) {
			size_t count = args.size();
			for (size_t i = 1; i < count; i++)
				fmt::print("argv[{}] = {:?}\n", i, args[i]);
		})
	);
	commands.insert(
		std::make_pair("clear",
		[](const std::vector<std::string>& args) {
		})
	);
	commands.insert(
		std::make_pair("exit", [](const std::vector<std::string>& args) { throw CLIExcepionExit(0); })
	);
	
}

int CLI::mainloop()
{
	int ret_code = 0;
	while (true)
	{
		std::unique_ptr<char> raw_input;
		// use unique_ptr to handle the pointer in case an exception shows up while handling commands
		raw_input.reset(readline(prompt.data()));
		if (!raw_input)
			break;

		std::string input(raw_input.get());
		add_history(input.data());

		// TODO:
		// [DONE] remove leading and trailing spaces in the input
		// [DONE] parse the input into some format
		//        -> std::vector<std::string>

		try
		{
			const auto& argv = str_to_argv(input);

			if (commands.contains(argv[0]))
				commands.at(argv[0])(argv);
			else if (!input.empty())
				fmt::print("{}: No such command\n", argv[0]);
		}
		catch(const std::exception& e)
		{
			if (auto* exit = dynamic_cast<const CLIExcepionExit*>(&e))
			{
				ret_code = exit->code();
				break;
			}
			fmt::print("{}\n", e.what());
		}
	}
	return ret_code;
}

__CLIPP_end