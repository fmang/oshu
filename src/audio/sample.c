/**
 * \file audio/sample.c
 * \ingroup audio_sample
 */

#include "audio/sample.h"
#include "log.h"

#include <assert.h>

/**
 * Convert a sample loaded from a WAV file in order to play it on the currently
 * opened device.
 *
 * Trust me, your ears don't want you to overlap two incompatible streams.
 */
static int convert_audio(SDL_AudioSpec *device_spec, SDL_AudioSpec *wav_spec, struct oshu_sample *sample)
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
	sample->samples = realloc(sample->samples, sample->size * converter.len_mult);
	if (sample->samples == NULL) {
		oshu_log_error("could not resize the samples buffer");
		return -1;
	}
	converter.buf = (Uint8*) sample->samples;
	converter.len = sample->size;
	SDL_ConvertAudio(&converter);
	sample->size = converter.len_cvt;
	/* reclaim the unrequired memory */
	sample->samples = realloc(sample->samples, sample->size);
	assert (sample->samples != NULL);
	return 0;
}

int oshu_load_sample(const char *path, SDL_AudioSpec *spec, struct oshu_sample *sample)
{
	assert (spec->format == AUDIO_F32);
	assert (spec->channels == 2);
	SDL_AudioSpec wav_spec;
	SDL_AudioSpec *wav = SDL_LoadWAV(path, &wav_spec, (Uint8**) &sample->samples, &sample->size);
	if (wav == NULL) {
		oshu_log_error("SDL error when loading the sample: %s", SDL_GetError());
		goto fail;
	}
	if (convert_audio(spec, wav, sample) < 0)
		goto fail;
	sample->nb_samples = sample->size / (2 * sizeof(*sample->samples));
	return 0;
fail:
	oshu_destroy_sample(sample);
	return -1;
}

void oshu_destroy_sample(struct oshu_sample *sample)
{
	if (sample->samples)
		SDL_FreeWAV((Uint8*) sample->samples);
}
