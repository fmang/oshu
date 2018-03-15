/**
 * \file src/oshu-library/command.h
 *
 * This module provides a common structure that all the sub-commands must
 * implement for the main entry point to process them generically.
 */

#pragma once

struct command {
	/**
	 * Name of the sub-command, tested against when the command is invoked.
	 *
	 * Call it `foo` and the command will be run when the user enters
	 * `oshu-library foo`.
	 */
	const char *name;
	/**
	 * The command's main function.
	 *
	 * The argc and argv are the same as main, with the first argument
	 * popped, so you can give it to getopt as if it were a standard C main
	 * function.
	 */
	int (*run)(int argc, char **argv);
};

extern command build_index;
extern command help;

/**
 * List of all the registered commands.
 */
extern command commands[];
