bin_PROGRAMS = oshu

oshu_SOURCES = \
	oshu.c \
	log.h \
	audio/audio.c \
	audio/audio.h \
	beatmap/beatmap.c \
	beatmap/beatmap.h \
	beatmap/geometry.c \
	beatmap/geometry.h \
	game/game.c \
	game/game.h \
	game/modes.h \
	game/osu.c \
	graphics/graphics.c \
	graphics/graphics.h

oshu_CFLAGS = -iquote . -DPKGDATADIR='"$(pkgdatadir)"' $(SDL2_CFLAGS) $(LAVF_CFLAGS) $(LAVC_CFLAGS) $(LAVU_CFLAGS)
oshu_LDADD = -lm $(SDL2_LIBS) $(LAVF_LIBS) $(LAVC_LIBS) $(LAVU_LIBS)
