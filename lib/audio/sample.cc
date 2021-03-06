/**
 * \file audio/sample.cc
 * \ingroup audio_sample
 */

#include "audio/sample.h"
#include "core/log.h"

#include <SDL2/SDL.h>

#include <assert.h>

/** Work in stereo. */
static const int channels = 2;

/**
 * Convert a sample loaded from a WAV file in order to play it on the currently
 * opened device.
 *
 * Trust me, your ears don't want you to overlap two incompatible streams.
 */
static int convert_audio(SDL_AudioSpec *device_spec, SDL_AudioSpec *wav_spec, oshu::sample *sample)
{
	SDL_AudioCVT converter;
	int rc = SDL_BuildAudioCVT(&converter,
		wav_spec->format, wav_spec->channels, wav_spec->freq,
		device_spec->format, device_spec->channels, device_spec->freq
	);
	if (rc == 0) {
		/* no conversion needed */
		return 0;
	} else if (rc < 0) {
		oshu_log_error("SDL audio conversion error: %s", SDL_GetError());
		return -1;
	}
	sample->samples = (float*) realloc(sample->samples, sample->size * converter.len_mult);
	if (sample->samples == NULL) {
		oshu_log_error("could not resize the samples buffer");
		return -1;
	}
	converter.buf = (Uint8*) sample->samples;
	converter.len = sample->size;
	if ((rc = SDL_ConvertAudio(&converter))) {
		oshu_log_error("SDL audio conversion error while converting: %s", SDL_GetError());
		return -1;
	}
	sample->size = converter.len_cvt;
	/* reclaim the unrequired memory */
	sample->samples = (float*) realloc(sample->samples, sample->size);
	assert (sample->size == 0 || sample->samples != NULL);
	return 0;
}

int oshu::load_sample(const char *path, SDL_AudioSpec *spec, oshu::sample *sample)
{
	assert (spec->format == AUDIO_F32);
	assert (spec->channels == channels);
	SDL_AudioSpec wav_spec;
	SDL_AudioSpec *wav = SDL_LoadWAV(path, &wav_spec, (Uint8**) &sample->samples, &sample->size);
	if (wav == NULL) {
		oshu_log_debug("SDL error when loading the sample: %s", SDL_GetError());
		goto fail;
	}
	if (convert_audio(spec, wav, sample) < 0)
		goto fail;
	sample->nb_samples = sample->size / (channels * sizeof(*sample->samples));
	return 0;
fail:
	oshu::destroy_sample(sample);
	return -1;
}

void oshu::destroy_sample(oshu::sample *sample)
{
	if (sample->samples) {
		SDL_FreeWAV((Uint8*) sample->samples);
		sample->samples = NULL;
	}
}
