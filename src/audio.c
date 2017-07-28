#include "oshu.h"

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

int oshu_audio_open(const char *url, struct oshu_audio_stream **stream)
{
	*stream = calloc(1, sizeof(**stream));
	int rc = avformat_open_input(&(*stream)->demuxer, url, NULL, NULL);
	if (rc < 0) {
		oshu_log_error("failed opening the audio file");
		goto fail;
	}
	rc = av_find_best_stream(
		(*stream)->demuxer,
		AVMEDIA_TYPE_AUDIO,
		-1, -1,
		&(*stream)->codec,
		0
	);
	if (rc < 0 || (*stream)->codec == NULL) {
		oshu_log_error("error finding the best audio stream");
		goto fail;
	}
	(*stream)->stream_id = rc;
	return 0;
fail:
	log_av_error(rc);
	oshu_audio_close(stream);
	return -1;
}

int oshu_audio_play(struct oshu_audio_stream *stream)
{
	return 0;
}

void oshu_audio_close(struct oshu_audio_stream **stream)
{
	avformat_close_input(&(*stream)->demuxer);
	free(*stream);
	*stream = NULL;
}
