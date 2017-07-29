#include "oshu.h"

#include <assert.h>
#include <libavutil/time.h>

/** Size of the SDL audio buffer, in samples.
 *  This is 23 ms in 44.1 KHz stereo. */
#define SAMPLE_BUFFER_SIZE 2048

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
 * Open the libavformat demuxer, and find the best audio stream.
 * @return 0 on success and an ffmpeg error code on failure.
 */
static int open_demuxer(const char *url, struct oshu_audio *stream)
{
	int rc = avformat_open_input(&stream->demuxer, url, NULL, NULL);
	if (rc < 0) {
		oshu_log_error("failed opening the audio file");
		return rc;
	}
	rc = avformat_find_stream_info(stream->demuxer, NULL);
	if (rc < 0) {
		oshu_log_error("error reading the stream headers");
		return rc;
	}
	rc = av_find_best_stream(
		stream->demuxer,
		AVMEDIA_TYPE_AUDIO,
		-1, -1,
		&stream->codec,
		0
	);
	if (rc < 0 || stream->codec == NULL) {
		oshu_log_error("error finding the best audio stream");
		return rc;
	}
	stream->stream_index = rc;
	return 0;
}

/**
 * Read a page for the stream and feed it to the decoder.
 * When reaching EOF, feed the decoder a NULL packet to flush it.
 * On error, log a message and mark the stream as finished to stop the
 * playback.
 */
static void next_page(struct oshu_audio *stream)
{
	int rc;
	AVPacket packet;
	for (;;) {
		rc = av_read_frame(stream->demuxer, &packet);
		if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last page, flushing");
			if ((rc = avcodec_send_packet(stream->decoder, NULL)))
				goto fail;
			else
				return;
		} else if (rc < 0) {
			goto fail;
		}
		if (packet.stream_index == stream->stream_index) {
			if ((rc = avcodec_send_packet(stream->decoder, &packet)))
				goto fail;
			return;
		}
	}
fail:
	log_av_error(rc);
	stream->finished = 1;
}

/**
 * Decode a frame into stream->frame.
 * Request a new page when the decoder needs more data.
 * Mark the stream as finished on EOF or on ERROR.
 */
static void next_frame(struct oshu_audio *stream)
{
	while (!stream->finished) {
		int rc = avcodec_receive_frame(stream->decoder, stream->frame);
		if (rc == 0) {
			stream->relative_timestamp = av_gettime_relative();
			stream->sample_index = 0;
			return;
		} else if (rc == AVERROR(EAGAIN)) {
			next_page(stream);
		} else if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last frame");
			stream->finished = 1;
		} else {
			/* Abandon all hope */
			log_av_error(rc);
			stream->finished = 1;
		}
	}
}

/**
 * Initialize the libavcodec decoder.
 * @return an ffmpeg error code on failure, and 0 on success.
 */
static int open_decoder(struct oshu_audio *stream)
{
	stream->decoder = avcodec_alloc_context3(stream->codec);
	int rc = avcodec_parameters_to_context(
		stream->decoder,
		stream->demuxer->streams[stream->stream_index]->codecpar
	);
	if (rc < 0) {
		oshu_log_error("error copying the codec context");
		return rc;
	}
	rc = avcodec_open2(stream->decoder, stream->codec, NULL);
	if (rc < 0) {
		oshu_log_error("error opening the codec");
		return rc;
	}
	stream->frame = av_frame_alloc();
	next_frame(stream);
	return 0;
}

/**
 * Log some helpful information about the decoded audio stream.
 * Meant for debugging more than anything else.
 */
static void dump_stream_info(struct oshu_audio *stream)
{
	oshu_log_info("audio codec: %s", stream->codec->long_name);
	oshu_log_info("sample rate: %d Hz", stream->decoder->sample_rate);
	oshu_log_info("number of channels: %d", stream->decoder->channels);
	oshu_log_info("format: %s", av_get_sample_fmt_name(stream->decoder->sample_fmt));
}

/**
 * Fill SDL's audio buffer, while requesting more frames as needed.
 *
 * libavcodec organize frames by channel (LLLLRRRR), while we'd like them to be
 * interleaved as SDL requires (LRLRLRLR). This makes a intricate nesting of
 * loops. In that order: frame loop, sample loop, channel loop. Makes sense.
 *
 * When the stream is finished, fill what remains of the buffer with silence,
 * because you never know what SDL might do with a left-over buffer. Most
 * likely, it would play the previous buffer over, and over again.
 */
static void audio_callback(void *userdata, Uint8 *buffer, int len)
{
	struct oshu_audio *stream;
	stream = (struct oshu_audio*) userdata;
	AVFrame *frame = stream->frame;
	int sample_size = av_get_bytes_per_sample(frame->format);
	int left = len;
	for (; left > 0 && !stream->finished; next_frame(stream)) {
		while (left > 0 && stream->sample_index < frame->nb_samples) {
			size_t offset = sample_size * stream->sample_index;
			for (int ch = 0; ch < frame->channels; ch++) {
				memcpy(buffer, frame->data[ch] + offset, sample_size);
				buffer += sample_size;
			}
			left -= sample_size * frame->channels;
			stream->sample_index++;
		}
	}
	assert (left >= 0);
	memset(buffer, left, stream->device_spec.silence);
}

/**
 * Initialize the SDL audio device.
 * @return 0 on success, -1 on error.
 */
static int open_device(struct oshu_audio *stream)
{
	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = stream->decoder->sample_rate;
	SDL_AudioFormat fmt = format_map[stream->decoder->sample_fmt];
	if (!fmt) {
		oshu_log_error("unsupported sample format");
		return -1;
	}
	want.format = fmt;
	want.channels = stream->decoder->channels;
	want.samples = SAMPLE_BUFFER_SIZE;
	want.callback = audio_callback;
	want.userdata = (void*) stream;
	stream->device_id = SDL_OpenAudioDevice(NULL, 0, &want, &stream->device_spec, 0);
	if (stream->device_id == 0)
		return -1;
	return 0;
}

int oshu_audio_open(const char *url, struct oshu_audio **stream)
{
	*stream = calloc(1, sizeof(**stream));
	int rc = open_demuxer(url, *stream);
	if (rc < 0)
		goto fail_av;
	rc = open_decoder(*stream);
	if (rc < 0)
		goto fail_av;
	dump_stream_info(*stream);
	rc = open_device(*stream);
	if (rc < 0) {
		oshu_log_error("failed to open the audio device: %s", SDL_GetError());
		goto fail;
	}
	return 0;
fail_av:
	log_av_error(rc);
fail:
	oshu_audio_close(stream);
	return -1;
}

void oshu_audio_play(struct oshu_audio *stream)
{
	SDL_PauseAudioDevice(stream->device_id, 0);
}

double oshu_audio_position(struct oshu_audio *stream)
{
	double base = av_q2d(stream->decoder->time_base);
	double timestamp = stream->frame->best_effort_timestamp;
	double delta_us = av_gettime_relative() - stream->relative_timestamp;
	return (timestamp * base) + (delta_us / 1e6);
}

void oshu_audio_close(struct oshu_audio **stream)
{
	if ((*stream)->device_id)
		SDL_CloseAudioDevice((*stream)->device_id);
	av_frame_free(&(*stream)->frame);
	avcodec_close((*stream)->decoder);
	avformat_close_input(&(*stream)->demuxer);
	free(*stream);
	*stream = NULL;
}
