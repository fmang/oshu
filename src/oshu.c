#include "oshu.h"

int main(int argc, char **argv)
{
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
		return 1;
	}
	oshu_log_debug("successfully initialized");

	oshu_audio_init();
	oshu_log_debug("initialized the audio module");

	struct oshu_audio *stream;
	if (oshu_audio_open("test.ogg", &stream)) {
		oshu_log_error("no audio, aborting");
		return 2;
	}
	oshu_log_debug("audio opened");

	struct oshu_sample *sample;
	if (oshu_sample_load("hit.wav", &stream->device_spec, &sample)) {
		oshu_log_error("no sample, aborting");
		return 3;
	}

	struct oshu_beatmap *beatmap;
	int rc = oshu_beatmap_load("beatmap.osu", &beatmap);
	if (rc < 0)
		return 4;

	oshu_log_info("starting the playback");
	oshu_audio_play(stream);

	for (int i = 0; i < 12000; i++) {
		SDL_LockAudio();
		double pos = oshu_audio_position(stream);
		int now = pos * 1000;
		while (beatmap->hit_cursor && beatmap->hit_cursor->time < now) {
			oshu_audio_play_sample(stream, sample);
			beatmap->hit_cursor = beatmap->hit_cursor->next;
		}
		SDL_UnlockAudio();
		SDL_Delay(10);
	}

	oshu_audio_close(&stream);
	SDL_Quit();
	return 0;
}
