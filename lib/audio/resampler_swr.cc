/**
 * \file src/audio/resampler_swr.cc
 * \ingroup audio_resampler
 */

#include "audio/resampler.h"
#include "core/log.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <iostream>
#include <stdexcept>

/**
 * Spew an error message according to the return value of a call to one of
 * ffmpeg's functions.
 *
 * \todo
 * Factor with the other modules.
 */
static void log_av_error(int rc)
{
	char errbuf[256];
	av_strerror(rc, errbuf, sizeof(errbuf));
	oshu_log_error("ffmpeg error: %s", errbuf);
}

namespace oshu {
namespace audio {

/**
 * Initialize the libswresample converter to resample data from
 * #oshu_stream::decoder into a definied output format.
 *
 * The output sample rate must be defined in #oshu_stream::sample_rate.
 */
resampler::resampler(struct AVCodecContext *input, int output_sample_rate)
{
	oshu::log::debug() << "using libswresample" << std::endl;
	swr = swr_alloc_set_opts(
		nullptr,
		/* output */
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_FLT,
		output_sample_rate,
		/* input */
		input->channel_layout,
		input->sample_fmt,
		input->sample_rate,
		0, NULL
	);
	if (!swr) {
		oshu_log_error("error allocating the audio resampler");
		throw std::runtime_error("libswresample allocation error");
	}
	int rc = swr_init(swr);
	if (rc < 0) {
		oshu_log_error("error initializing the audio resampler");
		log_av_error(rc);
		throw std::runtime_error("libswresample initialization error");
	}
}

resampler::~resampler()
{
	if (swr)
		swr_free(&swr);
}

void resampler::convert(uint8_t **out, int out_count, const uint8_t **in, int in_count)
{
	int rc = swr_convert(swr, out, out_count, in, in_count);
	if (rc < 0) {
		oshu_log_error("audio sample conversion error");
		log_av_error(rc);
		throw std::runtime_error("libswresample conversion error");
	}
}

}}
