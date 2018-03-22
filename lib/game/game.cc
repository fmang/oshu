/**
 * \file game/game.cc
 * \ingroup game
 *
 * Implemention game initialization, destruction and the main loop.
 */

#include "config.h"

#include "game/game.h"
#include "game/tty.h"
#include "video/texture.h"
#include "core/log.h"

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

static int open_display(struct oshu_game *game)
{
	if (oshu_open_display(&game->display) < 0) {
		oshu_log_error("no display, aborting");
		return -1;
	}
	struct oshu_metadata *meta = &game->beatmap.metadata;
	std::ostringstream title;
	title << meta->artist << " - " << meta->title << " ♯ " << meta->version << " 𝄞 oshu!";
	SDL_SetWindowTitle(game->display.window, title.str().c_str());
	oshu_reset_view(&game->display);
	return 0;
}

static int create_ui(struct oshu_game *game)
{
	if (game->beatmap.background_filename)
		oshu_load_background(&game->display, game->beatmap.background_filename, &game->ui.background);
	oshu_create_metadata_frame(&game->display, &game->beatmap, &game->clock.system, &game->ui.metadata);
	if (oshu_create_audio_progress_bar(&game->display, &game->audio.music, &game->ui.audio_progress_bar) < 0)
		return -1;
	return 0;
}

int oshu_create_game(const char *beatmap_path, struct oshu_game *game)
{
	if (open_beatmap(beatmap_path, game) < 0)
		goto fail;
	if (open_audio(game) < 0)
		goto fail;
	if (open_display(game) < 0)
		goto fail;
	if (create_ui(game) < 0)
		goto fail;
	if (game->initialize() < 0)
		goto fail;
	return 0;
fail:
	oshu_destroy_game(game);
	return -1;
}

static void destroy_ui(struct oshu_game *game)
{
	oshu_destroy_background(&game->ui.background);
	oshu_destroy_metadata_frame(&game->ui.metadata);
	oshu_destroy_score_frame(&game->ui.score);
	oshu_destroy_audio_progress_bar(&game->ui.audio_progress_bar);
}

void oshu_destroy_game(struct oshu_game *game)
{
	assert (game != NULL);
	game->destroy();
	oshu_destroy_beatmap(&game->beatmap);
	oshu_close_audio(&game->audio);
	oshu_close_sound_library(&game->library);
	destroy_ui(game);
	oshu_close_display(&game->display);
}
