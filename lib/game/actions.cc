/*2
 * \file game/actions.cc
 * \ingroup game_actions
 */

#include "game/base.h"
#include "game/tty.h"

#include <assert.h>

void oshu_rewind_game(struct oshu::game::base *game, double offset)
{
	oshu_seek_music(&game->audio, game->audio.music.current_timestamp - offset);
	game->clock.now = game->audio.music.current_timestamp;
	game->relinquish();
	oshu_print_state(game);

	assert (game->hit_cursor != NULL);
	while (game->hit_cursor->time > game->clock.now + 1.) {
		game->hit_cursor->state = OSHU_INITIAL_HIT;
		game->hit_cursor = game->hit_cursor->previous;
	}
}

void oshu_forward_game(struct oshu::game::base *game, double offset)
{
	oshu_seek_music(&game->audio, game->audio.music.current_timestamp + offset);
	game->clock.now = game->audio.music.current_timestamp;
	game->relinquish();

	oshu_print_state(game);

	assert (game->hit_cursor != NULL);
	while (game->hit_cursor->time < game->clock.now + 1.) {
		game->hit_cursor->state = OSHU_SKIPPED_HIT;
		game->hit_cursor = game->hit_cursor->next;
	}
}

void oshu_pause_game(struct oshu::game::base *game)
{
	oshu_pause_audio(&game->audio);
	game->paused = true;
	oshu_print_state(game);
}

void oshu_unpause_game(struct oshu::game::base *game)
{
	if (game->clock.now >= 0)
		oshu_play_audio(&game->audio);
	game->paused = false;
	oshu_print_state(game);
}

void oshu_stop_game(struct oshu::game::base *game)
{
	game->stop = 1;
}
