/**
 * \file game/game.c
 * \ingroup game
 *
 * Game module implementation.
 */

#include "../config.h"

#include "game/game.h"
#include "graphics/draw.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL_image.h>

/**
 * How long a frame should last in milleseconds. 17 is about 60 FPS.
 */
static const double frame_duration = 17;

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
	game->mode->adjust(game);
	if (game->beatmap.background_filename) {
		game->background = IMG_LoadTexture(game->display.renderer, game->beatmap.background_filename);
		if (game->background)
			SDL_SetTextureColorMod(game->background, 64, 64, 64);
	}

	/* 4. Clock */
	if (game->beatmap.audio_lead_in > 0.) {
		game->clock.now = - game->beatmap.audio_lead_in;
	} else {
		double first_hit = game->beatmap.hits->next->time;
		if (first_hit < 1.)
			game->clock.now = first_hit - 1.;
	}
	game->clock.ticks = SDL_GetTicks();

	return 0;

fail:
	oshu_destroy_game(game);
	return -1;
}

/**
 * Show the state of the game (paused/playing) and the current song position.
 *
 * Only do that for terminal outputs in order not to spam something if the
 * output is redirected.
 *
 * The state length must not decrease over time, otherwise you end up with
 * glitches. If you write `foo\rx`, you get `xoo`. This is the reason the
 * Paused string literal has an extra space.
 */
static void dump_state(struct oshu_game *game)
{
	if (!isatty(fileno(stdout)))
		return;
	int minutes = game->clock.now / 60.;
	double seconds = game->clock.now - minutes * 60.;
	const char *state = game->paused ? " Paused" : "Playing";
	double duration = game->audio.music.duration;
	int duration_minutes = duration / 60.;
	double duration_seconds = duration - duration_minutes * 60;
	printf(
		"%s: %d:%06.3f / %d:%06.3f\r",
		state, minutes, seconds,
		duration_minutes, duration_seconds
	);
	fflush(stdout);
}

static void pause_game(struct oshu_game *game)
{
	oshu_pause_audio(&game->audio);
	game->paused = 1;
	dump_state(game);
}

static void unpause_game(struct oshu_game *game)
{
	oshu_play_audio(&game->audio);
	game->paused = 0;
}

static void rewind_music(struct oshu_game *game, double offset)
{
	oshu_seek_music(&game->audio, game->audio.music.current_timestamp - offset);
	game->clock.now = game->audio.music.current_timestamp;
	game->mode->relinquish(game);
	dump_state(game);

	assert (game->hit_cursor != NULL);
	while (game->hit_cursor->time > game->clock.now) {
		game->hit_cursor->state = OSHU_INITIAL_HIT;
		game->hit_cursor = game->hit_cursor->previous;
	}
}

static void forward_music(struct oshu_game *game, double offset)
{
	oshu_seek_music(&game->audio, game->audio.music.current_timestamp + offset);
	game->clock.now = game->audio.music.current_timestamp;
	game->mode->relinquish(game);
	dump_state(game);

	assert (game->hit_cursor != NULL);
	while (game->hit_cursor->time < game->clock.now) {
		game->hit_cursor->state = OSHU_SKIPPED_HIT;
		game->hit_cursor = game->hit_cursor->next;
	}
}

enum oshu_key translate_key(SDL_Keysym *keysym)
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

/**
 * React to an event got from SDL.
 */
static void handle_event(struct oshu_game *game, SDL_Event *event)
{
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		if (game->paused) {
			switch (event->key.keysym.sym) {
			case SDLK_q:
				game->stop = 1;
				break;
			case SDLK_ESCAPE:
				unpause_game(game);
				break;
			case SDLK_PAGEUP:
				rewind_music(game, 5.);
				break;
			case SDLK_PAGEDOWN:
				forward_music(game, 20.);
				break;
			}
		} else {
			switch (event->key.keysym.sym) {
			case SDLK_ESCAPE:
				pause_game(game);
				break;
			case SDLK_PAGEUP:
				rewind_music(game, 5.);
				break;
			case SDLK_PAGEDOWN:
				forward_music(game, 20.);
				break;
			default:
				if (!game->autoplay) {
					enum oshu_key key = translate_key(&event->key.keysym);
					if (key != OSHU_UNKNOWN_KEY)
						game->mode->press(game, key);
				}
			}
		}
		break;
	case SDL_KEYUP:
		if (!game->paused && !game->autoplay) {
			enum oshu_key key = translate_key(&event->key.keysym);
			if (key != OSHU_UNKNOWN_KEY)
				game->mode->release(game, key);
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (!game->paused && !game->autoplay)
			game->mode->press(game, OSHU_LEFT_BUTTON);
		break;
	case SDL_MOUSEBUTTONUP:
		if (!game->paused && !game->autoplay)
			game->mode->release(game, OSHU_LEFT_BUTTON);
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			game->mode->adjust(game);
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			if (!game->autoplay)
				pause_game(game);
			break;
		case SDL_WINDOWEVENT_CLOSE:
			game->stop = 1;
			break;
		}
		break;
	}
}

/**
 * Print the score if the song was finished, then exit.
 */
static void end(struct oshu_game *game)
{
	/* Clear the status line. */
	printf("\r                                        \r");
	/* Compute the score. */
	int good = 0;
	int missed = 0;
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->state == OSHU_MISSED_HIT)
			missed++;
		else if (hit->state == OSHU_GOOD_HIT)
			good++;
	}
	double rate = (double) good / (good + missed);
	printf(
		"  \033[1mScore:\033[0m\n"
		"  \033[%dm%3d\033[0m good\n"
		"  \033[%dm%3d\033[0m miss\n"
		"\n",
		rate >= 0.9 ? 32 : 0, good,
		rate < 0.5  ? 31 : 0, missed
	);
}

static void draw(struct oshu_game *game)
{
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display.renderer);
	if (game->background)
		oshu_draw_background(&game->display, game->background);
	game->mode->draw(game);
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
	long int ticks = SDL_GetTicks();
	double diff = (double) (ticks - clock->ticks) / 1000.;
	double prev_audio = clock->audio;
	clock->audio = game->audio.music.current_timestamp;
	clock->before = clock->now;
	clock->ticks = ticks;
	if (game->paused)
		; /* don't update the clock */
	else if (clock->before < 0) /* leading in */
		clock->now = clock->before + diff;
	else if (clock->audio == prev_audio) /* audio clock stuck */
		clock->now = clock->before + diff;
	else
		clock->now = clock->audio;
	/* force monoticity */
	if (clock->now < clock->before)
		clock->now = clock->before;
}

static void welcome(struct oshu_game *game)
{
	struct oshu_beatmap *beatmap = &game->beatmap;
	struct oshu_metadata *meta = &beatmap->metadata;
	printf(
		"\n"
		"  \033[33m%s\033[0m // %s\n"
		"  \033[33m%s\033[0m // %s\n",
		meta->title_unicode, meta->title,
		meta->artist_unicode, meta->artist
	);
	if (meta->source)
		printf("  From %s\n", meta->source);

	printf("\n  \033[34m%s\033[0m\n", meta->version);
	if (meta->creator)
		printf("  By %s\n", meta->creator);

	int stars = beatmap->difficulty.overall_difficulty;
	double half_star = beatmap->difficulty.overall_difficulty - stars;
	printf("  ");
	for (int i = 0; i < stars; i++)
		printf("★ ");
	if (half_star >= .5)
		printf("☆ ");

	printf("\n\n");
}

int oshu_run_game(struct oshu_game *game)
{
	welcome(game);
	SDL_Event event;
	int missed_frames = 0;
	if (!game->paused && game->clock.now >= 0)
		oshu_play_audio(&game->audio);
	while (!game->audio.music.finished && !game->stop) {
		update_clock(game);
		if (game->clock.before < 0 && game->clock.now >= 0)
			oshu_play_audio(&game->audio);
		while (SDL_PollEvent(&event))
			handle_event(game, &event);
		if (!game->paused && !game->autoplay)
			game->mode->check(game);
		else if (!game->paused && game->autoplay)
			game->mode->autoplay(game);
		draw(game);
		if (!game->paused)
			dump_state(game);
		long int advance = frame_duration - (SDL_GetTicks() - game->clock.ticks);
		if (advance > 0) {
			SDL_Delay(advance);
		} else {
			missed_frames++;
			if (missed_frames == 1000)
				oshu_log_warning("your computer is having a hard time keeping up 60 FPS");
		}
	}
	end(game);
	return 0;
}

void oshu_destroy_game(struct oshu_game *game)
{
	assert (game != NULL);
	oshu_destroy_beatmap(&game->beatmap);
	oshu_close_audio(&game->audio);
	oshu_close_sound_library(&game->library);
	if (game->background) {
		SDL_DestroyTexture(game->background);
		game->background = NULL;
	}
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
