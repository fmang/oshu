/**
 * \file audio/stream.c
 * \ingroup audio_stream
 */

#include "audio/stream.h"
#include "log.h"

#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#include <assert.h>

/** Work in stereo. */
static const int channels = 2;

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

/**
 * Read a page for the demuxer and feed it to the decoder.
 *
 * When reaching EOF, feed the decoder a NULL packet to flush it.
 *
 * Beware: a single page may yield many codec frames. Only call this function
 * when the decoder returns EAGAIN.
 *
 * The management of the AVPacket object comes from the filtering_audio.c
 * example file from ffmpeg's documentation.
 *
 * \return 0 on success, -1 on error.
 */
static int next_page(struct oshu_stream *stream)
{
	int rc;
	AVPacket packet;
	for (;;) {
		rc = av_read_frame(stream->demuxer, &packet);
		if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last page, flushing");
			rc = avcodec_send_packet(stream->decoder, NULL);
			break;
		} else if (rc < 0) {
			break;
		}
		if (packet.stream_index == stream->stream->index) {
			rc = avcodec_send_packet(stream->decoder, &packet);
			av_packet_unref(&packet);
			break;
		}
	}
	if (rc < 0) {
		oshu_log_error("failed reading a page from the audio stream");
		log_av_error(rc);
		return -1;
	}
	return 0;
}

/**
 * Read the next frame from the stream into #oshu_stream::frame.
 *
 * When the end of file is reached, or when an error occurs, set
 * #oshu_stream::finished to true.
 */
static int next_frame(struct oshu_stream *stream)
{
	for (;;) {
		int rc = avcodec_receive_frame(stream->decoder, stream->frame);
		if (rc == 0) {
			int64_t ts = stream->frame->best_effort_timestamp;
			if (ts > 0)
				stream->current_timestamp = stream->time_base * ts;
			stream->sample_index = 0;
			return 0;
		} else if (rc == AVERROR(EAGAIN)) {
			if (next_page(stream) < 0) {
				oshu_log_warning("abrupt end of stream");
				return -1;
			}
		} else if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last frame");
			stream->finished = 1;
			return 0;
		} else {
			oshu_log_error("frame decoding failed");
			log_av_error(rc);
			stream->finished = 1;
			return -1;
		}
	}
}

/**
 * Convert a frame, starting from the sample at position *index*.
 *
 * Fill at most *wanted* samples per channel, and return the number of sample
 * per channel written into the *samples* output buffer.
 *
 * The first half of this function translates the planes in the frame's *data*
 * array, according to index. The second half passes the translated planes into
 * the resampler, which will write the converted samples into the output.
 */
static int convert_frame(struct SwrContext *converter, AVFrame *frame, int index, float *samples, int wanted)
{
	const uint8_t *data[frame->channels];
	int sample_size = av_get_bytes_per_sample(frame->format);
	if (av_sample_fmt_is_planar(frame->format)) {
		for (int c = 0; c < frame->channels; ++c)
			data[c] = frame->data[c] + index * sample_size;
	} else {
		data[0] = frame->data[0] + index * sample_size * frame->channels;
	}

	int left = frame->nb_samples - index;
	int consume = left < wanted ? left : wanted;
	int rc = swr_convert(converter, (uint8_t**) &samples, consume, data, consume);
	if (rc < 0) {
		oshu_log_error("audio sample conversion error");
		log_av_error(rc);
		return -1;
	}
	return rc;
}

int oshu_read_stream(struct oshu_stream *stream, float *samples, int nb_samples)
{
	int left = nb_samples;
	while (left > 0 && !stream->finished) {
		if (stream->sample_index >= stream->frame->nb_samples) {
			if (next_frame(stream) < 0)
				return -1;
			continue;
		}
		int rc = convert_frame(stream->converter, stream->frame, stream->sample_index, samples, left);
		if (rc < 0)
			return -1;
		left -= rc;
		stream->sample_index += rc;
		samples += rc * channels;
	}
	return nb_samples - left;
}

/**
 * Log some helpful information about the decoded audio stream.
 * Meant for debugging more than anything else.
 */
static void dump_stream_info(struct oshu_stream *stream)
{
	oshu_log_info("============ Audio information ============");
	oshu_log_info("            Codec: %s.", stream->codec->long_name);
	oshu_log_info("      Sample rate: %d Hz.", stream->decoder->sample_rate);
	oshu_log_info(" Average bit rate: %ld kbps.", stream->decoder->bit_rate / 1000);
	oshu_log_info("    Sample format: %s.", av_get_sample_fmt_name(stream->decoder->sample_fmt));
	oshu_log_info("         Duration: %d seconds.", (int) (stream->stream->duration * stream->time_base));
}

/**
 * Open the libavformat demuxer, and find the best stream stream.
 *
 * Fill #oshu_stream::demuxer, #oshu_stream::codec, #oshu_stream::stream and
 * #oshu_stream::time_base.
 *
 * \return 0 on success, -1 on error.
 */
static int open_demuxer(const char *url, struct oshu_stream *stream)
{
	int rc = avformat_open_input(&stream->demuxer, url, NULL, NULL);
	if (rc < 0) {
		oshu_log_error("failed opening the stream file");
		goto fail;
	}
	rc = avformat_find_stream_info(stream->demuxer, NULL);
	if (rc < 0) {
		oshu_log_error("error reading the stream headers");
		goto fail;
	}
	rc = av_find_best_stream(
		stream->demuxer,
		AVMEDIA_TYPE_AUDIO,
		-1, -1,
		&stream->codec,
		0
	);
	if (rc < 0 || stream->codec == NULL) {
		oshu_log_error("error finding the best stream stream");
		goto fail;
	}
	stream->stream = stream->demuxer->streams[rc];
	stream->time_base = av_q2d(stream->stream->time_base);
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
static int open_decoder(struct oshu_stream *stream)
{
	stream->decoder = avcodec_alloc_context3(stream->codec);
	int rc = avcodec_parameters_to_context(
		stream->decoder,
		stream->stream->codecpar
	);
	if (rc < 0) {
		oshu_log_error("error copying the codec context");
		goto fail;
	}
	rc = avcodec_open2(stream->decoder, stream->codec, NULL);
	if (rc < 0) {
		oshu_log_error("error opening the codec");
		goto fail;
	}
	if (!stream->decoder->channel_layout)
		stream->decoder->channel_layout = av_get_default_channel_layout(stream->decoder->channels);
	stream->frame = av_frame_alloc();
	if (stream->frame == NULL) {
		oshu_log_error("could not allocate the codec frame");
		goto fail;
	}
	return 0;
fail:
	log_av_error(rc);
	return -1;
}

/**
 * Initialize the libswresample converter to resample data from
 * #oshu_stream::decoder into a definied output format.
 *
 * The output sample rate must be defined in #oshu_stream::sample_rate.
 */
static int open_converter(struct oshu_stream *stream)
{
	assert (channels == 2);
	stream->converter = swr_alloc_set_opts(
		stream->converter,
		/* output */
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_FLT,
		stream->sample_rate,
		/* input */
		stream->decoder->channel_layout,
		stream->decoder->sample_fmt,
		stream->decoder->sample_rate,
		0, NULL
	);
	if (!stream->converter) {
		oshu_log_error("error allocating the audio resampler");
		return -1;
	}
	int rc = swr_init(stream->converter);
	if (rc < 0) {
		oshu_log_error("error initializing the audio resampler");
		log_av_error(rc);
		return -1;
	}
	return 0;
}

int oshu_open_stream(const char *url, struct oshu_stream *stream)
{
	av_register_all();
	if (open_demuxer(url, stream) < 0)
		goto fail;
	if (open_decoder(stream) < 0)
		goto fail;
	dump_stream_info(stream);
	stream->sample_rate = stream->decoder->sample_rate;
	if (open_converter(stream) < 0)
		goto fail;
	if (next_frame(stream) < 0)
		goto fail;
	return 0;
fail:
	oshu_close_stream(stream);
	return -1;
}

void oshu_close_stream(struct oshu_stream *stream)
{
	if (stream->frame)
		av_frame_free(&stream->frame);
	if (stream->decoder)
		avcodec_free_context(&stream->decoder);
	if (stream->demuxer)
		avformat_close_input(&stream->demuxer);
	if (stream->converter)
		swr_free(&stream->converter);
}
