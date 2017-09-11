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
#include <libavutil/time.h>

/**
 * Size of the SDL audio buffer, in samples.
 * The smaller it is, the less lag.
 */
static const int sample_buffer_size = 1024;

/**
 * Conversion map from ffmpeg's audio formats to SDL's.
 *
 * ffmpeg's format may be planar or not, while SDL only accepts interleaved
 * samples. The planar to interleaved conversion is performed by the audio
 * callback when filling SDL's buffer.
 *
 * 64-bit or double samples are not supported by SDL, so they're omitted here
 * and implicitly filled with zero's.
 */
static SDL_AudioFormat format_map[AV_SAMPLE_FMT_NB] = {
	[AV_SAMPLE_FMT_U8] = AUDIO_U8,
	[AV_SAMPLE_FMT_S16] = AUDIO_S16SYS,
	[AV_SAMPLE_FMT_S32] = AUDIO_S32SYS,
	[AV_SAMPLE_FMT_FLT] = AUDIO_F32,
	[AV_SAMPLE_FMT_U8P] = AUDIO_U8,
	[AV_SAMPLE_FMT_S16P] = AUDIO_S16SYS,
	[AV_SAMPLE_FMT_S32P] = AUDIO_S32SYS,
	[AV_SAMPLE_FMT_FLTP] = AUDIO_F32,
};

void oshu_audio_init()
{
	av_register_all();
}

/**
 * Log some helpful information about the decoded audio stream.
 * Meant for debugging more than anything else.
 */
static void dump_stream_info(struct oshu_audio *audio)
{
	struct oshu_stream *stream = &audio->source;
	oshu_log_info("============ Audio information ============");
	oshu_log_info("            Codec: %s.", stream->codec->long_name);
	oshu_log_info("      Sample rate: %d Hz.", stream->decoder->sample_rate);
	oshu_log_info(" Average bit rate: %ld kbps.", stream->decoder->bit_rate / 1000);
	oshu_log_info("    Sample format: %s.", av_get_sample_fmt_name(stream->decoder->sample_fmt));
	oshu_log_info("         Duration: %d seconds.", (int) (stream->stream->duration * stream->time_base));
}

/**
 * Decode a frame.
 *
 * Update #oshu_audio::current_timestamp and reset #oshu_audio::sample_index.
 *
 * Mark the stream as finished on EOF or on ERROR, meaning you must not call
 * this function anymore.
 */
int next_frame(struct oshu_audio *audio)
{
	int rc = oshu_next_frame(&audio->source);
	if (rc < 0) {
		audio->finished = 1;
	} else {
		int64_t ts = audio->source.frame->best_effort_timestamp;
		if (ts > 0)
			audio->current_timestamp = audio->source.time_base * ts;
		audio->sample_index = 0;
	}
	return 0;
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
static void fill_audio(struct oshu_audio *audio, Uint8 *buffer, int len)
{
	AVFrame *frame = audio->source.frame;
	int sample_size = av_get_bytes_per_sample(frame->format);
	int planar = av_sample_fmt_is_planar(frame->format);
	while (len > 0 && !audio->finished) {
		if (planar) {
			while (len > 0 && audio->sample_index < frame->nb_samples) {
				/* Copy 1 sample per iteration. */
				size_t offset = sample_size * audio->sample_index;
				for (int ch = 0; ch < frame->channels; ch++) {
					memcpy(buffer, frame->data[ch] + offset, sample_size);
					buffer += sample_size;
				}
				len -= sample_size * frame->channels;
				audio->sample_index++;
			}
		} else {
			size_t offset = sample_size * audio->sample_index * frame->channels;
			size_t left = sample_size * frame->nb_samples * frame->channels - offset;
			size_t block = (left < len) ? left : len;
			assert (block % (sample_size * frame->channels) == 0);
			memcpy(buffer, frame->data[0] + offset, block);
			buffer += block;
			len -= block;
			audio->sample_index += block / (sample_size * frame->channels);
		}
		if (audio->sample_index >= frame->nb_samples)
			next_frame(audio);
	}
	assert (len >= 0);
	memset(buffer, len, audio->device_spec.silence);
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
	want.freq = audio->source.decoder->sample_rate;
	SDL_AudioFormat fmt = format_map[audio->source.decoder->sample_fmt];
	if (!fmt) {
		oshu_log_error("unsupported sample format");
		return -1;
	}
	want.format = fmt;
	want.channels = audio->source.decoder->channels;
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
	if (oshu_open_stream(url, &(*audio)->source) < 0)
		return -1;
	dump_stream_info(*audio);
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
	oshu_close_stream(&(*audio)->source);
	free(*audio);
	*audio = NULL;
}
