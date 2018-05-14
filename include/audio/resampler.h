/**
 * \file include/audio/resampler.h
 * \ingroup audio_resampler
 */

#pragma once

#include <cstdint>

struct AVAudioResampleContext;
struct AVCodecContext;
struct SwrContext;

namespace oshu {
namespace audio {

/**
 * \defgroup audio_resampler Resampler
 * \ingroup audio
 *
 * The resampling process converts audio samples from one format to another.
 *
 * For simplicity, this module is specialized for use with libavcodec, and
 * restricted to outputting stereo 32-bit floating samples.
 *
 * Its main reason for being is to expose a common interface between
 * libswresample and libavresample. Both have a common feature set but the
 * functions are named differently. CMake will pick one implementation or the
 * other depending on the library it found. ffmpeg supports both, but
 * deprecates libavresample, while libav only supports libavresample.
 *
 * More details on libswresample vs. libavresample at
 * https://lists.ffmpeg.org/pipermail/ffmpeg-devel/2012-April/123746.html
 *
 * \{
 */

class resampler {

public:
	resampler(struct AVCodecContext *input, int output_sample_rate);
	~resampler();
	/**
	 * Main conversion function, wrapping `swr_convert`.
	 *
	 * \param out output buffers, only the first one need be set in case of packed audio
	 * \param out_count amount of space available for output in samples per channel
	 * \param in input buffers, only the first one need to be set in case of packed audio
	 * \param in_count number of input samples available in one channel
	 *
	 * \return number of samples output per channel
	 *
	 * On error, an runtime error is raised.
	 */
	int convert(uint8_t **out, int out_count, uint8_t **in, int in_count);

private:
	/**
	 * libswresample context.
	 *
	 * Only one of #swr or #avr may be not null.
	 */
	struct SwrContext *swr = nullptr;
	/**
	 * libavresample context.
	 *
	 * Conflicts with #avr.
	 */
	struct AVAudioResampleContext *avr = nullptr;

};

/** \} */

}}
