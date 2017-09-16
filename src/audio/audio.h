/**
 * \file audio.h
 * \ingroup audio
 *
 * \brief
 * Definition of the audio module and its submodules.
 */

#pragma once

#include "audio/stream.h"

#include <SDL2/SDL.h>

/** \defgroup audio Audio
 *
 * \brief
 * Open and decode any kind of audio file using ffmpeg. libavformat does the
 * demuxing, libavcodec decodes. Then feed the decoded samples to SDL's audio
 * device.
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
 * To use this module, first call #oshu_audio_init. Then open streams using
 * #oshu_audio_open, and play them using #oshu_audio_play. When you're
 * done, close your streams with #oshu_audio_close. Also make sure you
 * initialized SDL with the audio submodule.
 *
 * ```c
 * SDL_Init(SDL_AUDIO|...);
 * oshu_audio_init();
 * struct oshu_audio *audio;
 * oshu_audio_open("file.ogg", &audio);
 * oshu_audio_play(audio);
 * do_things();
 * oshu_audio_close(audio);
 * ```
 *
 * Depending on how the SDL handles multiple devices, you may not be able to
 * open simultaneous streams.
 *
 * This module was carefully built and should be relied upon. Its main
 * limitations are that it cannot play sample formats not supported by SDL,
 * like 64-bit integer samples that nobody use, and that it cannot play more
 * than one sample on top of the main audio stream.
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
 * though. Only lock when accessing the multiplie fields directly.
 *
 * The only two fields you'll want to use outside of this module are
 * #current_timestamp and #finished.
 */
struct oshu_audio {
	struct oshu_stream music;
	SDL_AudioDeviceID device_id;
	SDL_AudioSpec device_spec;
	/** Sound sample to play on top of the audio stream.
	 *
	 *  Its memory space is not managed by this structure though, make sure
	 *  you free it yourself using #oshu_sample_free. */
	struct oshu_sample *overlay;
};

/**
 * Open a file and initialize everything needed to play it.
 *
 * The stream can then be closed and freed with #oshu_audio_close.
 *
 * \param url Path or URL to the audio file to play.
 * \param audio Will receive a newly allocated audio context.
 *
 * \return 0 on success. On error, -1 is returned and everything is freed.
 */
int oshu_audio_open(const char *url, struct oshu_audio **audio);

/**
 * Start playing!
 *
 * The SDL plays audio in a separate thread, so you need not worry about
 * calling this function regularily or anything. Don't bother, it's magic!
 */
void oshu_audio_play(struct oshu_audio *audio);

/**
 * Pause the stream.
 *
 * Calling #oshu_audio_play will resume the audio playback where it was
 * left playing.
 */
void oshu_audio_pause(struct oshu_audio *audio);

/**
 * Close the audio stream and free everything associated to it, then set
 * `*audio` to *NULL.
 *
 * If `*audio` is *NULL*, do nothing.
 *
 * You must not call this function with a null pointer though.
 */
void oshu_audio_close(struct oshu_audio **audio);

/**
 * \defgroup sample Sample
 *
 * \brief
 * Load WAV files using SDL's WAV loader in order to play them over an audio
 * stream as sound effects.
 *
 * A sample is a short sound played when the user hits something. To be fast
 * and reactive, the samples are always stored in-memory. Do not confuse it
 * with a PCM sample.
 *
 * Use #oshu_sample_play to start playing a sample on top of the currently
 * playing audio.
 *
 * ```c
 * struct oshu_audio *audio; // defined somewhere else
 * struct oshu_sample *sample;
 * oshu_sample_load("hit.wav", audio, &sample);
 * oshu_sample_play(audio, sample);
 * things();
 * oshu_sample_free(&sample);
 * ```
 *
 * \{
 */

/**
 * An in-memory audio sample.
 */
struct oshu_sample {
	Uint8 *buffer;
	Uint32 length;
	Uint32 cursor;
	int loop;
};

/**
 * Open a WAV file and store it into a newly-allocated structure.
 *
 * The specs of the SDL audio device is required in order to convert the
 * samples into the apprioriate format, which is why the stream argument is
 * required. Note that this means the sample will be specific to that stream
 * only.
 *
 * On success, you must free the sample object with #oshu_sample_free. On
 * failure, the object is freed for you.
 *
 * \param path Path to the WAV file to load.
 * \param audio The stream on top of which the sample is meant to be played.
 * \param sample Receive the sample object.
 *
 * \return 0 on success, -1 on failure.
 */
int oshu_sample_load(const char *path, struct oshu_audio *audio, struct oshu_sample **sample);

/**
 * Play a sample on top of a stream.
 *
 * Only one sample can be played at a time. Calling this function while a
 * sample is playing with interrupt the previous sample.
 *
 * Note that the sample must have been loaded for that specific audio stream.
 *
 * To stop playing any sample, call this function with `sample` as NULL.
 */
void oshu_sample_play(struct oshu_audio *audio, struct oshu_sample *sample);

/**
 * Free the sample object along with its buffer data.
 *
 * Note that you should close the associated #oshu_audio object before freeing
 * the samples it may use.
 *
 * Set `*sample` to *NULL.
 */
void oshu_sample_free(struct oshu_sample **sample);


/** \}
 *
 * \defgroup sampleset Sample set
 *
 * \brief
 * Manage the set of samples a beatmap uses.
 *
 * It is stored as an array of pointers to sample, whose index is a combination
 * of flags from the structure below.
 *
 * This submodule is currently unimplemented. You must not call its functions
 * or you'll get a link error.
 *
 * \{
 */

enum oshu_sampleset_index {
	OSHU_SAMPLESET_CUSTOM_INDEX_MASK = 0b11,
	OSHU_SAMPLESET_SET_AUTO   = 0,
	OSHU_SAMPLESET_SET_NORMAL = 0b0100,
	OSHU_SAMPLESET_SET_SOFT   = 0b1000,
	OSHU_SAMPLESET_SET_DRUM   = 0b1100,
	OSHU_SAMPLESET_SET_MASK   = 0b1100,
	OSHU_SAMPLESET_SET_OFFSET = 2,
	OSHU_SAMPLESET_ADDITION_NONE    = 0,
	OSHU_SAMPLESET_ADDITION_WHISTLE = 0b010000,
	OSHU_SAMPLESET_ADDITION_FISNISH = 0b100000,
	OSHU_SAMPLESET_ADDITION_CLAP    = 0b110000,
	OSHU_SAMPLESET_ADDITION_MASK    = 0b110000,
	OSHU_SAMPLESET_ADDITION_OFFSET  = 4,
	OSHU_SAMPLESET_SIZE = 64,
};

typedef struct oshu_sample *oshu_sampleset;

int oshu_sampleset_new(oshu_sampleset *set);

/**
 * Try to load a sample found on the filesystem, trying the directories in that
 * order:
 *
 * 1. The current directory.
 * 2. The user's home oshu directory, like ~/.config/oshu/samples/.
 * 3. The oshu installation's share directory, like /usr/share/oshu/samples/.
 *
 * When a sample wasn't found, return 0 and leave the entry in the sample set
 * to *NULL*.
 */
int oshu_sampleset_load(oshu_sampleset set, int index);

/**
 * Call #oshu_sampleset_load for every possible sample.
 */
int oshu_sampleset_load_all(oshu_sampleset set);

/**
 * Free the sample set and all the samples it contains.
 *
 * Set `*set` to *NULL*.
 */
void oshu_sampleset_free(oshu_sampleset *set);

/** \} */

/** \} */
