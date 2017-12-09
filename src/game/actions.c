#include "game/game.h"

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

void oshu_pause_game(struct oshu_game *game)
{
	oshu_pause_audio(&game->audio);
	game->screen = &oshu_pause_screen;
	oshu_print_state(game);
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

void oshu_stop_game(struct oshu_game *game)
{
	game->stop = 1;
}
