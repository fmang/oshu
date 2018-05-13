/**
 * \file include/audio/resampler.h
 * \ingroup audio_resampler
 */

#pragma once

struct AVCodecContext;

namespace oshu {
namespace audio {

/**
 * The resample converts audio samples from one format to another.
 *
 * For simplicity, this one is specialized for use with libavcodec, and
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
 */
class resampler {

public:
	resampler(struct AVCodecContext *input, int output_sample_rate);
	~resampler();
	void convert(uint8_t *out, uint8_t out_count, uint8_t *in, uint8_t in_count);

}

}}
