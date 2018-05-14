/**
 * \file audio/stream.h
 * \ingroup audio_stream
 */

#pragma once

#include "audio/resampler.h"

#include <memory>

/**
 * \defgroup audio_stream Stream
 * \ingroup audio
 *
 * \brief
 * Read an audio file progressively.
 *
 * This module provides a high-level interface for reading an audio stream
 * using ffmpeg.
 *
 * Any file supported by ffmpeg will be read, but to make things simpler, the
 * audio is systematically transcoded to a uniform output format:
 *
 * - Stereo output: 2 channels.
 * - The sample rate is the same as the input stream. Usually 44.1 KHz or 48 KHz.
 * - The sample format is 32-bit float, also named FLT or F32 for SDL.
 * - The samples are packed, meaning they're interlaced like LRLRLR, as opposed to planar LLLRRR.
 *
 * The decoding process is a combination of demuxing using libavformat, then
 * decoding using libavcodec, and finally converting the samples using
 * libswresample. The pipeline is illustrated below.
 *
 * \dot
 * digraph {
 * 	rankdir=LR
 * 	node [shape=rect]
 * 	AVFormatContext -> AVCodecContext [label=AVPacket]
 * 	AVCodecContext -> SwrContext [label=AVFrame]
 * 	SwrContext -> "float*"
 * 	"float*" [shape=none]
 * }
 * \enddot
 *
 * \{
 */

/*
 * Forward declaration of the ffmpeg structures to avoid including big headers.
 */
struct AVFormatContext;
struct AVCodec;
struct AVStream;
struct AVCodecContext;
struct AVFrame;

/**
 * An audio stream, from its demuxer and decoder to its output device.
 *
 * The only properties you'd want to use outside this sub-modules are
 * #sample_rate when setting up the audio output device, and the
 * #current_timestamp to know the current stream position.
 */
struct oshu_stream {
	/**
	 * The libavformat demuxer, handling the I/O aspects.
	 */
	struct AVFormatContext *demuxer;
	/**
	 * Pointer to the best audio stream we've found in the media file, from
	 * the demuxer's point of view.
	 *
	 * It most notably contains temporal information, like the time base
	 * and the duration of the stream.
	 *
	 * Its memory is managed by the #demuxer context.
	 */
	struct AVStream *stream;
	/**
	 * The codec for the #stream.
	 *
	 * It's a generic object managed by libavcodec, and will therefore only
	 * tell us the name of the codec, along with other metadata. It is
	 * stored here for convenience when opening the stream. Do not free it.
	 */
	struct AVCodec *codec;
	/**
	 * The #decoder context, initialized from the #codec with the
	 * parameters defined in the #stream.
	 *
	 * It is the most useful source of information concerning the sample
	 * format. It defines the sample rate, type, the channel layout.
	 */
	struct AVCodecContext *decoder;
	/**
	 * A frame, containing a dozen milliseconds worth of audio data.
	 *
	 * It's allocated and freed by us once, and re-used every time a new
	 * frame is read.
	 */
	struct AVFrame *frame;
	/**
	 * The audio resampler that converts whatever audio data format #frame
	 * contains into the packed stereo 32-bit float samples we use for our
	 * output.
	 */
	std::unique_ptr<oshu::audio::resampler> converter;
	/**
	 * The sample rate of the output stream when read with
	 * #oshu_read_stream.
	 *
	 * It is set by the #converter, and in practice will always be the same
	 * as the sample rate of the #decoder.
	 */
	int sample_rate;
	/**
	 * The factor by which we must multiply ffmpeg timestamps to obtain
	 * seconds. Because it won't change for a given stream, compute it
	 * once and make it easily accessible.
	 *
	 * Its correct value is the one taken from the demuxer's #stream
	 * object, and not the codec's.
	 */
	double time_base;
	/**
	 * The duration of the stream in seconds.
	 */
	double duration;
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
	/**
	 * How many samples per channel of the current #frame we've read using
	 * #oshu_read_stream. When this number is bigger than the number of
	 * samples per channel in the frame, we must request a new frame.
	 */
	int sample_index;
	/**
	 * True when the end of the stream is reached.
	 *
	 * Set to 1 by #oshu_read_stream when we receive an AVERROR_EOF code
	 * while trying to read a frame.
	 */
	int finished;
};

/**
 * Open an audio stream.
 *
 * \param url Path or URL to the media you want to play.
 * \param stream A null-initialized stream object.
 *
 * \sa oshu_close_stream
 */
int oshu_open_stream(const char *url, struct oshu_stream *stream);

/**
 * Read *nb_samples* float samples from an audio stream.
 *
 * Samples are always float, because float samples are practical for mixing.
 *
 * The *nb_samples* parameter specifies how many samples per channel to read.
 *
 * The *samples* output buffer size must be at least `nb_samples * 2 *
 * sizeof(float)` bytes, because we're forcing stereo.
 *
 * \return
 * The number of samples read per channel. If it is less than *nb_samples*, it
 * means the end of the stream was reached, and further calls to this function
 * will return 0. On error, return -1.
 *
 * \sa oshu_stream::finished
 */
int oshu_read_stream(struct oshu_stream *stream, float *samples, int nb_samples);

/**
 * Seek the stream to the specifed target position in seconds.
 *
 * If the target is negative, the stream is rewinded to its beginning.
 *
 * To determine the new position of the stream after seeking, use
 * #oshu_stream::current_timestamp.
 *
 * You should probably use #oshu_seek_music instead.
 *
 * \todo
 * There's often some kind of audio distortion glitch right after seeking.
 *
 */
int oshu_seek_stream(struct oshu_stream *stream, double target);

/**
 * Close an audio stream, and free everything we can.
 */
void oshu_close_stream(struct oshu_stream *stream);

/** \} */
