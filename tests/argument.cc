/* this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details */

#include <acheron/arguments.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <ranges>

int main()
{
	ach::arguments args;

	std::cout << "Number of arguments: " << args.size() << '\n';
	std::cout << "Arguments:\n";

	for (std::size_t i = 0; i < args.size(); ++i)
	{
		std::cout << "  [" << i << "] native: " << args[i].native() << '\n';
		std::cout << "      string: " << args[i].string() << '\n';
		std::cout << "      u8string size: " << args[i].u8string().size() << '\n';
		std::cout << "      u16string size: " << args[i].u16string().size() << '\n';
		std::cout << "      u32string size: " << args[i].u32string().size() << '\n';
	}

	assert(!args.empty());
	assert(args.size() > 0);

	std::cout << "\nIterator test:\n";
	for (const auto &arg: args)
		std::cout << "  " << arg << '\n';

	std::cout << "\nReverse iterator test:\n";
	for (auto arg : std::ranges::reverse_view(args))
		std::cout << "  " << arg << '\n';

	std::cout << "\nComparison test:\n";
	if (args.size() > 1)
	{
		std::cout << "  args[0] == args[1]: " << (args[0] == args[1]) << '\n';
		std::cout << "  args[0] < args[1]: " << (args[0] < args[1]) << '\n';
	}

	std::cout << "\nConversion test with Unicode:\n";
	if (args.size() > 1)
	{
		auto str = args[1].string();
		auto wstr = args[1].wstring();
		auto u8str = args[1].u8string();
		auto u16str = args[1].u16string();
		auto u32str = args[1].u32string();

		std::cout << "  UTF-8 length: " << str.size() << '\n';
		std::cout << "  wstring length: " << wstr.size() << '\n';
		std::cout << "  UTF-8 length: " << u8str.size() << '\n';
		std::cout << "  UTF-16 length: " << u16str.size() << '\n';
		std::cout << "  UTF-32 length: " << u32str.size() << '\n';
	}

	std::cout << "\nFormat test:\n";
	if (!args.empty())
		std::cout << std::format("  First arg: {}\n", args[0]);

	std::cout << "\nat() bounds checking test:\n";
	try
	{
		[[maybe_unused]] auto &arg = args.at(args.size() + 100);
		std::cout << "  ERROR: at() should have thrown\n";
	}
	catch (const std::out_of_range &)
	{
		std::cout << "  at() correctly threw out_of_range\n";
	}

	std::cout << "\nAlgorithm test:\n";
	auto it = std::ranges::find_if(args, [](const ach::argument &arg)
								   { return arg.string().find("test") != std::string::npos; });
	if (it != args.end())
		std::cout << "  Found argument containing 'test': " << *it << '\n';
	else
		std::cout << "  No argument containing 'test' found\n";

	std::cout << "\nAll tests passed!\n";
	return 0;
}
