#ifndef __CLIPP_INTERACTIVE_HEADER__
#define __CLIPP_INTERACTIVE_HEADER__

#include "defines.hpp"
#include "Exceptions.hpp"
#include "ArgParser.hpp"

#include <map>
#include <functional>

__CLIPP_begin

class CLICommand
{
public:
	CLICommand(const std::string& cmd, const std::string& doc = std::string())
		: cmd(cmd), doc(doc) {}

	const std::string& name() const { return cmd; }
	const std::string& desc() const { return doc; }
	std::string& name() { return cmd; }
	std::string& desc() { return doc; }

private:
	std::string cmd;
	std::string doc;

public:
	bool operator==(const CLICommand& other) const { return cmd == other.cmd; }
	bool operator< (const CLICommand& other) const { return cmd <  other.cmd; }
};


class CLI
{
public:
	using CommandHandler = std::function<void(const std::vector<std::string>&)>;
public:
	CLI(const std::string& prompt, char completion_key = '\t');

	int mainloop();

	void setPrompt(const std::string& prompt) { this->prompt = prompt; };

	void insertCommand(const std::string& cmd, CommandHandler f, const std::string& doc = std::string())
	{
		commands.insert_or_assign(CLICommand{cmd, doc}, f);
	}

private:
	using CmdCallBackMap = std::map<CLICommand, CommandHandler>;

	static CLI* cli_instance;
	static CLI* instance() { return cli_instance; }

	friend char*  command_generator(const char *text, int state);
	friend char** command_completion(const char *text, int start, int end);
private:

	std::string prompt;
	CmdCallBackMap commands;
};

__CLIPP_end
#endif //! __CLIPP_INTERACTIVE_HEADER__