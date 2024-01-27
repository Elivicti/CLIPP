#include "../include/CLI++/CLI++.hpp"
#include "../include/CLI++/detail.hpp"
#include <sstream>
#include <ranges>

#include <readline/readline.h>
#include <readline/history.h>

__CLIPP_begin

///////////////// Readline API /////////////////
char* command_generator(const char* text, int state)
{
	static int len = 0;
	static CLI::CLICommandMap::iterator begin, end;

	// if this is a new word to complete, initialize now.
	// this includes saving the length of TEXT for efficiency, and initializing the iterator.
	if (state == 0)
	{
		begin = CLI::cli_instance->commands.begin();
		end   = CLI::cli_instance->commands.end();
		len = strlen(text);
	}

	// return the next name which partially matches from the command list.
	while (begin != end)
	{
		auto& [ name, cmd ] = *begin;
		++begin;

		if (char* matched = cmd->match(text, len))
			return matched;
	}
	return rl_filename_completion_function(text, state);
}
char** command_completion(const char* text, int start, int end)
{
	char** matches = (char**)NULL;
	// store start position so that CLICommand can find out whether to complete command or arguments
	CLICommand::pos = start;
	matches = rl_completion_matches(text, command_generator);
	return (matches);
}


/////////////// Special Excepion ///////////////
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


////////////////// CLICommand //////////////////
int CLICommand::pos = 0;
char* CLICommand::match(const char* text, int len) const
{
	if (pos == 0 && cmd.compare(0, len, text) == 0)
		return detail::strdup(cmd.data());

	if (pos != 0 && len > 0)
	{
		// search sub commands first
		auto subcmd_it = std::ranges::find_if(
			subcmds,
			[text, len](const OptionType& opt) {
				return opt.name.compare(0, len, text) == 0;
			}
		);
		if (subcmd_it != subcmds.cend())
			return detail::strdup(subcmd_it->name.data());
	}
	if (pos != 0 && len >= 2 && std::strncmp(text, "--", 2) == 0)
	{
		// search for options
		auto opt_it = std::ranges::find_if(
			options,
			[text, len](const OptionType& opt) {
				return opt.name.compare(0, len - 2, text + 2) == 0;
			}
		);
		if (opt_it != subcmds.cend())
			return detail::strdup(String("--" + opt_it->name).data());
	}

	return nullptr;
}
String CLICommand::usage() const
{
	// bool has_subcmd = subcmds.size() > 0;
	// bool has_option = options.size() > 0;

	// if (!has_subcmd && !has_option)
	// 	return "";

	std::ostringstream ss;
	if (subcmds.size() > 0)
	{
		ss << "sub commands:\n";
		std::size_t max_len = std::ranges::max(
			std::views::transform(subcmds,
			[](const OptionType& opt) { return opt.name.size(); }
		));
		for (auto& sub : subcmds)
			ss << fmt::format("  {:<{}} {}\n", sub.name, max_len + 1, sub.desc);
	}

	if (options.size() > 0)
	{
		std::size_t max_len = std::ranges::max(
			std::views::transform(options,
			[](const OptionType& opt) { return opt.name.size(); }
		));

		ss << "options:\n";
		for (auto& opt : options)
			ss << fmt::format("  {:3s} --{:<{}} {}\n"
					, opt.short_name ? fmt::format("-{},", opt.short_name) : ""
					, opt.name, max_len + 1
					, opt.desc);
	}

	return ss.str();
}

void CLICommand::addOption(const String& opt_name, char short_name, const String& desc)
{
	OptionType opt{ opt_name, short_name, desc };
	auto it = std::ranges::find(options, opt);
	if (it == options.end())
		options.insert(it, opt);
	else
		*it = opt;
}
void CLICommand::addSubCommand(const String& subcmd_name, const String& desc)
{
	OptionType cmd{ subcmd_name, 0, desc };
	auto it = std::ranges::find(subcmds, cmd);
	if (it == subcmds.end())
		subcmds.insert(it, cmd);
	else
		*it = cmd;
}
void CLICommand::removeOption(const String& opt_name)
{
	auto it = std::ranges::find_if(
		options, [&opt_name](const OptionType& opt) {
			return opt.name == opt_name;
		}
	);
	if (it != subcmds.end())
		options.erase(it);
}
void CLICommand::removeOption(char short_name)
{
	if (short_name == 0)
		return;
	auto it = std::ranges::find_if(
		options, [short_name](const OptionType& opt) {
			return opt.short_name == short_name;
		}
	);
	if (it != subcmds.end())
		options.erase(it);
}
void CLICommand::removeSubCommand(const String& subcmd)
{
	auto it = std::ranges::find_if(
		subcmds, [&subcmd](const OptionType& opt) {
			return opt.name == subcmd;
		}
	);
	if (it != subcmds.end())
		subcmds.erase(it);
}

//////////////////    CLI     //////////////////
CLI* CLI::cli_instance = nullptr;

CLI::CLI(const String& prompt, char completion_key)
	: in_exec_loop(false), prompt(prompt)
{
	if (cli_instance != nullptr)
		throw CLIExcpetion("CLI can ONLY have one instance.");
	cli_instance = this;

	rl_bind_key(completion_key, rl_complete);
	rl_attempted_completion_function = command_completion;

	commands.emplace("help", new CLICommand("help", [](CLI* cli, const ArgList& args) {
		cli->help(args);
	}, "list all available commands or print help for specified command"));
	commands.emplace("echo", new CLICommand("echo", [](CLI* cli, const ArgList& args) {
		cli->echo(args);
	}, "just an echo"));
	commands.emplace("clear", new CLICommand("clear", [](CLI* cli, const ArgList&) {
		cli->clearScreen();
	}, "clear screen"));
	commands.emplace("exit", new CLICommand("exit", [](CLI* cli, const ArgList& args) {
		cli->exitImpl(args);
	}, "exit cli with return code, if not specified, return 0"));

	auto cmd_help = commands.at("help");
	for (auto& [ cmd_name, cmd ] : commands)
		cmd_help->addSubCommand(cmd_name, cmd->description());
	// fmt::print("{}: {}\n{}\n", cmd_help->name(), cmd_help->description(), cmd_help->usage());
}

CLI::~CLI()
{
	for (auto& [name, cmd] : commands)
	{
		delete cmd;
	}
}

void CLI::exit(int code) const
{
	if (in_exec_loop)
		throw CLIExcepionExit(code);
}
void CLI::exitImpl(const ArgList& args) const
{
	int code = 0;
	if (args.size() > 1)
	{
		try { code = std::stoi(args[1]); }
		catch(const std::exception&)
		{
			throw CLIExcpetion("Invalid exit code.");
		}
	}
	this->exit(code);
}
void CLI::echo(const ArgList& args) const
{
	// fmt::print("{}\n", fmt::join(args.begin() + 1, args.end(), " "));
	size_t count = args.size();
	for (size_t i = 0; i < count; i++)
		fmt::print("argv[{}] = {}\n", i, args[i]);
}
void CLI::help(const ArgList& args) const
{
	const auto& cmds = this->commands;
	if (args.size() < 2)
	{
		fmt::print("available commands:\n");
		for (const auto& [ key, _ ] : cmds)
			fmt::print("{}  ", key);
		fmt::print("\n");
		return;
	}
	auto& cmd = args[1];
	if (!cmds.contains(cmd))
		throw CLIExcpetion("help: No such command");
	else
	{
		auto pcmd = cmds.at(cmd);
		fmt::print("{}: {}\n{}", cmd, pcmd->description(), pcmd->usage());
	}
}
void CLI::clearScreen() const
{
	fmt::print("\x1b\x5b\x48\x1b\x5b\x32\x4a");
}
void CLI::updateHelp(const CLICommand* cmd)
{
	if (!commands.contains("help"))
		return;
	auto* cmd_help = commands.at("help");
	cmd_help->addSubCommand(cmd->name(), cmd->description());
}

int CLI::exec()
{
	in_exec_loop = true;
	while (true)
	{
		char* raw_input = readline(prompt.data());
		if (!raw_input)
			break;
		String input(raw_input);
		free(raw_input);

		if (detail::is_empty_string(input))
			continue;

		add_history(input.data());

		try
		{
			const auto& argv = detail::string_to_argv(input);

			if (commands.contains(argv[0]))
				commands.at(argv[0])->operator()(this, argv);
			else if (!input.empty())
				fmt::print("{}: No such command\n", argv[0]);
		}
		catch(const CLIExcepionExit& exit) { return exit.code(); }
		catch(const std::exception& e)
		{
			fmt::print("{}\n", e.what());
		}
	}
	in_exec_loop = false;
	return 0;
}

__CLIPP_end