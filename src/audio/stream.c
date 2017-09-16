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
 * This function is meant to be called exclusively from #next_frame,
 * because a single page may yield many codec frames.
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
			break;
		}
	}
	av_packet_unref(&packet);
	if (rc < 0) {
		log_av_error(rc);
		return -1;
	}
	return 0;
}

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
				oshu_log_warn("abrupt end of stream");
				return -1;
			}
		} else if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last frame");
			stream->finished = 1;
			return 0;
		} else {
			log_av_error(rc);
			return -1;
		}
	}
}

int oshu_read_stream(struct oshu_stream *stream, float *samples, int nb_samples)
{
	int produced = 0;
	int wanted = nb_samples;
	while (wanted > 0 && !stream->finished) {
		int left = stream->frame->nb_samples - stream->sample_index;
		if (left == 0) {
			if (next_frame(stream) < 0)
				return -1;
			continue;
		}
		const uint8_t *data[stream->frame->channels];
		if (av_sample_fmt_is_planar(stream->frame->format)) {
			for (int c = 0; c < stream->frame->channels; ++c)
				data[c] = stream->frame->data[c] + stream->sample_index * av_get_bytes_per_sample(stream->frame->format);
		} else {
			data[0] = stream->frame->data[0] + stream->sample_index * av_get_bytes_per_sample(stream->frame->format) * stream->frame->channels;
		}
		int consume = left < wanted ? left : wanted;
		int rc = swr_convert(
			stream->converter,
			(uint8_t**) &samples, consume,
			data, consume
		);
		if (rc < 0) {
			oshu_log_error("audio sample conversion error");
			return -1;
		}
		assert (rc == consume);
		produced += consume;
		wanted -= consume;
		stream->sample_index += consume;
		samples += consume * 2;
	}
	return produced;
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

static int open_converter(struct oshu_stream *stream)
{
	stream->converter = swr_alloc();
	if (!stream->converter) {
		oshu_log_error("error allocating the audio resampler");
		return -1;
	}

	av_opt_set_int(stream->converter, "in_channel_layout", stream->decoder->channel_layout, 0);
	av_opt_set_int(stream->converter, "in_sample_rate", stream->decoder->sample_rate, 0);
	av_opt_set_sample_fmt(stream->converter, "in_sample_fmt", stream->decoder->sample_fmt, 0);

	av_opt_set_int(stream->converter, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(stream->converter, "out_sample_rate", stream->sample_rate, 0);
	av_opt_set_sample_fmt(stream->converter, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

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
