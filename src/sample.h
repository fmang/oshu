#pragma once

#include <SDL2/SDL.h>

struct oshu_sample {
	SDL_AudioSpec *spec;
	Uint8 *buffer;
	Uint32 length;
	Uint32 cursor;
	int loop;
};

int oshu_sample_load(const char *path, SDL_AudioSpec *spec, struct oshu_sample **sample);

void oshu_sample_mix(Uint8 *buffer, int len, struct oshu_sample *sample);

void oshu_sample_free(struct oshu_sample **sample);
