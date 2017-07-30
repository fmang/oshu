#pragma once

#include <libavformat/avformat.h>
#include <SDL2/SDL.h>

/** @defgroup audio Audio
 *
 * Oshu's audio module.
 *
 * Open and decode any kind of audio file using ffmpeg. libavformat does the
 * demuxing, libavcodec decodes. Then feed the decoded samples to SDL's audio
 * device.
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
 * lag can be rectified with a voluntary bias on the computed position.
 *
 * Playing a random file, I noticed the SDL callback for a buffer of 2048
 * samples is called about every 20 or 30 ms, which was mapped nicely to a
 * single frame. This is unfortunately too coarse under 60 FPS, so time we'll
 * use for the game is corrected with the delta between the frame decoding
 * timestamp and the current timestamp.
 *
 * To use this module, first call \ref oshu_audio_init. Then open streams using
 * \ref oshu_audio_open, and play them using \ref oshu_audio_play. When you're
 * done, close your streams with \ref oshu_audio_close.
 *
 * ```
 * oshu_audio_init();
 * struct oshu_audio *stream;
 * oshu_audio_open("file.ogg", &stream);
 * oshu_audio_play(stream);
 * do_things();
 * oshu_audio_close(stream);
 * ```
 *
 * Depending on how the SDL handles multiple devices, you may not be able to
 * open simultaneous streams.
 *
 * @{
 */

/**
 * An audio stream.
 *
 * This includes everything related to the playback of an audio file, from the
 * demuxer and decoder to the audio device that will output the sound.
 *
 * This structure is mainly accessed through an audio thread, and should be
 * locked using SDL's `SDL_LockAudio` and `SDL_UnlockAudio` procedures in order
 * to be accessed peacefully.
 */
struct oshu_audio {
	AVFormatContext *demuxer;
	AVCodec *codec;
	int stream_index;
	AVCodecContext *decoder;
	SDL_AudioDeviceID device_id;
	SDL_AudioSpec device_spec;
	AVFrame *frame;
	/** When the last frame was decoded, according to
	 *  `av_gettime_relative`. */
	int64_t relative_timestamp;
	/** Current position in the decoded frame's buffer.
	 *  Multiply it by the sample size to get the position in bytes. */
	int sample_index;
	/** When true, stop decoding the stream and output silence. */
	int finished;
	/** Sound sample to play on top of the audio stream. */
	struct oshu_sample *overlay;
};

/**
 * Initialize ffmpeg.
 *
 * Make sure you call it once at the beginning of the program.
 */
void oshu_audio_init();

/**
 * Opens a file and initialize everything needed to play it.
 *
 * The stream can then be close with \ref oshu_audio_close.
 *
 * @param stream Will receive a newly allocated stream object.
 *
 * @return 0 on success. On error, -1 is returned and everything is free'd.
 */
int oshu_audio_open(const char *url, struct oshu_audio **stream);

/**
 * Start playing!
 *
 * The SDL plays audio in a separate thread, so you need not worry about
 * calling this function regularily or anything. Don't bother, it's magic!
 */
void oshu_audio_play(struct oshu_audio *stream);

/**
 * Compute the current position in the stream in seconds.
 *
 * The time is computed from the best effort timestamp of the last decoded
 * frame, and the time elasped between it was decoded.
 *
 * You *should* lock the audio using `SDL_LockAudio` when accessing it.
 */
double oshu_audio_position(struct oshu_audio *stream);

/**
 * Play a sample on top of the stream.
 */
void oshu_audio_play_sample(struct oshu_audio *stream, struct oshu_sample *sample);

/**
 * Close the audio stream and free everything associated to it.
 *
 * Set `*stream` to *NULL*.
 */
void oshu_audio_close(struct oshu_audio **stream);

/** @} */
