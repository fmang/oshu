/**
 * \file audio.c
 * \ingroup audio
 *
 * \brief
 * Implementation of the audio module and its submodules.
 */

#include "audio.h"
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

/**
 * Spew an error message according to the return value of a call to one of
 * ffmpeg's functions.
 */
static void log_av_error(int rc)
{
	char errbuf[256];
	av_strerror(rc, errbuf, sizeof(errbuf));
	oshu_log_error("ffmpeg error: %s", errbuf);
}

void oshu_audio_init()
{
	av_register_all();
}

/**
 * Read a page for the demuxer and feed it to the decoder.
 *
 * When reaching EOF, feed the decoder a NULL packet to flush it.
 *
 * This function is meant to be called exclusively from #next_frame,
 * because a single page may yield many codec frames.
 *
 * \return 0 on success, -1 on error.
 */
static int next_page(struct oshu_audio *audio)
{
	int rc;
	for (;;) {
		rc = av_read_frame(audio->demuxer, &audio->packet);
		if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last page, flushing");
			rc = avcodec_send_packet(audio->decoder, NULL);
			break;
		} else if (rc < 0) {
			break;
		}
		if (audio->packet.stream_index == audio->stream->index) {
			rc = avcodec_send_packet(audio->decoder, &audio->packet);
			break;
		}
		av_packet_unref(&audio->packet);
	}
	av_packet_unref(&audio->packet);
	if (rc < 0) {
		log_av_error(rc);
		return -1;
	}
	return 0;
}

/**
 * Decode a frame into the stream's #oshu_audio::frame.
 *
 * Request a new page with #next_page when the decoder needs more data.
 *
 * Update #oshu_audio::current_timestamp and reset #oshu_audio::sample_index.
 *
 * Mark the stream as finished on EOF or on ERROR, meaning you must not call
 * this function anymore.
 */
static void next_frame(struct oshu_audio *audio)
{
	for (;;) {
		int rc = avcodec_receive_frame(audio->decoder, audio->frame);
		if (rc == 0) {
			int64_t ts = audio->frame->best_effort_timestamp;
			if (ts > 0)
				audio->current_timestamp = audio->time_base * ts;
			audio->sample_index = 0;
			return;
		} else if (rc == AVERROR(EAGAIN)) {
			if (next_page(audio) < 0)
				goto finish;
		} else if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last frame");
			goto finish;
		} else {
			log_av_error(rc);
			goto finish;
		}
	}
finish:
	audio->finished = 1;
}

/**
 * Open the libavformat demuxer, and find the best audio stream.
 *
 * Fill #oshu_audio::demuxer, #oshu_audio::codec, #oshu_audio::stream and
 * #oshu_audio::time_base.
 *
 * \return 0 on success, -1 on error.
 */
static int open_demuxer(const char *url, struct oshu_audio *audio)
{
	int rc = avformat_open_input(&audio->demuxer, url, NULL, NULL);
	if (rc < 0) {
		oshu_log_error("failed opening the audio file");
		goto fail;
	}
	rc = avformat_find_stream_info(audio->demuxer, NULL);
	if (rc < 0) {
		oshu_log_error("error reading the stream headers");
		goto fail;
	}
	rc = av_find_best_stream(
		audio->demuxer,
		AVMEDIA_TYPE_AUDIO,
		-1, -1,
		&audio->codec,
		0
	);
	if (rc < 0 || audio->codec == NULL) {
		oshu_log_error("error finding the best audio stream");
		goto fail;
	}
	audio->stream = audio->demuxer->streams[rc];
	audio->time_base = av_q2d(audio->stream->time_base);
	return 0;
fail:
	log_av_error(rc);
	return -1;
}

/**
 * Open the libavcodec decoder.
 *
 * You must call this function after #open_demuxer.
 *
 * \return 0 on success, and a negative ffmpeg error code on failure.
 */
static int open_decoder(struct oshu_audio *audio)
{
	audio->decoder = avcodec_alloc_context3(audio->codec);
	int rc = avcodec_parameters_to_context(
		audio->decoder,
		audio->stream->codecpar
	);
	if (rc < 0) {
		oshu_log_error("error copying the codec context");
		goto fail;
	}
	rc = avcodec_open2(audio->decoder, audio->codec, NULL);
	if (rc < 0) {
		oshu_log_error("error opening the codec");
		goto fail;
	}
	audio->frame = av_frame_alloc();
	if (audio->frame == NULL) {
		oshu_log_error("could not allocate the codec frame");
		goto fail;
	}
	return 0;
fail:
	log_av_error(rc);
	return -1;
}

/**
 * Log some helpful information about the decoded audio stream.
 * Meant for debugging more than anything else.
 */
static void dump_stream_info(struct oshu_audio *audio)
{
	oshu_log_info("audio codec: %s", audio->codec->long_name);
	oshu_log_info("sample rate: %d Hz", audio->decoder->sample_rate);
	oshu_log_info("number of channels: %d", audio->decoder->channels);
	oshu_log_info("format: %s", av_get_sample_fmt_name(audio->decoder->sample_fmt));
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
	AVFrame *frame = audio->frame;
	int sample_size = av_get_bytes_per_sample(frame->format);
	int planar = av_sample_fmt_is_planar(frame->format);
	while (len > 0 && !audio->finished) {
		if (audio->sample_index >= frame->nb_samples)
			next_frame(audio);
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
	want.freq = audio->decoder->sample_rate;
	SDL_AudioFormat fmt = format_map[audio->decoder->sample_fmt];
	if (!fmt) {
		oshu_log_error("unsupported sample format");
		return -1;
	}
	want.format = fmt;
	want.channels = audio->decoder->channels;
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
	if (open_demuxer(url, *audio) < 0)
		goto fail;
	if (open_decoder(*audio) < 0)
		goto fail;
	next_frame(*audio);
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
	if ((*audio)->frame)
		av_frame_free(&(*audio)->frame);
	if ((*audio)->decoder)
		avcodec_free_context(&(*audio)->decoder);
	if ((*audio)->demuxer)
		avformat_close_input(&(*audio)->demuxer);
	free(*audio);
	*audio = NULL;
}

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
