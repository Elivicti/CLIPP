#ifndef __CLIPP_ARGUMANETPARSER_HEADER__
#define __CLIPP_ARGUMANETPARSER_HEADER__

#include <sstream>
#include <vector>
#include <set>

#include "defines.hpp"
#include "Exceptions.hpp"
#include "detail/Types.hpp"

CLIPP_BEGIN

template<typename T>
concept HasNoCVRef = std::is_same_v<T, std::remove_cvref_t<T>>;

template<HasNoCVRef T>
class Option;

class OptionBase
{
public:
	enum class Type
	{
		None,
		Option, Flag, Positional
	};
public:
	OptionBase(Type opt_type, StringView desc = "")
		: is_required{false}, desc{desc}
	{}

	virtual ~OptionBase() = default;

	virtual constexpr const std::type_info& storedTypeInfo() const = 0;
	virtual constexpr const String& storedTypeName() const = 0;

	virtual const String& name() const = 0;
	virtual bool checkName(StringView name) const = 0;
	
	bool required() { return is_required; }
	OptionBase* setRequired(bool is_required) { this->is_required = is_required; return this; }

	const String& description() const { return desc; }
	OptionBase* setDescription(StringView desc) { this->desc = desc; return this; }

	Type type() const { return opt_type; }


	template<HasNoCVRef T>
	T& get()
	{
		if (auto opt = dynamic_cast<Option<T>*>(this))
			return opt->get();
		else
			throw CLIException(fmt::format("Invalid cast to {}.", detail::NameOfType<T>));
	}

	template<HasNoCVRef T>
	const T& get() const
	{
		if (auto opt = dynamic_cast<const Option<T>*>(this))
			return opt->get();
		else
			throw CLIException(fmt::format("Invalid cast to {}.", detail::NameOfType<T>));
	}

protected:
	static StringView validateLongName(StringView long_name)
	{
		auto space_pos = long_name.find_first_not_of(' ');
		if (space_pos >= long_name.size())
			throw CLIException{"empyt name"};
		long_name.remove_prefix(space_pos);

		space_pos = long_name.find_first_of(' ');
		auto size = long_name.size();
		long_name.remove_suffix((space_pos >= size ? 0 : (size - space_pos)));
		return long_name;
	}

private:
	bool is_required;
	String desc;

	Type opt_type;
};

using OptionType = OptionBase::Type;

template<HasNoCVRef T>
class Option : public OptionBase
{
public:
	using value_type = T;

public:
	Option(StringView name, char short_name = '\0', const T& default_value = T{}, StringView desc = "")
		: OptionBase{OptionType::Option, desc}, value{default_value}
		, short_name{short_name}
	{
		auto processed_name = this->validateLongName(name);
		long_names.emplace_back(processed_name);
	}

	virtual ~Option() = default;

	const value_type& get() const { return value; }
	value_type& get() { return value; }

	Option* setRequired(bool is_required)
	{
		this->OptionBase::setRequired(is_required);
		return this;
	}
	Option* setDescription(StringView desc)
	{
		this->OptionBase::setDescription(desc);
		return this;
	}

	virtual constexpr const std::type_info& storedTypeInfo() const override { return typeid(T); }
	virtual constexpr const String& storedTypeName() const override { return detail::NameOfType<T>; }

	virtual const String& name() const override { return long_names.front(); }
	virtual bool checkName(StringView name) const override
	{
		for (auto& lname : long_names)
			if (lname == name)
				return true;
		return false;
	}

	char shortName() const { return short_name; }
	const std::vector<String>& longNames() const { return long_names; }

	Option* addLongName(StringView long_name)
	{
		long_names.emplace_back(this->validateLongName(long_name));
		return this;
	}

	bool checkShortName(char short_name) const { return (this->short_name != '\0') && (this->short_name == short_name); }

private:
	T value;

	char short_name;
	std::vector<String> long_names;

public:
	bool operator==(StringView long_name) const { return this->checkName(long_name); }
	bool operator==(char short_name) const { return this->checkShortName(short_name); }
};


template<>
class Option<bool> : public OptionBase
{
public:
	using value_type = bool;

public:
	Option(StringView long_name, char short_name = '\0', bool default_value = false, StringView desc = "")
		: OptionBase{OptionType::Flag, desc}, value{default_value}
		, short_name{short_name}
	{ long_names.emplace_back(this->validateLongName(long_name)); }

	virtual ~Option() = default;

	const value_type& get() const { return value; }
	value_type& get() { return value; }

	Option* setRequired(bool is_required)
	{
		this->OptionBase::setRequired(is_required);
		return this;
	}
	Option* setDescription(StringView desc)
	{
		this->OptionBase::setDescription(desc);
		return this;
	}
	Option* setStoreFalse(bool store_false) { this->store_false = store_false; return this; }

	virtual constexpr const std::type_info& storedTypeInfo() const override { return typeid(bool); }
	virtual constexpr const String& storedTypeName() const override { return detail::NameOfType<bool>; }

	virtual const String& name() const override { return long_names.front(); }
	virtual bool checkName(StringView name) const override
	{
		for (auto& lname : long_names)
			if (lname == name)
				return true;
		return false;
	}

	char shortName() const { return short_name; }
	const std::vector<String>& longNames() const { return long_names; }

	Option* addLongName(StringView long_name)
	{
		long_names.emplace_back(this->validateLongName(long_name));
		return this;
	}

	bool checkShortName(char short_name) const { return (this->short_name != '\0') && (this->short_name == short_name); }


private:
	value_type value;
	bool store_false;

	char short_name;
	std::vector<String> long_names;

public:
	bool operator==(StringView long_name) const { return this->checkName(long_name); }
	bool operator==(char short_name) const { return this->checkShortName(short_name); }
};

template<HasNoCVRef T>
class PositionalArgument : public OptionBase
{
public:
	using value_type = T;
public:
	PositionalArgument(StringView name, const T& default_value = T{}, StringView desc = "")
		: OptionBase{OptionType::Positional, desc}, value{default_value}
		, arg_name{OptionBase::validateLongName(name)}
	{}
	
	virtual constexpr const std::type_info& storedTypeInfo() const override { return typeid(T); }
	virtual constexpr const String& storedTypeName() const override { return detail::NameOfType<T>; }

	virtual const String& name() const override { return arg_name; }
	virtual bool checkName(StringView name) const override { return arg_name == name; }

	PositionalArgument* setRequired(bool is_required)
	{
		this->OptionBase::setRequired(is_required);
		return this;
	}
	PositionalArgument* setDescription(StringView desc)
	{
		this->OptionBase::setDescription(desc);
		return this;
	}

	const value_type& get() const { return value; }
	value_type& get() { return value; }

private:
	T value;
	String arg_name;

public:
	bool operator==(StringView name) const { return this->checkName(name); }
};


class ArgParser
{
public:
	void parse(int argc, const char** argv);
	void parse(const std::vector<String>& args);

	template<HasNoCVRef T>
	Option<T>* addOption(StringView name, char short_name, T& def, StringView desc = String())
	{
		Option<T>* opt = new Option<bool>{name, short_name, false, desc};
		options.insert(opt);
		return opt;
	}

	Option<bool>* addFlag(StringView name, char short_name, StringView desc = String())
	{
		Option<bool>* opt = new Option<bool>{name, short_name, false, desc};
		options.insert(opt);
		
		return opt;
	}

private:
	String program;

	// std::map<StringView, OptionBase*> options;
	std::set<OptionBase*> options;
	std::vector<OptionBase*> positionals;
};

CLIPP_END
#endif //! __CLIPP_ARGUMANETPARSER_HEADER__