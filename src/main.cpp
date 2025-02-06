#include <std_include.hpp>

#include "console.hpp"

#include "updater/updater.hpp"

namespace
{
	int unsafe_main(std::string&& prog, std::vector<std::string>&& args)
	{
		// Parse command-line flags (only increment i for matching flags)
		for (auto i = args.begin(); i != args.end();)
		{
			if (*i == "-update-iw4x")
			{
				return updater::update_iw4x();
			}
			else
			{
				console::info("AlterWare Installer\n"
				              "Usage: %s OPTIONS\n"
				              "  -update-iw4x",
				              prog.c_str()
				);

				return EXIT_FAILURE;
			}
		}

		return EXIT_SUCCESS;
	}
}

int main(const int argc, char* argv[])
{
	console::set_title("AlterWare Installer");
	console::log("AlterWare Installer");

	try
	{
		std::string prog(argv[0]);
		std::vector<std::string> args;

		args.reserve(argc - 1);
		args.assign(argv + 1, argv + argc);
		return unsafe_main(std::move(prog), std::move(args));
	}
	catch (const std::exception& ex)
	{
		console::error("Fatal error: %s", ex.what());
		return EXIT_FAILURE;
	}
}
