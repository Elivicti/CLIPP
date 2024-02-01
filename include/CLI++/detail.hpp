#ifndef __CLIPP_STRINGHELPER_HEADER__
#define __CLIPP_STRINGHELPER_HEADER__

#include "defines.hpp"

#include <fmt/color.h>
#include <cstdint>
#include <vector>
#include <algorithm>

__CLIPP_begin namespace detail {

template <typename T>
struct styled_arg {
	styled_arg(const fmt::detail::styled_arg<T>& styled)
		: value(styled.value), style(styled.style) {}

	styled_arg(const T& value, fmt::text_style style)
		: value(value), style(style) {}

	const T& value;
	fmt::text_style style;
};

enum class String2ArgvErr {
	OK = 0,
	UNBALANCED_QUOTE
};

template<CharType... Chars>
struct StringConstant
{
	constexpr static CharType value[sizeof...(Chars) + 1] = { Chars..., '\0' };
	constexpr static String toString()
	{
		return String(value, sizeof...(Chars));
	}
};
template<CharType ... Chars>
inline constexpr auto StringConstant_v = StringConstant<Chars...>::value;

/**
 * @brief Split string into bash-like tokens.
 * @note  Special operators like `|`, `||` and `&&` won't be recognize if no white spaces are around them,
 *        e.g. "echo 1||echo 2" will be split into {"echo", "1||echo", "2"}
**/
std::vector<String> split_token(const String& cmd, String2ArgvErr* err = nullptr);

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
#ifdef _MSC_VER	// "nice" work, microsoft
	if (ret != nullptr)
		memcpy_s(ret, size, str, size);
#else
	if (ret != nullptr)
		std::memcpy(ret, str, size);
#endif
	return ret;
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

__CLIPP_end

template <typename T, typename Char>
struct fmt::formatter<__CLIPP::detail::styled_arg<T>, Char> : fmt::formatter<T, Char>
{
	template <typename FormatContext>
	auto format(const __CLIPP::detail::styled_arg<T>& arg, FormatContext& ctx) const
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

#endif //! __CLIPP_STRINGHELPER_HEADER__