/**
 * \file game/game.c
 * \ingroup game
 *
 * Game module implementation.
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

int oshu_create_game(const char *beatmap_path, struct oshu_game *game)
{
	/* 1. Beatmap */
	if (oshu_load_beatmap(beatmap_path, &game->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		goto fail;
	}
	if (game->beatmap.mode == OSHU_OSU_MODE) {
		game->mode = &osu_mode;
	} else {
		oshu_log_error("unsupported game mode");
		goto fail;
	}
	assert (game->beatmap.hits != NULL);
	game->hit_cursor = game->beatmap.hits;

	/* 2. Audio */
	assert (game->beatmap.audio_filename != NULL);
	if (oshu_open_audio(game->beatmap.audio_filename, &game->audio) < 0) {
		oshu_log_error("no audio, aborting");
		goto fail;
	}
	oshu_open_sound_library(&game->library, &game->audio.device_spec);
	oshu_populate_library(&game->library, &game->beatmap);

	/* 3. Display */
	if (oshu_open_display(&game->display) < 0) {
		oshu_log_error("no display, aborting");
		goto fail;
	}
	char *title;
	if (asprintf(&title, "%s - oshu!", beatmap_path) >= 0) {
		SDL_SetWindowTitle(game->display.window, title);
		free(title);
	}
	if (game->beatmap.background_filename)
		oshu_load_texture(&game->display, game->beatmap.background_filename, &game->background);

	/* 4. Post-initialization */
	game->screen = &oshu_play_screen;
	if (game->mode->initialize(game) < 0)
		goto fail;

	return 0;

fail:
	oshu_destroy_game(game);
	return -1;
}

static void draw(struct oshu_game *game)
{
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display.renderer);
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
	oshu_destroy_texture(&game->background);
	oshu_close_display(&game->display);
}
