/**
 * \file src/oshu/main.cc
 *
 * \brief
 * Main entry-point for the game.
 *
 * Interpret command-line arguments and spawn everything.
 */

#include "config.h"

#include "log.h"
#include "game/game.h"
#include "osu/osu.h"

extern "C" {
#include <libavutil/log.h>
}

#include <SDL2/SDL.h>

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

enum option_values {
	OPT_AUTOPLAY = 0x10000,
	OPT_HELP = 'h',
	OPT_PAUSE = 0x10001,
	OPT_VERBOSE = 'v',
	OPT_VERSION = 0x10002,
};

static struct option options[] = {
	{"autoplay", no_argument, 0, OPT_AUTOPLAY},
	{"help", no_argument, 0, OPT_HELP},
	{"pause", no_argument, 0, OPT_PAUSE},
	{"verbose", no_argument, 0, OPT_VERBOSE},
	{"version", no_argument, 0, OPT_VERSION},
	{0, 0, 0, 0},
};

static const char *flags = "vh";

static const char *usage =
	"Usage: oshu [-v] BEATMAP.osu\n"
	"       oshu --help\n"
	"       oshu --version\n"
;

static const char *help =
	"Options:\n"
	"  -v, --verbose       Increase the verbosity.\n"
	"  -h, --help          Show this help message.\n"
	"  --autoplay          Perform a perfect run.\n"
	"  --pause             Start the game paused.\n"
	"  --version           Print program version.\n"
	"\n"
	"Check the man page oshu(1) for details.\n"
;

static struct osu_game current_game;

static void signal_handler(int signum)
{
	oshu_stop_game(&current_game);
}

static void print_version()
{
	puts("oshu! version " PROJECT_VERSION);
}

int run(const char *beatmap_path, int autoplay, int pause)
{
	int rc = 0;

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
		oshu_log_error("SDL initialization error: %s", SDL_GetError());
		return -1;
	}

	if ((rc = oshu_create_game(beatmap_path, &current_game)) < 0) {
		oshu_log_error("game initialization failed");
		return -1;
	}

	current_game.autoplay = autoplay;
	if (pause)
		oshu_pause_game(&current_game);

	if ((rc = oshu_run_game(&current_game)) < 0)
		oshu_log_error("error while running the game, aborting");

	oshu_destroy_game(&current_game);
	SDL_Quit();
	return rc;
}

int main(int argc, char **argv)
{
	int autoplay = 0;
	int pause = 0;
	int verbosity = SDL_LOG_PRIORITY_INFO;

	for (;;) {
		int c = getopt_long(argc, argv, flags, options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case OPT_VERBOSE:
			if (verbosity > SDL_LOG_PRIORITY_VERBOSE)
				verbosity--;
			break;
		case OPT_HELP:
			print_version();
			puts(usage);
			fputs(help, stdout);
			return 0;
		case OPT_AUTOPLAY:
			autoplay = 1;
			break;
		case OPT_PAUSE:
			pause = 1;
			break;
		case OPT_VERSION:
			print_version();
			return 0;
		default:
			fputs(usage, stderr);
			return 2;
		}
	}

	if (argc - optind != 1) {
		fputs(usage, stderr);
		return 2;
	}

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, (SDL_LogPriority) verbosity);
	av_log_set_level(verbosity <= SDL_LOG_PRIORITY_DEBUG ? AV_LOG_INFO : AV_LOG_ERROR);

	char *beatmap_path = realpath(argv[optind], NULL);
	if (beatmap_path == NULL) {
		oshu_log_error("cannot locate %s", argv[optind]);
		return 3;
	}

	char *beatmap_file = beatmap_path;
	char *slash = strrchr(beatmap_path, '/');
	if (slash) {
		*slash = '\0';
		oshu_log_debug("changing the current directory to %s", beatmap_path);
		if (chdir(beatmap_path) < 0) {
			oshu_log_error("error while changing directory: %s", strerror(errno));
			return 3;
		}
		beatmap_file = slash + 1;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	if (run(beatmap_file, autoplay, pause) < 0)
		return 1;

	free(beatmap_path);

	return 0;
}
