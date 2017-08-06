/**
 * \file oshu.c
 *
 * \brief
 * Main entry-point for the game.
 *
 * Interpret command-line arguments and spawn everything.
 */

#include "log.h"
#include "game.h"
#include "../config.h"

#include <getopt.h>
#include <signal.h>
#include <unistd.h>

enum option_values {
	OPT_AUTOPLAY = 0x10000,
	OPT_HELP = 'h',
	OPT_VERBOSE = 'v',
};

static struct option options[] = {
	{"autoplay", no_argument, 0, OPT_AUTOPLAY},
	{"help", no_argument, 0, OPT_HELP},
	{"verbose", no_argument, 0, OPT_VERBOSE},
	{0, 0, 0, 0},
};

static const char *flags = "vh";

static const char *usage =
	"Usage: oshu [-v] BEATMAP.osu\n"
	"       oshu --help\n"
;

static const char *help =
	"Options:\n"
	"  -v, --verbose       Increase the verbosity.\n"
	"  -h, --help          Show this help message.\n"
	"  --autoplay          Perform a perfect run.\n"
	"\n"
	"Check the man page oshu(1) for details.\n"
;

static struct oshu_game *current_game;

static void signal_handler(int signum)
{
	if (current_game)
		current_game->stop = 1;
}

int run(const char *beatmap_path, int autoplay)
{
	int rc = 0;

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
		oshu_log_error("SDL initialization error: %s", SDL_GetError());
		return -1;
	}

	oshu_audio_init();

	if ((rc = oshu_game_create(beatmap_path, &current_game)) < 0) {
		oshu_log_error("game initialization failed");
		goto done;
	}

	current_game->autoplay = autoplay;

	if ((rc = oshu_game_run(current_game)) < 0) {
		oshu_log_error("error while running the game, aborting");
		goto done;
	}

done:
	if (current_game)
		oshu_game_destroy(&current_game);
	SDL_Quit();
	return rc;
}

int main(int argc, char **argv)
{
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

	int autoplay = 0;

	for (;;) {
		int c = getopt_long(argc, argv, flags, options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case OPT_VERBOSE:
			SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
			break;
		case OPT_HELP:
			puts("oshu! version " VERSION);
			puts(usage);
			fputs(help, stdout);
			return 0;
		case OPT_AUTOPLAY:
			autoplay = 1;
			break;
		default:
			fputs(usage, stderr);
			return 2;
		}
	}

	if (argc - optind != 1) {
		fputs(usage, stderr);
		return 2;
	}

	char *beatmap_path = argv[optind];
	char *slash = strrchr(beatmap_path, '/');
	if (slash) {
		*slash = '\0';
		oshu_log_debug("changing the current directory to %s", beatmap_path);
		if (chdir(beatmap_path) < 0) {
			oshu_log_error("error while changing directory: %s", strerror(errno));
			return 3;
		}
		beatmap_path = slash + 1;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	if (run(beatmap_path, autoplay) < 0)
		return 1;

	return 0;
}
