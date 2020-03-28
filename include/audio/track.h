/**
 * \file audio/track.h
 * \ingroup audio_track
 */

#pragma once

namespace oshu {

struct sample;

/**
 * \defgroup audio_track Track
 * \ingroup audio
 *
 * \brief
 * Mix samples on top of an audio stream.
 *
 * A track lets you play a sample on top of another stream. While samples are
 * dumb data buffers, a track remembers its position, its volume, and is able
 * to loop the sample.
 *
 * Multiple tracks may share the same sample.
 *
 * \{
 */

/**
 * A track object for playing one sample.
 *
 * Tracks may only play packed stereo float samples.
 *
 * \sa oshu::start_track
 * \sa oshu::stop_track
 */
struct track {
	/**
	 * The sample object the track is going to play.
	 *
	 * A null pointer here means the track is inactive.
	 *
	 * This pointer is automatically reset to NULL when the sample is done
	 * playing.
	 */
	oshu::sample *sample;
	/**
	 * The position in the #sample buffer, in samples per channel.
	 *
	 * It is comprised between 0 and #oshu::sample::nb_samples, inclusive.
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
	 * #oshu::stop_track is called.
	 */
	int loop;
};

/**
 * Play a sample on a track.
 *
 * It is okay to call this function on an uninitialized track, or on a track
 * that is already playing something. Note that any previously playing sample
 * will stop playing.
 *
 * \sa oshu::track
 */
void start_track(oshu::track *track, oshu::sample *sample, float volume, int loop);

/**
 * Stop the playback on a track.
 *
 * It gets useful when a sample is looping, as it wouldn't stop by itself
 * otherwise.
 */
void stop_track(oshu::track *track);

/**
 * Mix a track on top of an audio stream.
 *
 * The track may be active or not.
 *
 * The *samples* buffer must contain at least 2 × *nb_samples* floats, already
 * filled with audio data. The samples of the track are added on top of it.
 *
 * The operation performed for mixing the samples is called multiply-accumulate
 * (MAC) and when dealing with audio, there are specialized hardware units to
 * handle it. The *fmaf* function in math.h should be able to perform the
 * operation in a single step, with less rounding error.
 *
 * \return
 * The number of samples per channel that were added to the buffer. It may be 0
 * when the stream is inactive, or less than *nb_samples* when the sample has
 * reached an end.
 */
int mix_track(oshu::track *track, float *samples, int nb_samples);

/** \} */

}
