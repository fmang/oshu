#include "oshu.h"

int oshu_sample_load(const char *path, SDL_AudioSpec *spec, struct oshu_sample **sample)
{
	*sample = calloc(1, sizeof(**sample));
	(*sample)->spec = spec;
	SDL_AudioSpec *wav = SDL_LoadWAV(path, spec, &(*sample)->buffer, &(*sample)->length);
	if (wav == NULL) {
		free(*sample);
		return -1;
	}
	return 0;
}

void oshu_sample_mix(Uint8 *buffer, int len, struct oshu_sample *sample)
{
	while (len > 0) {
		if (sample->cursor >= sample->length) {
			if (sample->loop)
				sample->cursor = 0;
			else
				break;
		}
		size_t left = sample->length - sample->cursor;
		size_t block = (left < len) ? left : len;
		SDL_MixAudioFormat(
			buffer,
			sample->buffer + sample->cursor,
			sample->spec->format,
			block,
			SDL_MIX_MAXVOLUME
		);
		sample->cursor += block;
		buffer += block;
		len -= block;
	}
}

void oshu_sample_free(struct oshu_sample **sample)
{
	SDL_FreeWAV((*sample)->buffer);
	*sample = NULL;
}
