/**
 * \file audio.h
 * \ingroup audio
 *
 * \brief
 * Definition of the highest-level audio module.
 */

#pragma once

#include "audio/sample.h"
#include "audio/stream.h"
#include "audio/track.h"

#include <SDL2/SDL.h>

/** \defgroup audio Audio
 *
 * \brief
 * Play music and sound effects.
 *
 * Using this module, you should never need to call SDL's audio routines or
 * ffmpeg's directly.
 *
 * Given this level of control, we aim to synchronize as well as we can to the
 * audio playback. This is a rhythm game after all. However, determining what
 * part of the audio the sound card is playing is actually a tough task,
 * because there are many buffers in the audio chain, and each step adds some
 * lag. If you take only the SDL buffer, this lag is in the order of 10
 * milliseconds, which is already about 1 image frame in 60 FPS! However, we
 * should at least guarantee the game won't drift away from the audio playback,
 * even if for some reason the audio thread fails to decode a frame on time.
 *
 * ## The audio pipeline
 *
 * The playback is handled as follows:
 *
 * 1. SDL requests for audio samples by calling the callback function
 *    mentionned on device initialization.
 * 2. The callback function copies data from the current frame into SDL's
 *    supplied buffer, and requests more frames until the buffer is full.
 * 3. Frames are read from libavcodec, which keeps returning frames until the
 *    current page is completely read. Request a new page is the current one is
 *    completely consumed.
 * 4. Packets are read from libavformat, which reads data from the audio file,
 *    and returns pages until *EOF*. The packets are then passed to libavcodec
 *    for decoding.
 *
 * If this is not clear, here a few elements of structure and ffmpeg
 * terminology you should know:
 *
 * - A file contains *streams*. Audio, video, subtitles are all streams.
 * - Each stream is split into *packets*.
 * - Files are serialized by interleaving packets in chronological order.
 * - Each packet is decoded into one or more *frames*.
 * - A frame, in the case of audio, is a buffer of decoded *samples*.
 * - Samples are organized into *channels*. 2 channels for stereo.
 *
 * The *presentation timestamp* (PTS) is the time a frame should be displayed.
 * It's the job of the container format to maintain it, and is printed into
 * every packet. ffmpeg insert a more precise timestamp into every frame called
 * the *best effort timestamp*. This is what we'll use.
 *
 * Once a frame is decoded, and at the time we know its PTS, its samples are
 * sent into SDL's audio samples buffer, which are relayed to the sound card.
 * The elapsed time between the frame decoding and its actual playback cannot
 * easily be determined, so we'll get a consistent lag, sadly. This consistent
 * lag can be rectified with a voluntary bias on the computed position. Note
 * however than the sound effects we'll trigger will suffer from that exact
 * same bias.
 *
 * ## Mixing
 *
 * To play samples on top of the background music, the work is split between
 * the various audio sub-modules.
 *
 * Their relations are shown below.
 *
 * \dot
 * digraph {
 * 	rankdir=LR
 * 	node [shape=rect]
 *
 * 	track1 [label="oshu_track"]
 * 	track2 [label="oshu_track"]
 * 	track3 [label="oshu_track"]
 * 	sample1 [label="oshu_sample"]
 * 	sample2 [label="oshu_sample"]
 * 	"float*" [shape=none]
 * 	"music.mp3" [shape=none]
 * 	"hit.wav" [shape=none]
 * 	"clap.wav" [shape=none]
 *
 * 	"music.mp3" -> "oshu_stream" [label="oshu_open_stream"]
 * 	"oshu_stream" -> "float*" [label="oshu_read_stream"]
 * 	"hit.wav" -> sample1 [label="oshu_load_sample"]
 * 	"clap.wav" -> sample2 [label="oshu_load_sample"]
 * 	sample1 -> track1 [label="oshu_start_track"]
 * 	sample1 -> track2 [label="oshu_start_track"]
 * 	sample2 -> track3 [label="oshu_start_track"]
 * 	track1 -> "float*" [label="oshu_mix_track"]
 * 	track2 -> "float*" [label="oshu_mix_track"]
 * 	track3 -> "float*" [label="oshu_mix_track"]
 * 	"float*" -> "SDL"
 * }
 * \enddot
 *
 * ## Use
 *
 * To use this module, open streams using #oshu_open_audio, and play them using
 * #oshu_play_audio. When you're done, close your streams with
 * #oshu_pause_audio. Also make sure you initialized SDL with the audio
 * sub-module.
 *
 * ```c
 * SDL_Init(SDL_AUDIO|...);
 * struct oshu_audio audio;
 * memset(&audio, 0, sizeof(audio));
 * oshu_open_audio("file.ogg", &audio);
 * oshu_play_audio(&audio);
 * do_things();
 * oshu_close_audio(&audio);
 * ```
 *
 * ## Limitations
 *
 * To make the implementation simpler, samples are always converted to packed
 * stetero 32-bit floats. See \ref audio_stream. The float part is not a
 * limitation in itself because it's one of the most accurate format commonly
 * available. The packed part is no different from planar in terms of
 * information since it's just about ordering.
 *
 * The main limitation is the stereo mode, and fortunately, it's probably also
 * the easiest one to lift. However, I cannot see any use case, since players
 * have stereo headphones, and most songs are in stereo in the first place.
 * The number of channels is not enough to properly define a channel layout
 * though, because there are several ways to arrange them. It doesn't look like
 * SDL2 has been thinking it out through tough, so let's wait until someone
 * comes up with a concrete case, and test a few things when that time comes.
 *
 * \{
 */

/**
 * The full audio pipeline.
 *
 * This structure is mainly accessed through an audio thread, and should be
 * locked using SDL's `SDL_LockAudioDevice` and `SDL_UnlockAudioDevice`
 * procedures in order to be accessed peacefully from another thread. You don't
 * need to bother with locking when using the accessors defined in this module
 * though. Only lock when accessing multiple fields directly.
 */
struct oshu_audio {
	/**
	 * The background music.
	 */
	struct oshu_stream music;
	/**
	 * Tracks for playing sound effects on top of the music.
	 *
	 * Up to 8 samples may be played at a time.
	 *
	 * When the need for looping samples arise, it would be smart to
	 * dedicate a track for them.
	 *
	 * \sa oshu_play_sample
	 */
	struct oshu_track effects[16];
	/**
	 * Special track for the looping sample.
	 *
	 * As you can see, only one loop is supported.
	 *
	 * \sa oshu_play_loop
	 */
	struct oshu_track looping;
	/**
	 * A device ID returned by SDL, and required by most SDL audio
	 * functions.
	 */
	SDL_AudioDeviceID device_id;
	/**
	 * Contains the sample rate, format, and channel layout of the audio
	 * output device.
	 *
	 * This is useful for loading samples with #oshu_load_sample.
	 */
	SDL_AudioSpec device_spec;
};

/**
 * Open a file and initialize everything needed to play it.
 *
 * \param url Path or URL to the audio file to play.
 * \param audio Null-initialized #oshu_audio object.
 *
 * \return 0 on success. On error, -1 is returned and everything is freed.
 *
 * \sa oshu_audio_close
 */
int oshu_open_audio(const char *url, struct oshu_audio *audio);

/**
 * Start playing!
 *
 * The SDL plays audio in a separate thread, so you need not worry about
 * calling this function regularily or anything. Don't bother, it's magic!
 *
 * \sa oshu_pause_audio
 */
void oshu_play_audio(struct oshu_audio *audio);

/**
 * Pause the stream.
 *
 * Calling #oshu_play_audio will resume the audio playback where it was
 * left playing.
 */
void oshu_pause_audio(struct oshu_audio *audio);

/**
 * Play a sample on top of the background music.
 *
 * Multiple samples may be played at once, but there's still a limit to the
 * number of samples that can be played simultaneously. When that number is
 * reached because all the effects tracks are used, the playback of one of
 * the samples is stopped to play the new sample.
 */
void oshu_play_sample(struct oshu_audio *audio, struct oshu_sample *sample, float volume);

/**
 * Play a looping sample.
 *
 * Currently, only one looping sample may be played at a time. Calling this
 * function cancels any other looping sample.
 *
 * \sa oshu_play_sample
 * \sa oshu_stop_loop
 */
void oshu_play_loop(struct oshu_audio *audio, struct oshu_sample *sample, float volume);

/**
 * Seek the music stream to the specifed target position in seconds.
 *
 * See #oshu_seek_stream for details.
 *
 * It is similar to #oshu_seek_stream, with the different that this function
 * locks the audio thread, and stops all the currently playing sound effects,
 * which is definitely what you want.
 */
int oshu_seek_music(struct oshu_audio *audio, double target);

/**
 * Stop the looping sample.
 *
 * Since only one sample may be played at a time, it's not ambiguous. In the
 * future, if more looping samples are supported, this function's prototype
 * would change.
 *
 * \sa oshu_play_loop
 */
void oshu_stop_loop(struct oshu_audio *audio);

/**
 * Close the audio stream and free everything associated to it.
 */
void oshu_close_audio(struct oshu_audio *audio);

/** \} */
