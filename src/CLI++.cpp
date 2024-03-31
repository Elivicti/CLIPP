#include "../include/CLI++/CLI++.hpp"
#include <iostream>
#include <ranges>

#include <readline/readline.h>
#include <readline/history.h>

CLIPP_BEGIN
/////////////////   Constant   /////////////////
static auto CMDAND  = detail::StringConstant<'&', '&'>;
static auto CMDOR   = detail::StringConstant<'|', '|'>;
static auto CMDPIPE = detail::StringConstant<'|'>;

///////////////// Readline API /////////////////
char* command_generator(const char* text, int state)
{
	static std::size_t len = 0;
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
class CLIExceptionExit : public CLIException
{
public:
	CLIExceptionExit(int _code = 0) noexcept
		: CLIException("This CLIException is called to exit CLI with return code."), _code(_code) {}
	virtual ~CLIExceptionExit() noexcept = default;

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
	std::basic_ostringstream<CharType> ss;
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

//////////////////  Pipeline  //////////////////
template<typename CharT>
static std::basic_istream<CharT>& get_stdin_stream();
template<> std::basic_istream<char>&    get_stdin_stream()
{ return std::cin; }
template<> std::basic_istream<wchar_t>& get_stdin_stream()
{ return std::wcin; }

Pipeline::Pipeline(std_iostream* buffer1, std_iostream* buffer2)
	: buffer1{ buffer1 == nullptr ? new std_stringstream : buffer1, buffer1 != nullptr }
	, buffer2{ buffer2 == nullptr ? new std_stringstream : buffer2, buffer2 != nullptr }
	, working{ nullptr, &get_stdin_stream<CharType>() }
	, is_opened(false) {}

Pipeline::~Pipeline()
{
	if (!buffer1.is_external)
		delete buffer1.stream;
	if (!buffer2.is_external)
		delete buffer2.stream;
}

void Pipeline::clearAll()
{
	buffer1.clear();
	buffer2.clear();
}

Pipeline::std_ostream& Pipeline::write(const String& str)
{
	if (working.out == nullptr)
		throw CLIException("trying to write to a closed pipe");
	return (*working.out) << str;
}

void Pipeline::open()
{
	clearAll();
	working.out = buffer2.stream;
	working.in = &get_stdin_stream<CharType>();
	is_opened = true;
}
void Pipeline::close()
{
	working.out = nullptr;
	// note that we don't set working.in to default, because at this point,
	// the pipe is only half closed: working.out reaches the end (means it needs to be stdout now),
	// but working.in still holds the data from last command output, which may be needed.
	is_opened = false;
}
void Pipeline::swapWorkingInput()
{
	if (!this->opened())
		return;

	if (working.in != buffer1.stream)
		working.in = buffer1.stream;
	else
		working.in = buffer2.stream;
}
void Pipeline::swapWorkingOutput()
{
	if (!this->opened())
		return;

	if (working.out != buffer1.stream)
	{
		buffer1.clear();
		working.out = buffer1.stream;
	}
	else
	{
		buffer2.clear();
		working.out = buffer2.stream;
	}
}

//////////////////    CLI     //////////////////
CLI* CLI::cli_instance = nullptr;
void CLI::init(char completion_key)
{
	if (cli_instance != nullptr)
		throw CLIException("CLI can ONLY have one instance.");
	cli_instance = this;

	rl_bind_key(completion_key, rl_complete);
	rl_attempted_completion_function = command_completion;

	commands.emplace("help", new CLICommandGeneric("help", [](CLI& cli, const ArgList& args) {
		return cli.help(args);
	}, "list all available commands or print help for specified command"));
	commands.emplace("echo", new CLICommandGeneric("echo", [](CLI& cli, const ArgList& args) {
		return cli.echo(args);
	}, "just an echo"));
	commands.emplace("clear", new CLICommandGeneric("clear", [](CLI& cli, const ArgList&) {
		return cli.clearScreen();
	}, "clear screen"));
	commands.emplace("exit", new CLICommandGeneric("exit", [](CLI& cli, const ArgList& args) {
		cli.exitImpl(args); return -1;
	}, "exit cli with return code, if not specified, return 0"));

	auto cmd_help = commands.at("help");
	for (auto& [ cmd_name, cmd ] : commands)
		cmd_help->addSubCommand(String(cmd_name), cmd->description());
	// fmt::print("{}: {}\n{}\n", cmd_help->name(), cmd_help->description(), cmd_help->usage());
}

CLI::CLI(const String& prompt, char completion_key, TokenSpliterFunction spliter)
	: last_return_code(0), pipeline()
	, in_exec_loop(false), prompt(prompt), token_spliter(spliter)
{
	this->init(completion_key);
}
// CLI::CLI(Pipeline::std_iostream& stream, const String& prompt, char completion_key)
// 	: in_exec_loop(false), prompt(prompt), last_return_code(0), pipeline(stream)
// { this->init(); }

CLI::~CLI()
{
	for (auto& [name, cmd] : commands)
	{
		delete cmd;
	}
}

int CLI::exit(int code) const
{
	if (in_exec_loop)
		throw CLIExceptionExit(code);
	return code;
}
void CLI::exitImpl(const ArgList& args) const
{
	int code = 0;
	if (args.size() > 1)
	{
		try { code = std::stoi(String(args[1])); }
		catch(const std::exception&)
		{
			throw CLIException("Invalid exit code.");
		}
	}
	this->exit(code);
}
int CLI::echo(const ArgList& args) const
{
	// fmt::print("{}\n", fmt::join(args.begin() + 1, args.end(), " "));
	size_t count = args.size();
	for (size_t i = 0; i < count; i++)
		print("argv[{}] = {}\n", i, args[i]);
	return 0;
}
int CLI::help(const ArgList& args) const
{
	const auto& cmds = this->commands;
	if (args.size() < 2)
	{
		print("available commands:\n");
		auto range = std::views::transform(cmds, [](const auto& pair) { return pair.first; });
		print("{}\n", fmt::join(range.begin(), range.end(), "  "));
		return 0;
	}
	auto& cmd = args[1];
	if (!cmds.contains(cmd))
	{
		printStderr("help: Unkown command \"{}\"\n", cmd);
		return 1;
	}
	auto pcmd = cmds.at(cmd);
	print("{}: {}\n{}", cmd, pcmd->description(), pcmd->usage());
	return 0;
}
int CLI::clearScreen() const
{
	print("\x1b\x5b\x48\x1b\x5b\x32\x4a");
	return 0;
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

		/**
		 * TODO:
		 *   [DONE] Pipeline buffer
		 *   [DONE] parse pipeline operator
		 *   [DONE] other operators like "&&" and "||"
		**/
		try
		{
			TokenList tokens = detail::split_token(input);
			last_return_code = execute(parse(tokens));
		}
		catch(const CLIExceptionExit& exit) { return exit.code(); }
		catch(const std::exception& e)
		{
			fmt::print("{} {}\n",
				fmt::styled("Error:", fmt::fg(fmt::rgb(0xF14C4C)) | fmt::emphasis::bold),
				e.what());
		}
	}
	in_exec_loop = false;
	return 0;
}

std::vector<CLI::PipelineRange> CLI::parse(const CLI::TokenList& tokens)
{
	using TokenListConstIter = TokenList::const_iterator;
	const TokenListConstIter end = tokens.cend();
	std::vector<PipelineRange> cmds;

	auto is_operator = [](const String& s) {
		return (s == CMDAND) || (s == CMDOR) || (s == CMDPIPE);
	};

	for (TokenListConstIter start = tokens.cbegin(), it = start; it != end;)
	{
		if (!commands.contains(*it))
			throw CLICommandParseError("unrecognized command: {}", *it);
		TokenListConstIter op = std::find_if(it, end, is_operator);
		if (op != end && *op == CMDPIPE)
		{
			it = ++op;
			if (it == end)
				cmds.emplace_back(start, --op);
			continue;
		}
		cmds.emplace_back(start, op);
		if (op == end)
			break;

		start = it = ++op;
	}

	// makesure `&&` or `||` or `|` is not at the end
	if (cmds.back().end != end)
		throw CLICommandParseError("unexpected operator \"{}\" at the end", *cmds.back().end);
	return cmds;
}

int CLI::execute(const std::vector<CLI::PipelineRange>& cmd_list)
{
	std::vector<PipelineRange>::const_iterator ths = cmd_list.cbegin();
	std::vector<PipelineRange>::const_iterator end = cmd_list.cend();
	int ret_code = runPipeline(*ths);

	for (auto lst = ths++; ths != end; ++ths)
	{
		const String& op_token = *lst->end;

		if (op_token == CMDAND)
			ret_code = !(ret_code == 0 && runPipeline(*ths) == 0); // reverse the result because `0` is what means OK
		else if (op_token == CMDOR)
			ret_code = !(ret_code == 0 || runPipeline(*ths) == 0); // ditto
		else
			throw CLICommandParseError("unexpected operator \"{}\"", op_token);
		lst = ths;
	}

	return ret_code;
}

int CLI::runPipeline(const CLI::PipelineRange& _pipe)
{
	ScopeGuard guard{[this]() { pipeline.close(); }};
	/**
	 * pipeline procedure should be something like this:
	 * stdin > (pipe) > buffer1 > (pipe) > buffer2 > (pipe) > buffer1 > (pipe) > buffer2 > (pipe) > stdout
	 *                        ^                  ^                  ^                  ^
	 *    in            out  in            out  in            out  in            out  in            out
	 * buffer1 and buffer2 are used in turns
	**/
	pipeline.open();
	int ret_code = 0;
	for (auto cmd_begin = _pipe.start; cmd_begin != _pipe.end;)
	{
		auto cmd_end = std::find(cmd_begin, _pipe.end, CMDPIPE);
		if (cmd_end != _pipe.end)
			pipeline.swapWorkingOutput();
		else
			pipeline.close();

		const CLICommand* command = commands.at(*cmd_begin);
		ret_code |= std::invoke(*command, *this, ArgList(cmd_begin, cmd_end));

		pipeline.swapWorkingInput();

		if (cmd_end == _pipe.end)
			break;
		cmd_begin = ++cmd_end;
	}

	return ret_code;
}

CLIPP_END