/**
 * \file audio/pipeline.c
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
 * The smaller it is, the less lag.
 */
static const int sample_buffer_size = 1024;

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
static void fill_audio(struct oshu_audio *audio, Uint8 *buffer, int len)
{
	int unit = 2 * sizeof(float);
	assert (len % unit == 0);
	int nb_samples = len / unit;
	int rc = oshu_read_stream(&audio->music, (float*) buffer, nb_samples);
	if (rc < 0) {
		return;
	} else if (rc < nb_samples) {
		audio->finished = 1;
		memset(buffer + rc * unit, 0, len - rc * unit);
	}
}

/**
 * Mix the already present data in the buffer with the sample.
 *
 * This should be called right after #fill_audio.
 *
 * Note that SDL's documentation recommends against calling
 * `SDL_MixAudioFormat` more than once, because the clipping would deteriorate
 * the overall sound quality.
 */
static void mix_sample(Uint8 *buffer, int len, struct oshu_audio *audio, struct oshu_sample *sample)
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
			audio->device_spec.format,
			block,
			SDL_MIX_MAXVOLUME
		);
		sample->cursor += block;
		buffer += block;
		len -= block;
	}
}

/**
 * Fill the audio buffer with the song data, then optionally add a sample.
 */
static void audio_callback(void *userdata, Uint8 *buffer, int len)
{
	struct oshu_audio *audio;
	audio = (struct oshu_audio*) userdata;
	fill_audio(audio, buffer, len);
	if (audio->overlay != NULL)
		mix_sample(buffer, len, audio, audio->overlay);
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
	return 0;
}

int oshu_audio_open(const char *url, struct oshu_audio **audio)
{
	*audio = calloc(1, sizeof(**audio));
	if (*audio == NULL) {
		oshu_log_error("could not allocate the audio context");
		return -1;
	}
	if (oshu_open_stream(url, &(*audio)->music) < 0)
		return -1;
	if (open_device(*audio) < 0)
		goto fail;
	return 0;
fail:
	oshu_audio_close(audio);
	return -1;
}

void oshu_audio_play(struct oshu_audio *audio)
{
	SDL_PauseAudioDevice(audio->device_id, 0);
}

void oshu_audio_pause(struct oshu_audio *audio)
{
	SDL_PauseAudioDevice(audio->device_id, 1);
}

void oshu_audio_close(struct oshu_audio **audio)
{
	if (*audio == NULL)
		return;
	if ((*audio)->device_id)
		SDL_CloseAudioDevice((*audio)->device_id);
	oshu_close_stream(&(*audio)->music);
	free(*audio);
	*audio = NULL;
}
