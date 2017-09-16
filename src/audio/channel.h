/**
 * \file audio/channel.h
 * \ingroup audio_channel
 */

#pragma once

struct oshu_sample;

/**
 * \defgroup audio_channel Channel
 * \ingroup audio
 *
 * \{
 */

struct oshu_channel {
	struct oshu_sample *sample;
	int cursor;
	float volume;
	int loop;
};

void oshu_play_channel(struct oshu_channel *channel, struct oshu_sample *sample, float volume);

void oshu_reset_channel(struct oshu_channel *channel);

int oshu_mix_channel(struct oshu_channel *channel, float *samples, int nb_samples);

/** \} */
