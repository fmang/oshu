/**
 * \file lib/core/log.cc
 * \ingroup core_log
 */

#include "core/log.h"

#include <iostream>

namespace oshu {

log_level log_priority = log_level::warning;

log_level& operator++(log_level &l)
{
	if (l == log_level::critical)
		return l;
	int raw = static_cast<int>(l);
	l = static_cast<log_level>(raw + 1);
	return l;
}

log_level& operator--(log_level &l)
{
	if (l == log_level::verbose)
		return l;
	int raw = static_cast<int>(l);
	l = static_cast<log_level>(raw - 1);
	return l;
}

/**
 * Dummy output stream.
 */
static std::ostream devnull {nullptr};

std::ostream& logger(log_level priority)
{
	if (log_priority >= oshu::log_priority)
		return std::clog;
	else
		return devnull;
}

std::ostream& verbose_log()  { return logger(log_level::verbose)  << "VERBOSE: "; }
std::ostream& debug_log()    { return logger(log_level::debug)    << "DEBUG: "; }
std::ostream& info_log()     { return logger(log_level::info)     << "INFO: "; }
std::ostream& warning_log()  { return logger(log_level::warning)  << "WARNING: "; }
std::ostream& error_log()    { return logger(log_level::error)    << "ERROR: "; }
std::ostream& critical_log() { return logger(log_level::critical) << "CRITICAL: "; }

}
