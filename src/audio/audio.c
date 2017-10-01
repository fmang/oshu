/**
 * \file audio/audio.c
 * \ingroup audio
 *
 * \brief
 * Manage the audio pipeline, from ffmpeg input to SDL output.
 */

#include "audio/audio.h"
#include "log.h"

#include <assert.h>

/**
 * Size of the SDL audio buffer, in samples.
 *
 * The smaller it is, the less lag.
 *
 * It should be a power of 2 according to SDL's doc.
 */
static const int sample_buffer_size = 2048;

/**
 * Clip a buffer of float audio samples to ensure every sample's value is
 * normalized between -1 and 1.
 *
 * Without this, some audio cards emit an awful noise.
 */
static void clip(float *samples, int nb_samples, int channels)
{
	for (int i = 0; i < nb_samples * channels; ++i) {
		if (samples[i] > 1.)
			samples[i] = 1.;
		else if (samples[i] < -1.)
			samples[i] = -1.;
	}
}

/**
 * Fill SDL's audio buffer, while requesting more frames as needed.
 *
 * libavcodec organize frames by channel in planar mode (LLLLRRRR), while we'd
 * like them to be interleaved as SDL requires (LRLRLRLR). This makes a
 * intricate nesting of loops. In that order: frame loop, sample loop, channel
 * loop. Makes sense.
 *
 * When frames are interleaved, a coarser memcpy does the trick.
 *
 * When the stream is finished, fill what remains of the buffer with silence,
 * because you never know what SDL might do with a left-over buffer. Most
 * likely, it would play the previous buffer over, and over again.
 */
static void audio_callback(void *userdata, Uint8 *buffer, int len)
{
	struct oshu_audio *audio;
	audio = (struct oshu_audio*) userdata;
	int unit = audio->device_spec.channels * sizeof(float);
	assert (len % unit == 0);
	int nb_samples = len / unit;
	float *samples = (float*) buffer;

	int rc = oshu_read_stream(&audio->music, samples, nb_samples);
	if (rc < 0) {
		oshu_log_debug("failed reading samples from the audio stream");
		return;
	} else if (rc < nb_samples) {
		/* fill what remains with silence */
		memset(buffer + rc * unit, 0, len - rc * unit);
	}

	int tracks = sizeof(audio->effects) / sizeof(*audio->effects);
	for (int i = 0; i < tracks; i++)
		oshu_mix_track(&audio->effects[i], samples, nb_samples);

	clip(samples, nb_samples, audio->device_spec.channels);
}

/**
 * Initialize the SDL audio device.
 * \return 0 on success, -1 on error.
 */
static int open_device(struct oshu_audio *audio)
{
	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = audio->music.sample_rate;
	want.format = AUDIO_F32;
	want.channels = 2;
	want.samples = sample_buffer_size;
	want.callback = audio_callback;
	want.userdata = (void*) audio;
	audio->device_id = SDL_OpenAudioDevice(NULL, 0, &want, &audio->device_spec, 0);
	if (audio->device_id == 0) {
		oshu_log_error("failed to open the audio device: %s", SDL_GetError());
		return -1;
	}
	assert (audio->device_spec.freq == audio->music.sample_rate);
	assert (audio->device_spec.format == AUDIO_F32);
	assert (audio->device_spec.channels == 2);
	return 0;
}

int oshu_open_audio(const char *url, struct oshu_audio *audio)
{
	assert (sizeof(float) == 4);
	if (oshu_open_stream(url, &audio->music) < 0)
		goto fail;
	if (open_device(audio) < 0)
		goto fail;
	return 0;
fail:
	oshu_close_audio(audio);
	return -1;
}

void oshu_play_audio(struct oshu_audio *audio)
{
	SDL_PauseAudioDevice(audio->device_id, 0);
}

void oshu_pause_audio(struct oshu_audio *audio)
{
	SDL_PauseAudioDevice(audio->device_id, 1);
}

void oshu_close_audio(struct oshu_audio *audio)
{
	if (audio->device_id)
		SDL_CloseAudioDevice(audio->device_id);
	oshu_close_stream(&audio->music);
}

/**
 * Pick a track for playing sound effects.
 *
 * If one track is inactive, pick it without hesitation. If all the tracks
 * are active, pick the one with the biggest cursor, because there's a good
 * chance it's about to end.
 *
 * Make sure you lock the audio device when calling this function, in order to
 * ensure predictable results.
 */
static struct oshu_track *select_track(struct oshu_audio *audio)
{
	int max_cursor = 0;
	struct oshu_track *best_track = &audio->effects[0];
	int tracks = sizeof(audio->effects) / sizeof(*audio->effects);
	for (int i = 0; i < tracks; ++i) {
		struct oshu_track *c = &audio->effects[i];
		if (c->sample == NULL) {
			return c;
		} else if (c->cursor > max_cursor) {
			max_cursor = c->cursor;
			best_track = c;
		}
	}
	return best_track;
}

void oshu_play_sample(struct oshu_audio *audio, struct oshu_sample *sample)
{
	SDL_LockAudioDevice(audio->device_id);
	struct oshu_track *track = select_track(audio);
	if (track->sample != NULL)
		oshu_log_debug("all the effect tracks are taken, stealing one");
	oshu_start_track(track, sample, 1.);
	SDL_UnlockAudioDevice(audio->device_id);
}
