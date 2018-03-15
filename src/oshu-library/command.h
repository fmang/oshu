/**
 * \file src/oshu-library/command.h
 */

#pragma once

#include <exception>

class usage_error : public std::exception {
	const char* what() const throw() { return "bad usage"; }
};

struct command {
	const char *name;
	int (*run)(int argc, char **argv);
};

extern command build_index;

extern command commands[];
