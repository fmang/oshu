/**
 * \file game/game.cc
 * \ingroup game
 *
 * Implemention game initialization, destruction and the main loop.
 */

#include "config.h"

#include "core/log.h"
#include "game/game.h"
#include "game/tty.h"

#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL_image.h>

static int open_beatmap(const char *beatmap_path, struct oshu_game *game)
{
	if (oshu_load_beatmap(beatmap_path, &game->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		return -1;
	}
	if (game->beatmap.mode != OSHU_OSU_MODE) {
		oshu_log_error("unsupported game mode");
		return -1;
	}
	assert (game->beatmap.hits != NULL);
	game->hit_cursor = game->beatmap.hits;
	return 0;
}

static int open_audio(struct oshu_game *game)
{
	assert (game->beatmap.audio_filename != NULL);
	if (oshu_open_audio(game->beatmap.audio_filename, &game->audio) < 0) {
		oshu_log_error("no audio, aborting");
		return -1;
	}
	oshu_open_sound_library(&game->library, &game->audio.device_spec);
	oshu_populate_library(&game->library, &game->beatmap);
	return 0;
}

int oshu_create_game(const char *beatmap_path, struct oshu_game *game)
{
	if (open_beatmap(beatmap_path, game) < 0)
		goto fail;
	if (open_audio(game) < 0)
		goto fail;
	if (game->initialize() < 0)
		goto fail;
	return 0;
fail:
	oshu_destroy_game(game);
	return -1;
}

void oshu_destroy_game(struct oshu_game *game)
{
	assert (game != NULL);
	game->destroy();
	oshu_destroy_beatmap(&game->beatmap);
	oshu_close_audio(&game->audio);
	oshu_close_sound_library(&game->library);
}
