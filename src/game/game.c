/**
 * \file game/game.c
 * \ingroup game
 *
 * Implemention game initialization, destruction and the main loop.
 */

#include "../config.h"

#include "game/game.h"
#include "game/screen.h"
#include "game/tty.h"
#include "graphics/draw.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL_image.h>

/**
 * How long a frame should last in seconds. 17 is about 60 FPS.
 */
static const double frame_duration = .016666666;

static int open_beatmap(const char *beatmap_path, struct oshu_game *game)
{
	if (oshu_load_beatmap(beatmap_path, &game->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		return -1;
	}
	if (game->beatmap.mode == OSHU_OSU_MODE) {
		game->mode = &osu_mode;
	} else {
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
	char *title;
	if (asprintf(&title, "%s - %s ♯ %s 𝄞 oshu!", meta->artist, meta->title, meta->version) >= 0) {
		SDL_SetWindowTitle(game->display.window, title);
		free(title);
	}
	oshu_load_background(game);
	return 0;
}

int oshu_create_game(const char *beatmap_path, struct oshu_game *game)
{
	game->screen = &oshu_play_screen;
	if (open_beatmap(beatmap_path, game) < 0)
		goto fail;
	if (open_audio(game) < 0)
		goto fail;
	if (open_display(game) < 0)
		goto fail;
	if (game->mode->initialize(game) < 0)
		goto fail;
	return 0;
fail:
	oshu_destroy_game(game);
	return -1;
}

static void draw(struct oshu_game *game)
{
	oshu_reset_view(&game->display);
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display.renderer);
	oshu_show_background(game);
	game->screen->draw(game);
	SDL_RenderPresent(game->display.renderer);
}

int oshu_run_game(struct oshu_game *game)
{
	oshu_welcome(game);
	oshu_initialize_clock(game);

	SDL_Event event;
	int missed_frames = 0;

	while (!game->stop) {
		oshu_update_clock(game);
		while (SDL_PollEvent(&event))
			game->screen->on_event(game, &event);
		game->screen->update(game);
		draw(game);

		double advance = frame_duration - (SDL_GetTicks() / 1000. - game->clock.system);
		if (advance > 0) {
			SDL_Delay(advance * 1000);
		} else {
			missed_frames++;
			if (missed_frames == 1000)
				oshu_log_warning("your computer is having a hard time keeping up 60 FPS");
		}
	}

	oshu_log_debug("%d missed frames", missed_frames);
	return 0;
}

void oshu_destroy_game(struct oshu_game *game)
{
	assert (game != NULL);
	if (game->mode)
		game->mode->destroy(game);
	oshu_destroy_beatmap(&game->beatmap);
	oshu_close_audio(&game->audio);
	oshu_close_sound_library(&game->library);
	oshu_free_background(game);
	oshu_close_display(&game->display);
}
