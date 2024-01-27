#ifndef __CLIPP_INTERACTIVE_HEADER__
#define __CLIPP_INTERACTIVE_HEADER__

#include "defines.hpp"
#include "Exceptions.hpp"
#include "detail.hpp"

#include <map>
#include <functional>
#include <fmt/color.h>

__CLIPP_begin

using ArgList = std::vector<String>;
class CLI;

class CLICommand
{
protected:
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
public:
	using CommandHandler = std::function<void(CLI*, const ArgList&)>;
public:
	CLICommand(const String& cmd, CommandHandler func, const String& desc = String())
		: cmd(cmd), desc(desc), handler(func), options() {}
	virtual ~CLICommand() = default;

	const String& name() const { return cmd; }
	const String& description() const { return desc; }
	String& name() { return cmd; }
	String& description() { return desc; }

	/**
	 * @brief Try to match the text with this command (which is usually partially matched),
	 *        this function also tries to match its options or sub commands.
	 * @param text text to be matched
	 * @param len length of the text
	 * @return C-Style string pointer, if not matched, nullptr is returned.
	 * @note If you want to reimplement this virtual function, you should make sure that
	 *       returned string pointer is created through "malloc" or "strdup".
	**/
	virtual char* match(const char* text, int len) const;
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
	 * @param short_name short option name, if equals to '\0', this function do nothing
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

private:
	String cmd;
	String desc;

	CommandHandler handler;

	std::vector<OptionType> options;
	std::vector<OptionType> subcmds;

	static int pos;	//used to determine whether to complete command or its arguments

	friend char** command_completion(const char* text, int start, int end);
public:
	bool operator==(const CLICommand& other) const { return cmd == other.cmd; }
	bool operator< (const CLICommand& other) const { return cmd <  other.cmd; }

	virtual void operator()(CLI* cli, const ArgList& args) const { handler(cli, args); }
};

class CLI
{
public:
	CLI(const String& prompt = String("CLI> "), char completion_key = '\t');
	~CLI();

	int exec();

	void setPrompt(const String& prompt) { this->prompt = prompt; };

	/**
	 * @brief Create a command with specific name, description and action.
	 * @note  If a command with same name already exists, the old one will be deleted.
	 * @param name command name
	 * @param f function to be called
	 * @param desc description for this command
	**/
	void insertCommand(const String& name, CLICommand::CommandHandler f, const String& desc = String())
	{
		if (commands.contains(name))
			delete commands.at(name);
		auto* command = new CLICommand(name, f, desc);
		this->updateHelp(command);
		commands.insert_or_assign(name, command);
	}
	/**
	 * @brief Insert a CLICommand or its derived class intance.
	 * @note  This function will take pointer's ownership, and delete it in deconstructor.
	 *        If a command with same name already exists, the old one will be deleted.
	 * @param command pointer to a CLICommand or its derived class intance, must be created with new operator
	**/
	void insertCommand(CLICommand* command)
	{
		if (commands.contains(command->name()))
			delete commands.at(command->name());
		this->updateHelp(command);
		commands.insert_or_assign(command->name(), command);
	}

	/**
	 * @brief Checks if CLI contains a command with specific name.
	 * @param name command name to match
	**/
	bool contains(const String& name) const { return commands.contains(name); }

	/**
	 * @brief Remove a CLICommand or its derived class intance pointer from CLI, and its ownership.
	 * @note  This function will not delete the instance.
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

public:
	/** @brief Exit CLI exec loop with given code. Do nothing if called outside exec loop. */
	void exit(int code = 0) const;

	/** @brief Just an echo. */
	void echo(const ArgList& args) const;
	/** @brief List all available commands or print help for specified command. */
	void help(const ArgList& args) const;
	/** @brief Clear screen. */
	void clearScreen() const;
private:
	void exitImpl(const ArgList& args) const;
	void updateHelp(const CLICommand* cmd);
private:
	using CLICommandMap = std::map<String, CLICommand*>;

	static CLI* cli_instance;
	static CLI* instance() { return cli_instance; }

	friend char*  command_generator(const char* text, int state);
	friend char** command_completion(const char* text, int start, int end);
private:
	bool in_exec_loop;
	String prompt;
	CLICommandMap commands;
};

template <typename T>
FMT_CONSTEXPR auto styled(const T& value, fmt::text_style ts)
	-> detail::styled_arg<fmt::remove_cvref_t<T>>
{
	return detail::styled_arg<fmt::remove_cvref_t<T>>{value, ts};
}

__CLIPP_end
#endif //! __CLIPP_INTERACTIVE_HEADER__