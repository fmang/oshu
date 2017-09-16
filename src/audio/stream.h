/**
 * \file audio/stream.h
 * \ingroup audio_stream
 *
 * \brief
 * Read an audio file progressively.
 */

#pragma once

#include <libavformat/avformat.h>

/**
 * \defgroup audio_stream
 * \ingroup audio
 *
 * \{
 */

/**
 * An audio stream, from its demuxer and decoder to its output device.
 */
struct oshu_stream {
	AVFormatContext *demuxer;
	AVCodec *codec;
	AVStream *stream;
	AVCodecContext *decoder;
	AVPacket packet;
	AVFrame *frame;
	/**
	 * The factor by which we must multiply ffmpeg timestamps to obtain
	 * seconds. Because it won't change for a given stream, compute it
	 * once and make it easily accessible.
	 */
	double time_base;
};

/**
 * Open an audio stream.
 *
 * \param stream A null-initialized stream object.
 */
int oshu_open_stream(const char *url, struct oshu_stream *stream);

/**
 * Read the next frame from a stream into #oshu_stream::frame.
 *
 * \return `AVERROR_EOF` on end of file, 0 on success, -1 on failure.
 */
int oshu_next_frame(struct oshu_stream *steam);

/**
 * Close an audio stream.
 */
void oshu_close_stream(struct oshu_stream *stream);

/** \} */
