/**
 * \file oshu.c
 */

#include "log.h"
#include "game.h"

#include <signal.h>

static struct oshu_game *current_game;

static void signal_handler(int signum)
{
	if (current_game)
		current_game->stop = 1;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		puts("Usage: oshu beatmap.osu");
		return 5;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	int rc = 0;
	char *beatmap_path = argv[1];

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
		oshu_log_error("SDL initialization error: %s", SDL_GetError());
		return 1;
	}

	oshu_audio_init();

	if ((rc = oshu_game_create(beatmap_path, &current_game)) < 0) {
		oshu_log_error("game initialization failed");
		goto done;
	}

	if ((rc = oshu_game_run(current_game)) < 0) {
		oshu_log_error("error while running the game, aborting");
		goto done;
	}

done:
	if (current_game)
		oshu_game_destroy(&current_game);
	SDL_Quit();
	return (rc == 0) ? 0 : 1;
}
