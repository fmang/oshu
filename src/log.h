#pragma once

#include <SDL2/SDL.h>

/** @defgroup log Log
 *
 * This module provides a set of macros to avoid mentionning the verbosely
 * named `SDL_LOG_CATEGORY_APPLICATION` parameter when using SDL's logging
 * facility.
 *
 * @{
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
#define oshu_log_warn(...)     SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

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

/** @} */
