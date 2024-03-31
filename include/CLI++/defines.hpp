#ifndef __CLIPP_DEFINES_HEADER__
#define __CLIPP_DEFINES_HEADER__

#define NAMESPACE_CLIPP__ CLIPP
#define NS_CLIPP NAMESPACE_CLIPP__

#define NAMESPACE_BEGIN(name) namespace name {
#define NAMESPACE_END(name)   }

#define CLIPP_BEGIN NAMESPACE_BEGIN(NS_CLIPP)
#define CLIPP_END   NAMESPACE_END(NS_CLIPP)

#define SET_CLIPP_ALIAS(alias) namespace alias = __namespace_CLIPP

#define PROMPT_IGNORE_START "\001"
#define PROMPT_IGNORE_END   "\002"

#include <string>

CLIPP_BEGIN

using String     = std::string;
using CharType   = String::value_type;
using StringView = std::basic_string_view<CharType>;

CLIPP_END

#endif //! __CLIPP_DEFINES_HEADER__