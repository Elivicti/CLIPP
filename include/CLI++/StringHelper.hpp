#ifndef __CLIPP_STRINGHELPER_HEADER__
#define __CLIPP_STRINGHELPER_HEADER__

#include "defines.hpp"

#include <fmt/color.h>
#include <cstdint>
#include <vector>

__CLIPP_begin

template <typename T>
struct styled_arg {
	styled_arg(const fmt::detail::styled_arg<T>& styled)
		: value(styled.value), style(styled.style) {}

	styled_arg(const T& value, fmt::text_style style)
		: value(value), style(style) {}

	const T& value;
	fmt::text_style style;
};

template <typename T>
FMT_CONSTEXPR auto styled(const T& value, fmt::text_style ts)
    -> __CLIPP::styled_arg<fmt::remove_cvref_t<T>>
{
	return __CLIPP::styled_arg<fmt::remove_cvref_t<T>>{value, ts};
}

#define PROMPT_IGNORE_START "\001"
#define PROMPT_IGNORE_END   "\002"

std::vector<std::string> str_to_argv(const std::string& cmd);

__CLIPP_end

template <typename T, typename Char>
struct fmt::formatter<__CLIPP::styled_arg<T>, Char> : fmt::formatter<T, Char>
{
	template <typename FormatContext>
	auto format(const __CLIPP::styled_arg<T>& arg, FormatContext& ctx) const
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