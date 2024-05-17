#ifndef __CLIPP_DETAIL_TYPES_HEADER__
#define __CLIPP_DETAIL_TYPES_HEADER__

#include "../defines.hpp"

#ifdef __GNUC__
#  include <cxxabi.h>
#endif

CLIPP_BEGIN NAMESPACE_BEGIN(detail)

/**
 * @brief  Get name from a type.
 * @return Name of the type, is demangled if compile under gcc or clang.
**/
template<typename T>
String TypeName()
{
#ifdef __GNUC__	// gcc and clang requires demangle
	char* p = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
	String ret(p);
	free(p);
	return ret;
#else
	return typeid(T).name();
#endif
}

template<typename T>
inline const String NameOfType = TypeName<T>();

/**
 * @brief Specify a name for a type. This macro is only meant to simplify some work
 *        for simple types. See `detail::TypeName` for more info.
 * @note  This macro will not work for types that have multiple template arguments.
 *        e.g. std::map<int, int>
**/
#define REGISTER_TYPENAME(type, name) template<> String NS_CLIPP::detail::TypeName<type>() { return name; }

// `T` has overloaded `operator>>(std::istream, T)`
template<typename T>
concept LexicalCastTarget = requires (std::basic_istream<CharType> ss, T target)
{ ss >> target; };

// `T` has overloaded `operator<<(std::ostream, T)`
template<typename T>
concept LexicalCastSource = requires (std::basic_ostream<CharType> ss, T src)
{ ss << src; };

/**
 * @brief If casting from `Source` type to `Target` type is custom defined.
 *        If `value` is `true`, concept constrain `LexicalCastTarget` and
 *        `LexicalCastSource` will be skipped.
**/
template<typename Target, typename Source>
struct UsesCustomCast { static constexpr bool value = false; };

template<typename Target, typename Source>
requires (LexicalCastTarget<Target> && LexicalCastSource<Source>)
	  ||  std::is_same_v<Target, Source> // same type falls to specialization
	  ||  UsesCustomCast<Target, Source>::value
struct LexicalCast
{
	static Target cast(const Source& src)
	{
		std::basic_stringstream<CharType> ss;
		Target ret = Target();

		if (!((ss << src) && (ss >> ret)))
			throw BadLexicalCast<Target, Source>();
		return ret;
	}
};

template<LexicalCastTarget Target>
struct LexicalCast<Target, String>
{
	static Target cast(const String& src)
	{
		Target ret = Target();
		std::basic_istringstream<CharType> ss(src);
		if (!(ss >> ret))
			throw BadLexicalCast<Target, String>();
		return ret;
	}
};

template<LexicalCastSource Source>
struct LexicalCast<String, Source>
{
	static String cast(const Source& src)
	{
		std::basic_ostringstream<CharType> ss;
		ss << src;
		return ss.str();
	}
};

template<typename T>
struct LexicalCast<T, T> { static const T& cast(const T& src) { return src; } };


NAMESPACE_END(detail) CLIPP_END


#endif //! __CLIPP_DETAIL_TYPES_HEADER__