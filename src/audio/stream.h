/**
 * \file audio/stream.h
 * \ingroup audio_stream
 *
 * \brief
 * Read an audio file progressively.
 */

#pragma once

/**
 * \defgroup audio_stream
 * \ingroup audio
 *
 * \{
 */

struct AVFormatContext;
struct AVCodec;
struct AVStream;
struct AVCodecContext;
struct AVFrame;

/**
 * An audio stream, from its demuxer and decoder to its output device.
 */
struct oshu_stream {
	struct AVFormatContext *demuxer;
	struct AVCodec *codec;
	struct AVStream *stream;
	struct AVCodecContext *decoder;
	struct AVFrame *frame;
	struct SwrContext *converter;
	int sample_rate;
	/**
	 * The factor by which we must multiply ffmpeg timestamps to obtain
	 * seconds. Because it won't change for a given stream, compute it
	 * once and make it easily accessible.
	 */
	double time_base;
	/**
	 * The current temporal position in the playing stream, expressed in
	 * floating seconds.
	 *
	 * Sometimes the best-effort timestamp computed from a frame is
	 * erroneous. Rather than break everything, let's try to rely on the
	 * previous frame's timestamp, and therefore keep the timestamp in our
	 * structure, rather than using only ffmpeg's AVFrame.
	 *
	 * In order to do that, the frame decoding routine will update the
	 * #current_timestamp field whenever it reads a frame with a reasonable
	 * timestamp. It is computed from libavcodec's `best_effort_timestamp`,
	 * which means it was be multiplied by the time base in order to get a
	 * duration in seconds.
	 */
	double current_timestamp;
	int sample_index;
	int finished;
};

/**
 * Open an audio stream.
 *
 * \param stream A null-initialized stream object.
 */
int oshu_open_stream(const char *url, struct oshu_stream *stream);

/**
 * Read *nb_samples* from an audio stream.
 *
 * Samples are always float, because float samples are practical for mixing.
 *
 * The *nb_samples* parameter specifies how many samples per channel to read.
 *
 * The *samples* output buffer size must be at least `nb_samples * 2 *
 * sizeof(float)` bytes, because right now we're forcing stereo.
 *
 * \return
 * The number of samples read per channel. If it is less than *nb_samples*, it
 * means the end of the stream was reached, and further calls to this function
 * will return 0. On error, return -1.
 */
int oshu_read_stream(struct oshu_stream *stream, float *samples, int nb_samples);

/**
 * Close an audio stream.
 */
void oshu_close_stream(struct oshu_stream *stream);

/** \} */
