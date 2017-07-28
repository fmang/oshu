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
	int rc = avformat_open_input(&(*stream)->context, url, NULL, NULL);
	if (rc)
		goto fail;
	rc = av_find_best_stream(
		(*stream)->context,
		AVMEDIA_TYPE_AUDIO,
		-1, -1,
		&(*stream)->decoder,
		0
	);
	if (rc)
		goto fail;
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
	avformat_close_input(&(*stream)->context);
	free(*stream);
	*stream = NULL;
}
