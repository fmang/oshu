/**
 * \file audio/stream.cc
 * \ingroup audio_stream
 */

#include "audio/stream.h"
#include "core/log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

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
static int convert_frame(oshu_stream *stream, AVFrame *frame, int index, float *samples, int wanted)
{
	uint8_t *data[frame->channels];
	int sample_size = av_get_bytes_per_sample((AVSampleFormat) frame->format);
	if (av_sample_fmt_is_planar((AVSampleFormat) frame->format)) {
		for (int c = 0; c < frame->channels; ++c)
			data[c] = frame->data[c] + index * sample_size;
	} else {
		data[0] = frame->data[0] + index * sample_size * frame->channels;
	}

	int left = frame->nb_samples - index;
	int consume = left < wanted ? left : wanted;
	try {
		return stream->converter->convert((uint8_t**) &samples, consume, data, consume);
	} catch (std::runtime_error &e) {
		return -1;
	}
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
		int rc = convert_frame(stream, stream->frame, stream->sample_index, samples, left);
		if (rc < 0)
			return -1;
		left -= rc;
		stream->sample_index += rc;
		stream->current_timestamp += (double) rc / stream->decoder->sample_rate;
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
	oshu_log_info("         Duration: %0.3f", stream->duration);
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
	stream->duration = stream->time_base * stream->stream->duration;
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

int oshu_open_stream(const char *url, struct oshu_stream *stream)
{
	av_register_all();
	if (open_demuxer(url, stream) < 0)
		goto fail;
	if (open_decoder(stream) < 0)
		goto fail;
	dump_stream_info(stream);
	stream->sample_rate = stream->decoder->sample_rate;
	try {
		stream->converter = std::make_unique<oshu::audio::resampler>(stream->decoder, stream->sample_rate);
	} catch (std::runtime_error &e) {
		goto fail;
	}
	if (next_frame(stream) < 0)
		goto fail;
	return 0;
fail:
	oshu_close_stream(stream);
	return -1;
}

void oshu_close_stream(struct oshu_stream *stream)
{
	/* the av routines set the pointers to NULL */
	if (stream->frame)
		av_frame_free(&stream->frame);
	if (stream->decoder)
		avcodec_free_context(&stream->decoder);
	if (stream->demuxer)
		avformat_close_input(&stream->demuxer);
	stream->converter.reset(nullptr);
}

int oshu_seek_stream(struct oshu_stream *stream, double target)
{
	if (target < 0.) {
		target = 0.;
	} else if (target >= stream->duration) {
		oshu_log_warning("cannot seek past the end of the stream");
		return -1;
	}
	int rc = av_seek_frame(
		stream->demuxer,
		stream->stream->index,
		target / stream->time_base,
		target < stream->current_timestamp ? AVSEEK_FLAG_BACKWARD : 0
	);
	if (rc < 0) {
		log_av_error(rc);
		return -1;
	}
	stream->current_timestamp = target;
	/* Flush the buffers. */
	next_page(stream);
	next_frame(stream);
	return 0;
}
