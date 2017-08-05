/**
 * \file game.c
 * \ingroup game
 *
 * \ref game module implementation.
 */

#include "log.h"
#include "game.h"

static int hit_tolerance = 200 ; /* ms */

int oshu_game_create(const char *beatmap_path, struct oshu_game **game)
{
	*game = calloc(1, sizeof(**game));

	if (oshu_beatmap_load(beatmap_path, &(*game)->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		goto fail;
	}

	if (oshu_audio_open((*game)->beatmap->general.audio_filename, &(*game)->audio) < 0) {
		oshu_log_error("no audio, aborting");
		goto fail;
	}

	if (oshu_sample_load("hit.wav", (*game)->audio, &(*game)->hit_sound) < 0) {
		oshu_log_error("could not load hit.wav, aborting");
		goto fail;
	}

	if (oshu_display_init(&(*game)->display) < 0) {
		oshu_log_error("no display, aborting");
		goto fail;
	}

	return 0;
fail:
	oshu_game_destroy(game);
	return -1;
}

static void hit(struct oshu_game *game, int x, int y)
{
}

static void toggle_pause(struct oshu_game *game)
{
	SDL_LockAudio();
	if (game->paused) {
		oshu_audio_play(game->audio);
		game->paused = 0;
	} else {
		oshu_audio_pause(game->audio);
		game->paused = 1;
	}
	SDL_UnlockAudio();
}

static void handle_event(struct oshu_game *game, SDL_Event *event)
{
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		if (event->key.keysym.sym == SDLK_w)
			hit(game, 0, 0);
		else if (event->key.keysym.sym == SDLK_x)
			hit(game, 0, 0);
		else if (event->key.keysym.sym == SDLK_q)
			game->stop = 1;
		else if (event->key.keysym.sym == SDLK_ESCAPE)
			toggle_pause(game);
		break;
	}
}

static void check_audio(struct oshu_game *game)
{
	int now = game->audio->current_timestamp * 1000;
	while (game->beatmap->hit_cursor && game->beatmap->hit_cursor->time < now) {
		SDL_LockAudio();
		oshu_sample_play(game->audio, game->hit_sound);
		SDL_UnlockAudio();
		game->beatmap->hit_cursor = game->beatmap->hit_cursor->next;
	}
}

int oshu_game_run(struct oshu_game *game)
{
	SDL_Event event;
	oshu_audio_play(game->audio);
	while (!game->audio->finished && !game->stop) {
		while (SDL_PollEvent(&event))
			handle_event(game, &event);
		check_audio(game);
		int now = game->audio->current_timestamp * 1000;
		oshu_draw_beatmap(game->display, game->beatmap, now);
		SDL_Delay(20);
	}
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
	if ((*game)->display)
		oshu_display_destroy(&(*game)->display);
	if ((*game)->beatmap)
		oshu_beatmap_free(&(*game)->beatmap);
	free(*game);
	*game = NULL;
}