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

	/* 4. Clock */
	if (game->beatmap.audio_lead_in > 0.) {
		game->clock.now = - game->beatmap.audio_lead_in;
	} else {
		double first_hit = game->beatmap.hits->next->time;
		if (first_hit < 1.)
			game->clock.now = first_hit - 1.;
	}

	/* 5. Post-initialization */
	game->autoplay = 0;
	game->stop = 0;
	game->screen = &oshu_play_screen;
	if (game->mode->initialize(game) < 0)
		goto fail;

	return 0;

fail:
	oshu_destroy_game(game);
	return -1;
}

void oshu_pause_game(struct oshu_game *game)
{
	oshu_pause_audio(&game->audio);
	game->screen = &oshu_pause_screen;
	oshu_print_state(game);
}

void oshu_rewind_game(struct oshu_game *game, double offset)
{
	oshu_seek_music(&game->audio, game->audio.music.current_timestamp - offset);
	game->clock.now = game->audio.music.current_timestamp;
	game->mode->relinquish(game);
	oshu_print_state(game);

	assert (game->hit_cursor != NULL);
	while (game->hit_cursor->time > game->clock.now + 1.) {
		game->hit_cursor->state = OSHU_INITIAL_HIT;
		game->hit_cursor = game->hit_cursor->previous;
	}
}

void oshu_forward_game(struct oshu_game *game, double offset)
{
	oshu_seek_music(&game->audio, game->audio.music.current_timestamp + offset);
	game->clock.now = game->audio.music.current_timestamp;
	game->mode->relinquish(game);

	oshu_print_state(game);

	assert (game->hit_cursor != NULL);
	while (game->hit_cursor->time < game->clock.now + 1.) {
		game->hit_cursor->state = OSHU_SKIPPED_HIT;
		game->hit_cursor = game->hit_cursor->next;
	}
}

void oshu_unpause_game(struct oshu_game *game)
{
	if (game->clock.now >= 0) {
		if (!game->autoplay)
			oshu_rewind_game(game, 1.);
		oshu_play_audio(&game->audio);
	}
	game->screen = &oshu_play_screen;
}

enum oshu_finger oshu_translate_key(SDL_Keysym *keysym)
{
	switch (keysym->scancode) {
	/* Bottom row, for standard and taiko modes. */
	case SDL_SCANCODE_Z: return OSHU_LEFT_MIDDLE;
	case SDL_SCANCODE_X: return OSHU_LEFT_INDEX;
	case SDL_SCANCODE_C: return OSHU_RIGHT_INDEX;
	case SDL_SCANCODE_V: return OSHU_RIGHT_MIDDLE;
	/* Middle row, for mania. */
	case SDL_SCANCODE_A: return OSHU_LEFT_PINKY;
	case SDL_SCANCODE_S: return OSHU_LEFT_RING;
	case SDL_SCANCODE_D: return OSHU_LEFT_MIDDLE;
	case SDL_SCANCODE_F: return OSHU_LEFT_INDEX;
	case SDL_SCANCODE_SPACE: return OSHU_THUMBS;
	case SDL_SCANCODE_J: return OSHU_RIGHT_INDEX;
	case SDL_SCANCODE_K: return OSHU_RIGHT_MIDDLE;
	case SDL_SCANCODE_L: return OSHU_RIGHT_RING;
	case SDL_SCANCODE_SEMICOLON: return OSHU_RIGHT_PINKY;
	default:             return OSHU_UNKNOWN_KEY;
	}
}

static void draw(struct oshu_game *game)
{
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display.renderer);
	game->screen->draw(game);
	SDL_RenderPresent(game->display.renderer);
}

/**
 * Update the game clock.
 *
 * It as roughly 2 modes:
 *
 * 1. When the audio has a lead-in time, rely on SDL's ticks to increase the
 *    clock.
 * 2. When the lead-in phase is over, use the audio clock. However, if we
 *    detect it hasn't change, probably because the codec frame is too big, then
 *    we make it progress with the SDL clock anyway.
 *
 * In both cases, we wanna ensure the *now* clock is always monotonous. If we
 * detect the new time is before the previous time, then we stop the time until
 * now catches up with before. That case does happen at least right after the
 * lead-in phase, because the audio starts when the *now* clock becomes
 * positive, while the audio clock will be null at that moment.
 */
static void update_clock(struct oshu_game *game)
{
	struct oshu_clock *clock = &game->clock;
	double system = SDL_GetTicks() / 1000.;
	double diff = system - clock->system;
	double prev_audio = clock->audio;
	clock->audio = game->audio.music.current_timestamp;
	clock->before = clock->now;
	clock->system = system;
	if (game->screen == &oshu_pause_screen)
		; /* don't update the clock */
	else if (clock->before < 0) /* leading in */
		clock->now = clock->before + diff;
	else if (clock->audio == prev_audio) /* audio clock stuck */
		clock->now = clock->before + diff;
	else
		clock->now = clock->audio;
	/* force monotonicity */
	if (clock->now < clock->before)
		clock->now = clock->before;
}

int oshu_run_game(struct oshu_game *game)
{
	oshu_welcome(game);
	/* Reset the clock.
	 * Otherwise, when the startup is slow, the clock would jump. */
	game->clock.system = SDL_GetTicks() / 1000.;
	SDL_Event event;
	int missed_frames = 0;
	while (!game->stop) {
		update_clock(game);
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

struct oshu_hit* oshu_look_hit_back(struct oshu_game *game, double offset)
{
	struct oshu_hit *hit = game->hit_cursor;
	double target = game->clock.now - offset;
	/* seek backward */
	while (oshu_hit_end_time(hit) > target)
		hit = hit->previous;
	/* seek forward */
	while (oshu_hit_end_time(hit) < target)
		hit = hit->next;
	/* here we have the guarantee that hit->time >= target */
	return hit;
}

struct oshu_hit* oshu_look_hit_up(struct oshu_game *game, double offset)
{
	struct oshu_hit *hit = game->hit_cursor;
	double target = game->clock.now + offset;
	/* seek forward */
	while (hit->time < target)
		hit = hit->next;
	/* seek backward */
	while (hit->time > target)
		hit = hit->previous;
	/* here we have the guarantee that hit->time <= target */
	return hit;
}

void oshu_stop_game(struct oshu_game *game)
{
	game->stop = 1;
}
