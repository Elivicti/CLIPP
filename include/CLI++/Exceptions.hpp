#ifndef __CLIPP_EXCEPTION_HEDER__
#define __CLIPP_EXCEPTION_HEDER__

#include "defines.hpp"
#include <exception>
#include <string>

__CLIPP_begin

class CLIExcpetion : public std::exception
{
public:
	CLIExcpetion(const std::string& msg) noexcept
		: msg(msg) {}
	CLIExcpetion(const char* msg) noexcept
		: msg(msg) {}
	virtual ~CLIExcpetion() noexcept = default;

	virtual const char* what() noexcept { return msg.data(); }

private:
	std::string msg;
};


__CLIPP_end
#endif //! __CLIPP_EXCEPTION_HEDER__