#include "oshu.h"

#include <assert.h>

#define SAMPLE_BUFFER_SIZE 4096 /* # of samples */

static void log_av_error(int rc)
{
	char errbuf[256];
	av_strerror(rc, errbuf, sizeof(errbuf));
	oshu_log_error("ffmpeg error: %s", errbuf);
}

void oshu_audio_init()
{
	av_register_all();
}

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

static void next_frame(struct oshu_audio_stream *stream)
{
	while (!stream->finished) {
		int rc = avcodec_receive_frame(stream->decoder, stream->frame);
		if (rc == 0) {
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

static void dump_stream_info(struct oshu_audio_stream *stream)
{
	oshu_log_info("audio codec: %s", stream->codec->long_name);
	oshu_log_info("sample rate: %d", stream->decoder->sample_rate);
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

int oshu_audio_play(struct oshu_audio_stream *stream)
{
	SDL_PauseAudioDevice(stream->device_id, 0);
	return 0;
}

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
