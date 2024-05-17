#include "../include/CLI++/ArgumentParser.hpp"

SET_CLIPP_ALIAS(cli);

int main(int argc, const char** argv)
{
    // CLI::ArgParser parser;
	cli::Option<int> opt{"answer", 'a', 42, "what is the answer?"};
	cli::OptionBase* p = &opt;

	// fmt::vformat()

	try
	{
		fmt::print("{}\n", opt.get());
		fmt::print("{}\n", p->get<int>());
		fmt::print("{}\n", p->name());
		fmt::print("{}\n", p->description());
		fmt::print("{}\n", opt.shortName());
		fmt::print("{0}:{1}:{0}\n", 'a', 'b');
	}
	catch(const std::exception& e)
	{
		fmt::print("{}\n", e.what());
	}
	
}