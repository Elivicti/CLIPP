#ifndef __CLIPP_DETAIL_HEADER__
#define __CLIPP_DETAIL_HEADER__

#include "defines.hpp"

#include <fmt/color.h>
#include <cstdint>
#include <vector>
#include <algorithm>

#ifdef __GNUC__
#  include <cxxabi.h>
#endif

__CLIPP_begin namespace detail {

template <typename T>
struct StyledArg {
	StyledArg(const StyledArg<T>& styled)
		: value(styled.value), style(styled.style) {}
	StyledArg(const fmt::detail::styled_arg<T>& styled)
		: value(styled.value), style(styled.style) {}

	StyledArg(const T& value, fmt::text_style style)
		: value(value), style(style) {}

	const T& value;
	fmt::text_style style;
};

struct CliSyntaxError
{
	enum Type
	{
		OK = 0,
		UNBALANCED_QUOTE
	};

	CliSyntaxError() : type{OK}, quote{0} {}
	CliSyntaxError(Type t, CharType quote = 0) : type{t}, quote{quote} {}

	Type type;
	CharType quote;

	bool operator==(Type t) { return type == t; }
};

template<CharType... Chars>
struct StringConstantType
{
	constexpr static CharType value[sizeof...(Chars) + 1] = { Chars..., '\0' };
	constexpr static String toString()
	{
		return String(value, sizeof...(Chars));
	}
};
template<CharType ... Chars>
inline constexpr auto StringConstant = StringConstantType<Chars...>::value;

template<typename CharT>
inline const CharT* convert_str(const char* str, std::size_t len);

template<>
inline const char* convert_str<char>(const char* str, std::size_t len) { return str; }

template<>
inline const wchar_t* convert_str<wchar_t>(const char* str, std::size_t len)
{
	wchar_t* wcs = new wchar_t[len + 1];
#ifdef _MSC_VER // "nice" work, microsoft
	mbstowcs_s(nullptr, wcs, len + 1, str, len + 1);
#else
	std::mbstowcs(wcs, str, len + 1);
#endif // _MSC_VER
	return wcs;
}

/**
 * @brief Split string into bash-like tokens.
 * @note  Special operators like `|`, `||` and `&&` won't be recognize if no white spaces are around them,
 *        e.g. "echo 1||echo 2" will be split into {"echo", "1||echo", "2"}
**/
std::vector<String> split_token(StringView cmd, CliSyntaxError* err = nullptr);

/** @brief Check if a string is empty, or its characters are all white spaces (i.e. character that `std::isspace` returns true). */
template<typename CharT>
bool is_empty_string(const std::basic_string<CharT>& str)
{
	return str.empty() || std::ranges::all_of(str, isspace);
}

/**
 * @brief Calculate length of given C-Style string
 * @tparam CharT type of a character
 * @param str C-Style string pointer
 * @return length of the given string.
**/
template<typename CharT>
std::size_t strlen(const CharT* str)
{
	std::size_t len = 0;
	while (*(str++) != 0)
		len++;
	return len;
}

/**
 * @brief Duplicate a C-Style string from another.
 * @note  This function will allocate memory on heap through `malloc`, remember to free it.
 * @tparam CharT type of a character
 * @param str C-Style string pointer
 * @return Duplicated string, if no memory available (allocation failed), `nullptr` is returned.
**/
template<typename CharT>
CharT* strdup(const CharT* str)
{
	std::size_t size = sizeof(CharT) * (strlen(str) + 1);
	CharT* ret = (CharT*)std::malloc(size);
#ifdef _MSC_VER // "nice" work, microsoft
	if (ret != nullptr)
		memcpy_s(ret, size, str, size);
#else
	if (ret != nullptr)
		std::memcpy(ret, str, size);
#endif
	return ret;
}

/**
 * @brief  Get name from a type.
 * @return Name of the type, is demangled if compile under gcc or clang.
**/
template<typename T>
String type_name()
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

}	// namespace detail

template<typename Func, typename ...Args>
struct ScopeGuard
{
	Func _f;
	std::tuple<Args...> _args;
	ScopeGuard(Func&& fn, Args&& ...args)
		: _f(fn), _args(std::forward<Args>(args)...) {}
	~ScopeGuard() noexcept { std::apply(_f, _args); }
};

namespace literals {

inline String operator""_S(const char* text, std::size_t len)
{
	auto p = detail::convert_str<CharType>(text, len);
	String ret{ p, len };
	if constexpr (std::is_same_v<CharType, wchar_t>)
		delete p;
	return ret;
}

}	// namespace literals

__CLIPP_end

template <typename T, typename Char>
struct fmt::formatter<__CLIPP::detail::StyledArg<T>, Char> : fmt::formatter<T, Char>
{
	template <typename FormatContext>
	auto format(const __CLIPP::detail::StyledArg<T>& arg, FormatContext& ctx) const
		-> decltype(ctx.out())
	{
		const auto& ts = arg.style;
		const auto& value = arg.value;
		auto out = ctx.out();

		uint8_t has_style = (uint8_t)ts.has_emphasis()
			| (uint8_t)ts.has_foreground() << 1
			| (uint8_t)ts.has_background() << 2;

		auto rl_prompt_ignore_start = fmt::string_view(PROMPT_IGNORE_START);
		auto rl_prompt_ignore_end   = fmt::string_view(PROMPT_IGNORE_END);

		out = std::copy(rl_prompt_ignore_start.begin(), rl_prompt_ignore_start.end(), out);
		if (has_style & 0b001)	// ts.has_emphasis()
		{
			auto emphasis = fmt::detail::make_emphasis<Char>(ts.get_emphasis());
			out = std::copy(emphasis.begin(), emphasis.end(), out);
		}
		if (has_style & 0b010)	// ts.has_foreground()
		{
			auto foreground = fmt::detail::make_foreground_color<Char>(ts.get_foreground());
			out = std::copy(foreground.begin(), foreground.end(), out);
		}
		if (has_style & 0b100)	// ts.has_background()
		{
			auto background = fmt::detail::make_background_color<Char>(ts.get_background());
			out = std::copy(background.begin(), background.end(), out);
		}
		out = std::copy(rl_prompt_ignore_end.begin(), rl_prompt_ignore_end.end(), out);
		out = fmt::formatter<T, Char>::format(value, ctx);
		if (has_style)
		{
			auto reset_color = fmt::string_view(PROMPT_IGNORE_START "\x1b[0m" PROMPT_IGNORE_END);
			out = std::copy(reset_color.begin(), reset_color.end(), out);
		}
		return out;
	}
};

#endif //! __CLIPP_DETAIL_HEADER__