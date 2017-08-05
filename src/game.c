/**
 * \file game.c
 * \ingroup game
 *
 * \ref game module implementation.
 */

#include "log.h"
#include "game.h"

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

int oshu_game_run(struct oshu_game *game)
{
	oshu_audio_play(game->audio);
	while (!game->audio->finished && !game->stop) {
		SDL_LockAudio();
		int now = game->audio->current_timestamp * 1000;
		while (game->beatmap->hit_cursor && game->beatmap->hit_cursor->time < now) {
			oshu_sample_play(game->audio, game->hit_sound);
			game->beatmap->hit_cursor = game->beatmap->hit_cursor->next;
		}
		SDL_UnlockAudio();
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
