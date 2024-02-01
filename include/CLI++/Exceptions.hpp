#ifndef __CLIPP_EXCEPTION_HEDER__
#define __CLIPP_EXCEPTION_HEDER__

#include "defines.hpp"
#include <exception>
#include <fmt/format.h>

__CLIPP_begin

class CLIException : public std::exception
{
public:
	CLIException(const std::string& msg) noexcept
		: msg(msg) {}
	CLIException(const char* msg) noexcept
		: msg(msg) {}
	virtual ~CLIException() noexcept = default;

	virtual const char* what() const noexcept { return msg.data(); }

private:
	std::string msg;
};

class CLICommandParseError: public CLIException
{
public:
	template<typename ...Args>
	CLICommandParseError(fmt::format_string<Args...> fmt, Args&&... args) noexcept
		: CLIException(fmt::format(fmt, std::forward<Args>(args)...)) {}
	virtual ~CLICommandParseError() noexcept = default;

};

__CLIPP_end
#endif //! __CLIPP_EXCEPTION_HEDER__