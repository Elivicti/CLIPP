#ifndef __CLIPP_INTERACTIVE_HEADER__
#define __CLIPP_INTERACTIVE_HEADER__

#include "Exceptions.hpp"
#include "detail.hpp"

#include <map>
#include <functional>
#include <fmt/color.h>
#include <sstream>

__CLIPP_begin

using ArgList = std::vector<StringView>;
using TokenSpliterFunction = std::vector<String> (*)(const String&, detail::String2ArgvErr*);

class CLI;

/** @brief Callable type for commands, must have `(CLI&, const ArgList&)` as argument types
 *         and `int` as return type. */
template<typename Func>
concept CommandHandler = std::is_invocable_r_v<int, Func, CLI&, const ArgList&>;

class CLICommand
{
public:
	CLICommand(const String& cmd, const String& desc = String())
		: cmd(cmd), desc(desc), options() {}
	virtual ~CLICommand() = default;

	const String& name() const { return cmd; }
	const String& description() const { return desc; }
	String& name() { return cmd; }
	String& description() { return desc; }

	/**
	 * @brief Try to match the text with this command (which is usually partially matched),
	 *        this method also tries to match its options or sub commands.
	 * @param text text to be matched
	 * @param len length of the text
	 * @return C-Style string pointer, if not matched, nullptr is returned.
	 * @note If you want to reimplement this virtual function, you should make sure that
	 *       returned string pointer is created through `malloc` or `strdup`.
	**/
	virtual char* match(const char* text, int len) const;
	/**
	 * @brief Create a usage string.
	 * @details Generated format:
	 * sub commands:
	 *   <sub-command-name>  <sub-command-desc>
	 *   ...
	 * options:
	 *   -<short-name>, --<long-name>  <option-desc>
	 *   ...
	 * @return Created usage string, if a command has no sub command or option, an empty string is returned.
	 * @note If a command has any sub command or option, generated string will always have a `\\n` at the end.
	**/
	virtual String usage() const;

	/**
	 * @brief Add an option to this command.
	 * @note  Options are only used in completion and to generate usage string, CLI won't check if input meet
	 *        command's requirement.
	 * @param opt option name
	 * @param short_name short option name
	 * @param desc description for this option
	**/
	void addOption(const String& opt, char short_name = '\0', const String& desc = String());
	/**
	 * @brief Remove an option from this command.
	 * @param opt option name
	**/
	void removeOption(const String& opt);
	/**
	 * @brief Remove an option from this command.
	 * @param short_name short option name, if equals to `0`, this method does nothing
	**/
	void removeOption(char short_name);
	/**
	 * @brief Add a sub command to this command.
	 * @note  Sub commands are only used in completion and to generate usage string, CLI won't check if input meet
	 *        command's requirement.
	 * @param opt sub command name
	 * @param desc description for this sub command
	**/
	void addSubCommand(const String& subcmd, const String& desc = String());
	/**
	 * @brief Remove a sub command from this command.
	 * @param subcmd sub command name
	**/
	void removeSubCommand(const String& subcmd);
protected:
	int cursorPos() const { return pos; }
	struct OptionType
	{
		String name;
		char short_name;
		String desc;

		// if either of the name or short name is same, options should be considered the same
		bool operator==(const OptionType& opt) const
		{
			return name == opt.name
				|| ((short_name == opt.short_name) && (short_name != 0));
		}
		bool operator<(const OptionType& opt) const
		{ return name < opt.name; }
	};

	std::vector<OptionType> options;
	std::vector<OptionType> subcmds;
private:
	String cmd;
	String desc;

	static int pos;	//used to determine whether to complete command or its arguments

	friend char** command_completion(const char* text, int start, int end);
public:
	bool operator==(const CLICommand& other) const { return cmd == other.cmd; }
	bool operator< (const CLICommand& other) const { return cmd <  other.cmd; }

	virtual int operator()(CLI& cli, const ArgList& args) const = 0;
};

template<CommandHandler Func>
class CLICommandGeneric : public CLICommand
{
public:
	CLICommandGeneric(const String& cmd, Func&& f, const String& desc = String())
		: CLICommand(cmd, desc), fn(f) {}
	virtual ~CLICommandGeneric() = default;
private:
	Func fn;
public:
	virtual int operator()(CLI& cli, const ArgList& args) const override
	{ return std::invoke(fn, cli, args); }
};

class Pipeline
{
public:
	using std_stringstream = std::basic_stringstream<CharType>;
	using std_iostream = std::basic_iostream<CharType>;
	using std_ostream = std::basic_ostream<CharType>;
	using std_istream = std::basic_istream<CharType>;
public:
	/**
	 * @brief Create a `Pipeline` object, uses specified stream objects as buffer.
	 * @note  The caller need to ensure the life time of these two stream objects.
	**/
	Pipeline(std_iostream* buffer1, std_iostream* buffer2);
	/** @brief Create a `Pipeline` object, uses `std::stringstream` objects as buffer. */
	Pipeline() : Pipeline(nullptr, nullptr) {};
	virtual ~Pipeline();

	/**
	 * @brief Call stream obejcts' `clear` method, if it's a `std::stringstream`,
	 *        clear its contents as well.
	**/
	void clearAll();

	/** @brief Open the pipeline, do some initialization work. */
	virtual void open();
	/** @brief Close the pipeline. */
	virtual void close();

	/** @brief Return if the pipeline is opened. */
	bool opened() const { return is_opened; }

	/**
	 * @brief Swap working input stream object, see `CLI::runPipeline`'s implementation
	 *        for detail. Do nothing if pipeline is closed.
	 * @note  Only when you need to reimplement this method, do you need to care about it
	**/
	virtual void swapWorkingInput();
	/**
	 * @brief Swap working output stream object, see `CLI::runPipeline`'s implementation
	 *        for detail. Do nothing if pipeline is closed.
	 * @note  Only when you need to reimplement this method, do you need to care about it
	**/
	virtual void swapWorkingOutput();
protected:
	struct StreamBuffer
	{
		std_iostream* stream;
		const bool is_external;

		void clear()
		{
			if (auto ss = dynamic_cast<std_stringstream*>(stream))
				ss->str(detail::StringConstant<>);
			stream->clear();
		}
		void flush() { stream->flush(); }
	};

private:
	StreamBuffer buffer1;
	StreamBuffer buffer2;

	struct {
		std_ostream* out;
		std_istream* in;
	} working;

	bool is_opened;
public:
	template<typename T>
	std_istream& operator>>(T& value) { return (*working.in) >> value; }
	std_istream& getline(String& str) { return std::getline(*working.in, str); }
	std_istream& get() { return *working.in; }

	/**
	 * @brief  Write string to working output stream, if pipeline is closed, throw an exception.
	 * @throws `CLIException` trying to write to a closed pipe
	 * @return Reference to the currently working output stream.
	**/
	std_ostream& write(const String& str);
};

class CLI
{
public:
	CLI(const String& prompt = String("CLI> "), char completion_key = '\t', TokenSpliterFunction spliter = detail::split_token);
	virtual ~CLI();

	/**  @brief Start CLI main loop. */
	virtual int exec();

	void setPrompt(const String& prompt) { this->prompt = prompt; };

	/**
	 * @brief Create a command with specific name, description and action.
	 * @note  If a command with same name already exists, the old one will be deleted.
	 * @tparam Func Func(CLI&, const ArgList&) -> int
	 * @param name command name
	 * @param f function to be called
	 * @param desc description for this command
	**/
	template<CommandHandler Func>
	void insertCommand(const String& name, Func&& f, const String& desc = String())
	{
		CLICommand* command = new CLICommandGeneric(name, std::forward<Func>(f), desc);
		this->insertCommand(command);
	}
	/**
	 * @brief Insert a CLICommand or its derived class intance.
	 * @note  This method will take pointer's ownership, and delete it in destructor.
	 *        If a command with same name already exists, the old one will be deleted.
	 * @param command pointer to a CLICommand or its derived class intance, must be created with new operator
	**/
	void insertCommand(CLICommand* command)
	{
		if (CLICommand* old = this->take(command->name()))
			delete old;
		commands.emplace(command->name(), command);
		this->updateHelp(command);
	}

	/**
	 * @brief Checks if CLI contains a command with specific name.
	 * @param name command name to match
	**/
	bool contains(const String& name) const { return commands.contains(name); }

	/**
	 * @brief Remove a CLICommand or its derived class intance pointer from CLI, and its ownership.
	 * @note  This method will not delete the instance.
	 * @param name command name to match
	 * @return return nullptr if command with specified name does not exists
	**/
	CLICommand* take(const String& name)
	{
		if (!commands.contains(name))
			return nullptr;
		CLICommand* ret = commands.at(name);
		commands.erase(name);
		return ret;
	}
	/**
	 * @brief Find command by name
	 * @param name command name to be matched
	 * @return Pointer to the command found, if no matching command, nullptr is returned.
	**/
	CLICommand* command(const String& name)
	{
		if (!commands.contains(name))
			return nullptr;
		return commands.at(name);
	}
	const CLICommand* command(const String& name) const
	{
		if (!commands.contains(name))
			return nullptr;
		return commands.at(name);
	}
public: // pipeline supported i/o
	/**
	 * @brief Print message to stdout, if pipeline is opened (i.e used `|` in command line),
	 *        formatted message is sent to pipeline.
	 * @param fmt  format string
	 * @param args format args
	**/
	template<typename ...Args>
	void print(fmt::format_string<Args...>&& fmt, Args&& ... args) const
	{
		if (pipeline.opened())
		{
			pipeline.write(fmt::format(fmt, std::forward<Args>(args)...));
			return;
		}
		fmt::print(fmt, std::forward<Args>(args)...);
	}
	/**
	 * @brief Get a std::istream object reference, if pipeline is opened (i.e used `|` in command line),
	 *        contents are get from the pipeline.
	 * @note  This method basically does the same thing as std::cin or std::wcin, but supports pipeline.
	**/
	Pipeline::std_istream& get()
	{
		return pipeline.get();
	}
	/**
	 * @brief Read data from input, if pipeline is opened (i.e used `|` in command line),
	 *        contents are get from the pipeline.
	 * @note   basically does the same thing as std::cin or std::wcin, but supports pipeline.
	 * @param args variables to store data
	**/
	template<typename ...Args>
	Pipeline::std_istream& get(Args& ... args)
	{
		return (pipeline >> ... >> args);
		// return pipeline.get(std::forward<Args...>(args...));
	}
	/**
	 * @brief Get a line from stdin, if pipeline is opened (i.e used `|` in command line),
	 *        contents are get from the pipeline.
	 * @note  This method basically does the same thing as std::getline(), but supports pipeline.
	 * @param line string to store the data of the line
	**/
	Pipeline::std_istream& getline(String& line)
	{
		return pipeline.getline(line);
	}

	/**
	 * @brief Print message to stderr.
	 * @note  This method won't pass content to pipeline in any case.
	 * @param fmt  format string
	 * @param args format args
	**/
	template<typename ...Args>
	void printStderr(fmt::format_string<Args...>&& fmt, Args&& ... args) const
	{
		fmt::print(stderr, std::forward<fmt::format_string<Args...>>(fmt), std::forward<Args>(args)...);
	}
public:	// predefined commands
	/** @brief Exit CLI exec loop with given code. Do nothing if called outside exec loop. */
	int exit(int code = 0) const;

	/** @brief Just an echo. */
	int echo(const ArgList& args) const;
	/** @brief List all available commands or print help for specified command. */
	int help(const ArgList& args) const;
	/** @brief Clear screen. */
	int clearScreen() const;
protected:
	using TokenList = std::vector<String>;
	struct PipelineRange
	{
		TokenList::const_iterator start;
		TokenList::const_iterator end;
	};
	/**
	 * @brief Parse tokens, split them into sub ranges, each range represents a complete pipeline.
	 * @note Any syntax error should be checked and handled in this method.
	 * @throws `CLICommandParseError` represents an error during parsing tokens.
	 * @param tokens tokens splited by `TokenSpliterFunction`
	 * @return Sub ranges of token list, each range represents a complete pipeline. Ranges are seperated
	 *         by operator `&&` or `||`, but not `|`. `PipelineRange::end` member is an iterator that
	 *         points to the operator, except the last range.
	**/
	virtual std::vector<PipelineRange> parse(const TokenList& tokens);
	/**
	 * @brief Execute pipelines one by one, the short circuit effect of operator `&&` or `||` will work.
	 * @param cmds sub ranges returned by `CLI::parse`
	 * @return The overall return code of all pipelines.
	**/
	virtual int execute(const std::vector<PipelineRange>& cmds);
	/**
	 * @brief Execute pipeline.
	 * @param pipe a `PipelineRange` parsed by `CLI::parse`
	 * @return The overall return code of this pipelines
	**/
	virtual int runPipeline(const PipelineRange& _pipe);

	int last_return_code;
	mutable Pipeline pipeline;
private:
	using CLICommandMap = std::map<StringView, CLICommand*>;

	static CLI* cli_instance;
	static CLI* instance() { return cli_instance; }

	friend char*  command_generator(const char* text, int state);
	friend char** command_completion(const char* text, int start, int end);

	void init(char completion_key = '\t');
	void exitImpl(const ArgList& args) const;
	void updateHelp(const CLICommand* cmd);
private:
	bool in_exec_loop;
	String prompt;
	CLICommandMap commands;

	TokenSpliterFunction token_spliter;
};

template <typename T>
FMT_CONSTEXPR auto styled(const T& value, fmt::text_style ts)
	-> detail::StyledArg<fmt::remove_cvref_t<T>>
{
	return detail::StyledArg<fmt::remove_cvref_t<T>>{value, ts};
}

__CLIPP_end
#endif //! __CLIPP_INTERACTIVE_HEADER__