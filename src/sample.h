#pragma once

#include <SDL2/SDL.h>

/**
 * @defgroup sample Sample
 * @ingroup audio
 *
 * This sample module provides a means to load WAV files using SDL's WAV
 * loader.
 *
 * A sample is a short sound played when the user hits something. To be fast
 * and reactive, the samples are always stored in-memory.
 *
 * Use \ref oshu_audio_play_sample to start playing a sample on top of the
 * currently playing audio.
 *
 * @{
 */

/**
 * An in-memory audio sample.
 */
struct oshu_sample {
	SDL_AudioSpec *spec;
	Uint8 *buffer;
	Uint32 length;
	Uint32 cursor;
	int loop;
};

/**
 * Open a WAV file and store it into a newly-allocated structure.
 *
 * The specs of the SDL audio device is required in order to convert the
 * samples into the apprioriate format.
 *
 * On success, you must free the sample object with \ref oshu_sample_free. On
 * failure, the object is already free'd.
 *
 * @param sample Receive the sample object.
 * @return 0 on success, -1 on failure.
 */
int oshu_sample_load(const char *path, SDL_AudioSpec *spec, struct oshu_sample **sample);

/**
 * Mix the already present data in the buffer with the sample.
 *
 * You should not call this directly. It's the audio module's job to use this
 * in its buffer filling callback, right after it filled the buffer with the
 * playing song.
 */
void oshu_sample_mix(Uint8 *buffer, int len, struct oshu_sample *sample);

/**
 * Free the sample object along with its buffer data.
 *
 * Set `*sample` to *NULL.
 */
void oshu_sample_free(struct oshu_sample **sample);

/** @} */
