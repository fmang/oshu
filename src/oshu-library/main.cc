/**
 * \file src/oshu-library/main.cc
 */

#include "config.h"

#include <cstring>
#include <iostream>

#include "./command.h"

static void print_usage(std::ostream &os)
{
	os << "Usage:" << std::endl;
	for (command *cmd = commands; cmd->name; ++cmd)
		os << "    oshu-library " << cmd->name << std::endl;
}

static int print_help(int argc, char **argv)
{
	std::cout << "oshu-library " << PROJECT_VERSION << std::endl;
	std::cout << "Manage your beatmaps." << std::endl << std::endl;
	print_usage(std::cout);
	std::cout << std::endl;
	std::cout << "Please refer to the oshu-library(1) man page for details." << std::endl;
	return 0;
}

command help {
	.name = "help",
	.run = print_help,
};

command commands[] = {
	build_index,
	help,
	{},
};

int main(int argc, char **argv)
{
	if (argc < 2) {
		print_usage(std::cerr);
		return 1;
	}
	const char *cmdarg = argv[1];
	for (command *cmd = commands; cmd->name; ++cmd) {
		if (!strcmp(cmd->name, cmdarg))
			return cmd->run(argc - 1, argv + 1);
	}
	std::cerr << "unknown command: " << argv[1] << std::endl;
	print_usage(std::cerr);
	return 1;
}
