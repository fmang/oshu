/**
 * \file oshu.c
 */

#include "beatmap.h"
#include "log.h"
#include "audio.h"
#include "graphics.h"

#include <signal.h>

static volatile int stop;

static void signal_handler(int signum)
{
	stop = 1;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		puts("Usage: oshu beatmap.osu");
		return 5;
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	char *beatmap_file = argv[1];

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
		return 1;
	}
	oshu_log_debug("successfully initialized");

	oshu_audio_init();
	oshu_log_debug("initialized the audio module");

	struct oshu_beatmap *beatmap;
	int rc = oshu_beatmap_load(beatmap_file, &beatmap);
	if (rc < 0)
		return 4;

	struct oshu_audio *stream;
	if (oshu_audio_open(beatmap->general.audio_filename, &stream)) {
		oshu_log_error("no audio, aborting");
		return 2;
	}
	oshu_log_debug("audio opened");

	struct oshu_sample *sample;
	if (oshu_sample_load("hit.wav", stream, &sample)) {
		oshu_log_error("no sample, aborting");
		return 3;
	}

	struct oshu_display *display;
	if (oshu_display_init(&display)) {
		oshu_log_error("no display, aborting");
		return 7;
	}

	oshu_log_info("starting the playback");
	oshu_audio_play(stream);

	while (!stream->finished && !stop) {
		SDL_LockAudio();
		int now = stream->current_timestamp * 1000;
		while (beatmap->hit_cursor && beatmap->hit_cursor->time < now) {
			oshu_sample_play(stream, sample);
			beatmap->hit_cursor = beatmap->hit_cursor->next;
		}
		oshu_draw_beatmap(display, beatmap, now);
		SDL_UnlockAudio();
		SDL_Delay(20);
	}

	oshu_audio_close(&stream);
	oshu_sample_free(&sample);
	oshu_beatmap_free(&beatmap);
	oshu_display_destroy(&display);

	SDL_Quit();
	return 0;
}
