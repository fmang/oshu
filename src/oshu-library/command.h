/**
 * \file src/oshu-library/command.h
 */

#pragma once

struct command {
	const char *name;
	int (*run)(int argc, char **argv);
};

extern command build_index;

extern command commands[];
