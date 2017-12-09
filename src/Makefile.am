bin_PROGRAMS = oshu

oshu_SOURCES = \
	oshu.c \
	log.h \
	audio/audio.c \
	audio/audio.h \
	audio/library.c \
	audio/library.h \
	audio/sample.c \
	audio/sample.h \
	audio/stream.c \
	audio/stream.h \
	audio/track.c \
	audio/track.h \
	beatmap/beatmap.h \
	beatmap/geometry.c \
	beatmap/geometry.h \
	beatmap/helpers.c \
	beatmap/parser.c \
	beatmap/parser.h \
	beatmap/tokens.h \
	game/controls.h \
	game/game.c \
	game/game.h \
	game/mode.h \
	game/pause.c \
	game/play.c \
	game/score.c \
	game/screen.h \
	graphics/display.c \
	graphics/display.h \
	graphics/draw.c \
	graphics/draw.h \
	graphics/paint.c \
	graphics/paint.h \
	osu/draw.c \
	osu/osu.c \
	osu/osu.h \
	osu/paint.c

oshu_CFLAGS = -iquote . -DPKGDATADIR='"$(pkgdatadir)"' $(SDL2_CFLAGS) $(SDL2_image_CFLAGS) $(LAVF_CFLAGS) $(LAVC_CFLAGS) $(LSWR_CFLAGS) $(LAVU_CFLAGS) $(PANGOCAIRO_CFLAGS) $(CAIRO_CFLAGS)
oshu_LDADD = -lm $(SDL2_LIBS) $(SDL2_image_LIBS) $(LAVF_LIBS) $(LAVC_LIBS) $(LSWR_LIBS) $(LAVU_LIBS) $(PANGOCAIRO_LIBS) $(CAIRO_LIBS)
