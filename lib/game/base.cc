/**
 * \file game/game.cc
 * \ingroup game
 *
 * Implemention game initialization, destruction and the main loop.
 */

#include "config.h"

#include "core/log.h"
#include "game/base.h"
#include "game/tty.h"

#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL_image.h>

static int open_beatmap(const char *beatmap_path, struct oshu::game_base *game)
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

static int open_audio(struct oshu::game_base *game)
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

namespace oshu {

game_base::game_base(const char *beatmap_path)
{
	if (open_beatmap(beatmap_path, this) < 0)
		throw std::runtime_error("could not load the beatmap");
	if (open_audio(this) < 0)
		throw std::runtime_error("could not open the audio device");
}

game_base::~game_base()
{
	oshu_destroy_beatmap(&beatmap);
	oshu_close_audio(&audio);
	oshu_close_sound_library(&library);
}

void game_base::rewind(double offset)
{
	oshu_seek_music(&this->audio, this->audio.music.current_timestamp - offset);
	this->clock.now = this->audio.music.current_timestamp;
	this->relinquish();
	oshu_print_state(this);

	assert (this->hit_cursor != NULL);
	while (this->hit_cursor->time > this->clock.now + 1.) {
		this->hit_cursor->state = OSHU_INITIAL_HIT;
		this->hit_cursor = this->hit_cursor->previous;
	}
}

void game_base::forward(double offset)
{
	oshu_seek_music(&this->audio, this->audio.music.current_timestamp + offset);
	this->clock.now = this->audio.music.current_timestamp;
	this->relinquish();

	oshu_print_state(this);

	assert (this->hit_cursor != NULL);
	while (this->hit_cursor->time < this->clock.now + 1.) {
		this->hit_cursor->state = OSHU_SKIPPED_HIT;
		this->hit_cursor = this->hit_cursor->next;
	}
}

void game_base::pause()
{
	oshu_pause_audio(&this->audio);
	this->paused = true;
	oshu_print_state(this);
}

void game_base::unpause()
{
	if (this->clock.now >= 0)
		oshu_play_audio(&this->audio);
	this->paused = false;
	oshu_print_state(this);
}

}
