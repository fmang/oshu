/**
 * \file game/game.c
 * \ingroup game
 *
 * Game module implementation.
 */

#include "game/game.h"
#include "graphics/draw.h"
#include "log.h"

#include <unistd.h>

#include <SDL2/SDL_image.h>

/**
 * How long a frame should last in milleseconds. 17 is about 60 FPS.
 */
static const double frame_duration = 17;

int oshu_game_create(const char *beatmap_path, struct oshu_game **game)
{
	*game = calloc(1, sizeof(**game));

	if (oshu_beatmap_load(beatmap_path, &(*game)->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		goto fail;
	}
	if ((*game)->beatmap->mode == OSHU_MODE_OSU) {
		(*game)->mode = &oshu_osu_mode;
	} else {
		oshu_log_error("unsupported game mode");
		goto fail;
	}

	if (oshu_audio_open((*game)->beatmap->audio_filename, &(*game)->audio) < 0) {
		oshu_log_error("no audio, aborting");
		goto fail;
	}

	const char *hit_path = "hit.wav";
	if (access(hit_path, F_OK) != 0)
		hit_path = PKGDATADIR "/hit.wav";
	oshu_log_debug("loading %s", hit_path);
	if (oshu_sample_load(hit_path, (*game)->audio, &(*game)->hit_sound) < 0) {
		oshu_log_error("could not load hit.wav, aborting");
		goto fail;
	}

	if (oshu_open_display(&(*game)->display) < 0) {
		oshu_log_error("no display, aborting");
		goto fail;
	}
	(*game)->display->system = OSHU_GAME_COORDINATES;

	if ((*game)->beatmap->background_file) {
		(*game)->background = IMG_LoadTexture((*game)->display->renderer, (*game)->beatmap->background_file);
		if ((*game)->background)
			SDL_SetTextureColorMod((*game)->background, 64, 64, 64);
	}

	if ((*game)->beatmap->audio_lead_in > 0) {
		(*game)->clock.now = - (*game)->beatmap->audio_lead_in;
	} else {
		double first_hit = (*game)->beatmap->hits->time;
		if (first_hit < 1.)
			(*game)->clock.now = first_hit - 1.;
	}
	(*game)->clock.ticks = SDL_GetTicks();

	return 0;
fail:
	oshu_game_destroy(game);
	return -1;
}

static void pause_game(struct oshu_game *game)
{
	oshu_audio_pause(game->audio);
	game->paused = 1;
}

static void toggle_pause(struct oshu_game *game)
{
	if (game->paused) {
		oshu_audio_play(game->audio);
		game->paused = 0;
	} else {
		pause_game(game);
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
		switch (event->key.keysym.sym) {
		case SDLK_q:
			game->stop = 1;
			break;
		case SDLK_ESCAPE:
		case SDLK_SPACE:
			toggle_pause(game);
			break;
		default:
			if (!game->paused && !game->autoplay && game->mode->key_pressed)
				game->mode->key_pressed(game, &event->key.keysym);
		}
		break;
	case SDL_KEYUP:
		if (!game->paused && !game->autoplay && game->mode->key_pressed)
			game->mode->key_released(game, &event->key.keysym);
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (!game->paused && !game->autoplay && game->mode->mouse_released)
			game->mode->mouse_pressed(game, event->button.button);
		break;
	case SDL_MOUSEBUTTONUP:
		if (!game->paused && !game->autoplay && game->mode->mouse_released)
			game->mode->mouse_released(game, event->button.button);
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			oshu_resize_display(game->display);
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
	if (!game->audio->finished)
		return;
	int good = 0;
	int missed = 0;
	for (struct oshu_hit *hit = game->beatmap->hits; hit; hit = hit->next) {
		if (hit->state == OSHU_HIT_MISSED)
			missed++;
		else if (hit->state == OSHU_HIT_GOOD)
			good++;
	}
	printf(
		"\n"
		"*** Score ***\n"
		"  Good: %d\n"
		"  Miss: %d\n"
		"\n",
		good,
		missed
	);
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
	int minutes = (int) game->clock.now / 60.;
	double seconds = game->clock.now - (minutes * 60.);
	const char *state = game->paused ? " Paused" : "Playing";
	printf("%s: %d:%06.3f\r", state, minutes, seconds);
	fflush(stdout);
}

static void draw(struct oshu_game *game)
{
	SDL_SetRenderDrawColor(game->display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display->renderer);
	if (game->background)
		oshu_draw_background(game->display, game->background);
	if (game->mode->draw)
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
	clock->audio = game->audio->current_timestamp;
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

int oshu_game_run(struct oshu_game *game)
{
	SDL_Event event;
	if (!game->paused && game->clock.now >= 0)
		oshu_audio_play(game->audio);
	while (!game->audio->finished && !game->stop) {
		update_clock(game);
		if (game->clock.before < 0 && game->clock.now >= 0)
			oshu_audio_play(game->audio);
		while (SDL_PollEvent(&event))
			handle_event(game, &event);
		if (!game->paused && game->mode->check)
			game->mode->check(game);
		draw(game);
		dump_state(game);
		long int advance = frame_duration - (SDL_GetTicks() - game->clock.ticks);
		if (advance > 0)
			SDL_Delay(advance);
	}
	end(game);
	return 0;
}

void oshu_game_destroy(struct oshu_game **game)
{
	if (!game)
		return;
	if (!*game)
		return;
	if ((*game)->audio)
		oshu_audio_close(&(*game)->audio);
	if ((*game)->hit_sound)
		oshu_sample_free(&(*game)->hit_sound);
	if ((*game)->background)
		SDL_DestroyTexture((*game)->background);
	if ((*game)->display)
		oshu_close_display(&(*game)->display);
	if ((*game)->beatmap)
		oshu_beatmap_free(&(*game)->beatmap);
	free(*game);
	*game = NULL;
}
