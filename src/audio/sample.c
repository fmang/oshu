/**
 * \file audio/sample.c
 * \ingroup audio
 */

#include "audio/audio.h"
#include "log.h"

#include <assert.h>

void oshu_sample_play(struct oshu_audio *audio, struct oshu_sample *sample)
{
	SDL_LockAudioDevice(audio->device_id);
	audio->overlay = sample;
	if (sample)
		sample->cursor = 0;
	SDL_UnlockAudioDevice(audio->device_id);
}

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
	sample->buffer = realloc(sample->buffer, sample->length * converter.len_mult);
	if (sample->buffer == NULL) {
		oshu_log_error("could not resize the sample buffer");
		return -1;
	}
	converter.buf = sample->buffer;
	converter.len = sample->length;
	SDL_ConvertAudio(&converter);
	sample->length = converter.len_cvt;
	/* reclaim the unrequired memory */
	sample->buffer = realloc(sample->buffer, sample->length);
	assert (sample->buffer != NULL);
	return 0;
}

int oshu_sample_load(const char *path, struct oshu_audio *audio, struct oshu_sample **sample)
{
	SDL_AudioSpec spec;
	*sample = calloc(1, sizeof(**sample));
	if (*sample == NULL) {
		oshu_log_error("could not allocate the memory for the sample object");
		return -1;
	}
	SDL_AudioSpec *wav = SDL_LoadWAV(path, &spec, &(*sample)->buffer, &(*sample)->length);
	if (wav == NULL) {
		oshu_log_error("SDL error when loading sample: %s", SDL_GetError());
		goto fail;
	}
	if (convert_audio(&audio->device_spec, wav, *sample) < 0)
		goto fail;
	return 0;
fail:
	oshu_sample_free(sample);
	return -1;
}

void oshu_sample_free(struct oshu_sample **sample)
{
	if (*sample == NULL)
		return;
	if ((*sample)->buffer)
		SDL_FreeWAV((*sample)->buffer);
	free(*sample);
	*sample = NULL;
}
