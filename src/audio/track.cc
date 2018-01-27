/**
 * \file audio/track.cc
 * \ingroup audio_track
 */

#include "audio/sample.h"
#include "audio/track.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

/** Work in stereo. */
static const int channels = 2;

void oshu_start_track(struct oshu_track *track, struct oshu_sample *sample, float volume, int loop)
{
	if (sample && sample->nb_samples == 0)
		sample = NULL;
	track->sample = sample;
	track->cursor = 0;
	track->volume = volume;
	track->loop = loop;
}

void oshu_stop_track(struct oshu_track *track)
{
	track->sample = NULL;
}

int oshu_mix_track(struct oshu_track *track, float *samples, int nb_samples)
{
	int wanted = nb_samples;
	while (wanted > 0 && track->sample) {
		int left = track->sample->nb_samples - track->cursor;
		if (left == 0) {
			if (track->loop) {
				assert (track->sample->nb_samples > 0);
				track->cursor = 0;
			} else {
				track->sample = NULL;
			}
			continue;
		}
		int consume = left < wanted ? left : wanted;
		float *input = track->sample->samples + track->cursor * channels;
		for (int i = 0; i < consume * channels; i++)
			samples[i] = fmaf(input[i], track->volume, samples[i]);
		track->cursor += consume;
		samples += consume * channels;
		wanted -= consume;
	}
	return nb_samples - wanted;
}
