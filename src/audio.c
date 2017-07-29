/**
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
 *    and returns pages until EOF. The packets are then passed to libavcodec
 *    for decoding.
 *
 * If this is not clear, here a few elements of structure and ffmpeg
 * terminology you should know:
 *
 * - A file contains streams. Audio, video, subtitles are all streams.
 * - Each stream is split into packets.
 * - Files are serialized by interleaving packets in chronological order.
 * - Each packet is decoded into one or more frames.
 * - A frame, in the case of audio, is a buffer of decoded samples.
 * - Samples are organized into channels. 2 channels for stereo.
 *
 * The presentation timestamp (PTS) is the time a frame should be displayed.
 * It's the job of the container format to maintain it, and is printed into
 * every packet. ffmpeg insert a more precise timestamp into every frame called
 * the best effort timestamp. This is what we'll use.
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
 */

#include "oshu.h"

#include <assert.h>
#include <libavutil/time.h>

/** Size of the SDL audio buffer, in samples.
 *  This is 23 ms in 44.1 KHz stereo. */
#define SAMPLE_BUFFER_SIZE 2048

static void log_av_error(int rc)
{
	char errbuf[256];
	av_strerror(rc, errbuf, sizeof(errbuf));
	oshu_log_error("ffmpeg error: %s", errbuf);
}

/**
 * Initialize ffmpeg.
 * Make sure you call it once at the beginning of the program.
 */
void oshu_audio_init()
{
	av_register_all();
}

/**
 * Open the libavformat demuxer, and find the best audio stream.
 * Returns 0 on success and an ffmpeg error code on failure.
 */
static int open_demuxer(const char *url, struct oshu_audio_stream *stream)
{
	int rc = avformat_open_input(&stream->demuxer, url, NULL, NULL);
	if (rc < 0) {
		oshu_log_error("failed opening the audio file");
		return rc;
	}
	rc = av_find_best_stream(
		stream->demuxer,
		AVMEDIA_TYPE_AUDIO,
		-1, -1,
		&stream->codec,
		0
	);
	if (rc < 0 || stream->codec == NULL) {
		oshu_log_error("error finding the best audio stream");
		return rc;
	}
	stream->stream_id = rc;
	return 0;
}

/**
 * Read a page for the stream and feed it to the decoder.
 * When reaching EOF, feed the decoder a NULL packet to flush it.
 * On error, log a message and mark the stream as finished to stop the
 * playback.
 */
static void next_page(struct oshu_audio_stream *stream)
{
	int rc;
	AVPacket packet;
	for (;;) {
		rc = av_read_frame(stream->demuxer, &packet);
		if (rc == 0) {
			if (packet.stream_index == stream->stream_id) {
				if ((rc = avcodec_send_packet(stream->decoder, &packet)))
					goto fail;
				return;
			}
		} else if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last page, flushing");
			if ((rc = avcodec_send_packet(stream->decoder, NULL)))
				goto fail;
			else
				return;
		} else {
			goto fail;
		}
	}
fail:
	log_av_error(rc);
	stream->finished = 1;
}

/**
 * Decode a frame into stream->frame.
 * Request a new page when the decoder needs more data.
 * Mark the stream as finished on EOF or on ERROR.
 */
static void next_frame(struct oshu_audio_stream *stream)
{
	while (!stream->finished) {
		int rc = avcodec_receive_frame(stream->decoder, stream->frame);
		if (rc == 0) {
			stream->relative_timestamp = av_gettime_relative();
			stream->sample_index = 0;
			return;
		} else if (rc == AVERROR(EAGAIN)) {
			next_page(stream);
		} else if (rc == AVERROR_EOF) {
			oshu_log_debug("reached the last frame");
			stream->finished = 1;
		} else {
			/* Abandon all hope */
			log_av_error(rc);
			stream->finished = 1;
		}
	}
}

/**
 * Initialize the libavcodec decoder.
 * Return an ffmpeg error code on failure, and 0 on success.
 */
static int open_decoder(struct oshu_audio_stream *stream)
{
	stream->decoder = avcodec_alloc_context3(stream->codec);
	int rc = avcodec_parameters_to_context(
		stream->decoder,
		stream->demuxer->streams[stream->stream_id]->codecpar
	);
	if (rc < 0) {
		oshu_log_error("error copying the codec context");
		return rc;
	}
	rc = avcodec_open2(stream->decoder, stream->codec, NULL);
	if (rc < 0) {
		oshu_log_error("error opening the codec");
		return rc;
	}
	stream->frame = av_frame_alloc();
	next_frame(stream);
	return 0;
}

/**
 * Log some helpful information about the decoded audio stream.
 * Meant for debugging more than anything else.
 */
static void dump_stream_info(struct oshu_audio_stream *stream)
{
	oshu_log_info("audio codec: %s", stream->codec->long_name);
	oshu_log_info("sample rate: %d Hz", stream->decoder->sample_rate);
	oshu_log_info("number of channels: %d", stream->decoder->channels);
	oshu_log_info("format: %s", av_get_sample_fmt_name(stream->decoder->sample_fmt));
}

static void audio_callback(void *userdata, Uint8 *buffer, int len)
{
	struct oshu_audio_stream *stream;
	stream = (struct oshu_audio_stream*) userdata;
	int left = len;
	for (; left > 0 && !stream->finished; next_frame(stream)) {
		int ch;
		int sample_size = av_get_bytes_per_sample(stream->frame->format);
		while (left > 0 && stream->sample_index < stream->frame->nb_samples) {
			for (ch = 0; ch < stream->frame->channels; ch++) {
				memcpy(buffer, stream->frame->data[ch] + sample_size * stream->sample_index, sample_size);
				buffer += sample_size;
				left -= sample_size;
			}
			stream->sample_index++;
		}
	}
	assert (left >= 0);
	memset(buffer, left, stream->device_spec.silence);
}

/**
 * Initialize the SDL audio device.
 * Returns 0 on success, -1 on error.
 */
static int open_device(struct oshu_audio_stream *stream)
{
	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = stream->decoder->sample_rate;
	assert(stream->decoder->sample_fmt == AV_SAMPLE_FMT_FLTP); /* TODO */
	want.format = AUDIO_F32SYS;
	want.channels = stream->decoder->channels;
	want.samples = SAMPLE_BUFFER_SIZE;
	want.callback = audio_callback;
	want.userdata = (void*) stream;
	stream->device_id = SDL_OpenAudioDevice(NULL, 0, &want, &stream->device_spec, 0);
	if (stream->device_id == 0)
		return -1;
	return 0;
}

/**
 * Opens a file and initialize everything needed to play it.
 *
 * The oshu_audio_stream is allocated and its pointer returned through the
 * stream argument. Call oshu_audio_close to free the structure.
 *
 * Returns 0 on success. On error, -1 is returned and everything is free'd.
 */
int oshu_audio_open(const char *url, struct oshu_audio_stream **stream)
{
	*stream = calloc(1, sizeof(**stream));
	int rc = open_demuxer(url, *stream);
	if (rc < 0)
		goto fail_av;
	rc = open_decoder(*stream);
	if (rc < 0)
		goto fail_av;
	dump_stream_info(*stream);
	rc = open_device(*stream);
	if (rc < 0) {
		oshu_log_error("failed to open the audio device: %s", SDL_GetError());
		goto fail;
	}
	return 0;
fail_av:
	log_av_error(rc);
fail:
	oshu_audio_close(stream);
	return -1;
}

/**
 * Start playing!
 */
void oshu_audio_play(struct oshu_audio_stream *stream)
{
	SDL_PauseAudioDevice(stream->device_id, 0);
}

/**
 * Return the current position in the stream in seconds.
 * The time is computed from the best effort timestamp of the last decoded
 * frame, and the time elasped between it was decoded.
 */
double oshu_audio_position(struct oshu_audio_stream *stream)
{
	double base = av_q2d(stream->decoder->time_base);
	double timestamp = stream->frame->best_effort_timestamp;
	double delta_us = av_gettime_relative() - stream->relative_timestamp;
	return (timestamp * base) + (delta_us / 1e6);
}

/**
 * Close the audio stream and free everything associated to it.
 * Set *stream to NULL.
 */
void oshu_audio_close(struct oshu_audio_stream **stream)
{
	if ((*stream)->device_id)
		SDL_CloseAudioDevice((*stream)->device_id);
	av_frame_free(&(*stream)->frame);
	avcodec_close((*stream)->decoder);
	avformat_close_input(&(*stream)->demuxer);
	free(*stream);
	*stream = NULL;
}
