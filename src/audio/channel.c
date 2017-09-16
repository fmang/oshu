/**
 * \file audio/channel.c
 * \ingroup audio_channel
 */

#include "audio/channel.h"
#include "audio/sample.h"

#include <stdlib.h>

void oshu_play_channel(struct oshu_channel *channel, struct oshu_sample *sample, float volume)
{
	channel->sample = sample;
	channel->cursor = 0;
	channel->volume = volume;
	channel->loop = 0;
}

void oshu_reset_channel(struct oshu_channel *channel)
{
	channel->sample = NULL;
}

int oshu_mix_channel(struct oshu_channel *channel, float *samples, int nb_samples)
{
	int wanted = nb_samples;
	while (wanted > 0 && channel->sample) {
		int left = channel->sample->nb_samples - channel->cursor;
		if (left == 0) {
			if (channel->loop)
				channel->cursor = 0;
			else
				channel->sample = NULL;
			continue;
		}
		int consume = left < wanted ? left : wanted;
		float *input = channel->sample->samples + channel->cursor * 2;
		for (int i = 0; i < consume * 2; i++)
			samples[i] += input[i] * channel->volume;
		samples += consume * 2;
	}
	return nb_samples - wanted;
}
