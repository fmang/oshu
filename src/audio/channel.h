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
 * A channel lets you play a sample on top of another stream. While samples are
 * dumb data buffers, a channel remembers its position, its volume, and is able
 * to loop the sample.
 *
 * Multiple channels may share the same sample.
 *
 * \{
 */

/**
 * A channel object for playing one sample.
 *
 * \sa oshu_start_channel
 * \sa oshu_stop_channel
 */
struct oshu_channel {
	/**
	 * The sample object the channel is going to play.
	 *
	 * A null pointer here means the channel is inactive.
	 *
	 * This pointer is automatically reset to NULL when the sample is done
	 * playing.
	 */
	struct oshu_sample *sample;
	/**
	 * The position in the #sample buffer, in samples per channel.
	 *
	 * It is comprised between 0 and #oshu_sample::nb_samples, inclusive.
	 */
	int cursor;
	/**
	 * The volume to apply when mixing the sample into another stream.
	 *
	 * It ranges from 0 to 1.
	 */
	float volume;
	/**
	 * If true, the sample is looped over and over until
	 * #oshu_reset_channel is called.
	 */
	int loop;
};

/**
 * Play a sample on a channel.
 *
 * It is okay to call this function on an unitialized channel, or on a channel
 * that is already playing something. Note that any previously playing sample
 * will stop playing.
 *
 * \sa oshu_channel
 */
void oshu_start_channel(struct oshu_channel *channel, struct oshu_sample *sample, float volume);

/**
 * Stop the playback on a channel.
 *
 * It gets useful when a sample is looping, as it wouldn't stop by itself
 * otherwise.
 */
void oshu_stop_channel(struct oshu_channel *channel);

/**
 * Mix a channel on top of an audio stream.
 *
 * The channel may be active or not.
 *
 * The *samples* buffer must contain at least 2 × *nb_samples* floats, already
 * filled with audio data. The samples of the channel are added on top of it.
 *
 * \return
 * The number of samples per channel that were added to the buffer. It may be 0
 * when the stream is inactive, or less than *nb_samples* when the sample has
 * reached an end.
 */
int oshu_mix_channel(struct oshu_channel *channel, float *samples, int nb_samples);

/** \} */
