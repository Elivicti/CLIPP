#ifndef __CLIPP_DEFINES_HEADER__
#define __CLIPP_DEFINES_HEADER__

#define __namespace_CLIPP CLIPP
#define __CLIPP __namespace_CLIPP

#define __CLIPP_begin namespace __namespace_CLIPP {
#define __CLIPP_end }

#define SET_CLIPP_ALIAS(alias) namespace alias = __namespace_CLIPP

#define PROMPT_IGNORE_START "\001"
#define PROMPT_IGNORE_END   "\002"

#include <string>

__CLIPP_begin

// #ifndef WIN32
// using String = std::string;
// #else
// using String = std::wstring;
// #endif

using String = std::string;
using CharType = String::value_type;

__CLIPP_end

#endif //! __CLIPP_DEFINES_HEADER__