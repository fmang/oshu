/**
 * \file include/core/log.h
 * \ingroup core_log
 */

#pragma once

#include <iosfwd>
#include <SDL2/SDL_log.h>

/**
 * \defgroup core_log Log
 * \ingroup core
 *
 * \brief
 * Facility for logging messages.
 *
 * ### Legacy interface
 *
 * This module provides a set of macros to avoid mentionning the verbosely
 * named `SDL_LOG_CATEGORY_APPLICATION` parameter when using SDL's logging
 * facility.
 *
 * It works with C and looks like printf calls, but requires SDL to be linked
 * with the executable.
 *
 * ### New interface
 *
 * The new C++ interface is based on the standard iostream library.
 *
 * It is a bit more verbose but it is also easier to extend, and also
 * type-safe.
 *
 * See #oshu::core::log.
 *
 * \{
 */

/**
 * Unused so far.
 */
#define oshu_log_verbose(...)  SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

/**
 * Debugging messages are for developers and advanced users. They shouldn't be
 * shown to everyone, except to inspect the cause of a failure. Use this
 * freely.
 */
#define oshu_log_debug(...)    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[" __FILE__ "] " __VA_ARGS__)

/**
 * Informational messages are things that we'd like the techniest users to see,
 * like the fact the loaded audio file is made of signed 16-bit little-endian
 * PCM samples. It won't be shown to the regular user.
 *
 * To show information to the regular user, use a regular printing routing to
 * standard output.
 */
#define oshu_log_info(...)     SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

/**
 * Warning messages are for non-fatal errors. They'll be the primary indicator
 * of unimplemented non-essential features.
 */
#define oshu_log_warning(...)     SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

/**
 * Error messages explain errors fatal to the task we were doing. These won't
 * make the game crash, even though in most cases we'll choose to end the game
 * anyway. Failure to read a beatmap, or to open an audio file are errors.
 */
#define oshu_log_error(...)    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

/**
 * Critical messages are for desperate cases, when the game has no other option
 * but to crash. For example, when SDL fails to initialize.
 */
#define oshu_log_critical(...) SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

/** \} */

namespace oshu {
inline namespace core {

/** \ingroup core_log */
namespace log {

/**
 * \ingroup core_log
 * \{
 */

/**
 * Logging level, a.k.a. priority.
 *
 * The bigger the more important.
 *
 * It is currently fully compatible with SDL's log priorities, but only reuses
 * the constants. No SDL functions will be called, so there's no need to link
 * to SDL for that. In the mid-term, the enum will be unrelated to SDL's, even
 * though they're likely to remain equal.
 */
enum class level : int {
	verbose = SDL_LOG_PRIORITY_VERBOSE,
	debug = SDL_LOG_PRIORITY_DEBUG,
	info = SDL_LOG_PRIORITY_INFO,
	warning = SDL_LOG_PRIORITY_WARN,
	error = SDL_LOG_PRIORITY_ERROR,
	critical = SDL_LOG_PRIORITY_CRITICAL,
};

/**
 * Current log level.
 *
 * Only messages with a level higher or equal to this level are logged. The
 * other messages are silently discarded.
 */
extern level priority;

/**
 * Return the output stream for the wanted verbosity.
 *
 * If the level is greater than or equal to #priority, return a handle to
 * std::clog, otherwise return a dummy stream.
 *
 * It is your responsibility to write std::endl at the end of your log message.
 *
 * You should use #critical, #error, #warning, #info, #debug or #verbose to
 * access your logger than this function directly, as they prefix your message
 * with the log level.
 */
std::ostream& logger(level priority);

std::ostream& verbose();
std::ostream& debug();
std::ostream& info();
std::ostream& warning();
std::ostream& error();
std::ostream& critical();

/** \} */

}}}
