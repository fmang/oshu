bin_PROGRAMS = oshu

oshu_SOURCES = \
	oshu.c \
	log.h \
	audio/audio.h \
	audio/pipeline.c \
	audio/sample.c \
	audio/sample.h \
	audio/stream.c \
	audio/stream.h \
	beatmap/beatmap.h \
	beatmap/helpers.c \
	beatmap/parser.c \
	beatmap/parser.h \
	beatmap/geometry.c \
	beatmap/geometry.h \
	game/game.c \
	game/game.h \
	game/modes.h \
	game/osu.c \
	graphics/display.c \
	graphics/display.h \
	graphics/draw.c \
	graphics/draw.h

oshu_CFLAGS = -iquote . -DPKGDATADIR='"$(pkgdatadir)"' $(SDL2_CFLAGS) $(SDL2_image_CFLAGS) $(LAVF_CFLAGS) $(LAVC_CFLAGS) $(LSWR_CFLAGS) $(LAVU_CFLAGS)
oshu_LDADD = -lm $(SDL2_LIBS) $(SDL2_image_LIBS) $(LAVF_LIBS) $(LAVC_LIBS) $(LSWR_LIBS) $(LAVU_LIBS)
