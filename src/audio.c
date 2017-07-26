#include "oshu.h"

#include <libavformat/avformat.h>

int oshu_audio_init()
{
	av_register_all();
	return 0;
}

int oshu_audio_open(const char *url, struct oshu_audio_stream **stream)
{
	return 0;
}

int oshu_audio_play(struct oshu_audio_stream *stream)
{
	return 0;
}

int oshu_audio_close(struct oshu_audio_stream *stream)
{
	return 0;
}

