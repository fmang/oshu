/**
 * \file src/oshu-library/main.cc
 */

#include <cstring>
#include <iostream>

#include "./command.h"

command commands[] = {
	build_index,
	{},
};

static void print_usage(std::ostream &os)
{
	os << "Usage:" << std::endl;
	for (command *cmd = commands; cmd->name; ++cmd)
		os << "    oshu-library " << cmd->name << std::endl;
}

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
