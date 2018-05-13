/**
 * \file src/audio/resampler_avr.cc
 * \ingroup audio_resampler
 *
 * \brief
 * libavresample backend for the resampler.
 */

#include "audio/resampler.h"
#include "core/log.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavresample/avresample.h>
#include <libavutil/opt.h>
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

resampler::resampler(struct AVCodecContext *input, int output_sample_rate)
{
	oshu::log::debug() << "using libavresample" << std::endl;
	avr = avresample_alloc_context();
	if (!avr) {
		oshu_log_error("error allocating the audio resampler");
		throw std::runtime_error("libavresample allocation error");
	}
	av_opt_set_int(avr, "out_channel_layout", AV_CH_LAYOUT_STEREO,   0);
	av_opt_set_int(avr, "out_sample_fmt",     AV_SAMPLE_FMT_FLT,     0);
	av_opt_set_int(avr, "out_sample_rate",    output_sample_rate,    0);
	av_opt_set_int(avr, "in_channel_layout",  input->channel_layout, 0);
	av_opt_set_int(avr, "in_sample_fmt",      input->sample_fmt,     0);
	av_opt_set_int(avr, "in_sample_rate",     input->sample_rate,    0);
	int rc = avresample_open(avr);
	if (rc < 0) {
		oshu_log_error("error initializing the audio resampler");
		log_av_error(rc);
		throw std::runtime_error("libavresample initialization error");
	}
}

resampler::~resampler()
{
	if (avr)
		avresample_free(&avr);
}

/**
 * \todo
 * The 3rd a 6th arguments are the plane sizes, and may be used to optimize the
 * process. However, what is this number exactly, and how do we know it?
 */
int resampler::convert(uint8_t **out, int out_count, uint8_t **in, int in_count)
{
	int rc = avresample_convert(avr, out, 0, out_count, in, 0, in_count);
	if (rc < 0) {
		oshu_log_error("audio sample conversion error");
		log_av_error(rc);
		throw std::runtime_error("libavresample conversion error");
	} else {
		return rc;
	}
}

}}
