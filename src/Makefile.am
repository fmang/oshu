bin_PROGRAMS = oshu

oshu_SOURCES = \
	oshu.c oshu.h \
	audio.c audio.h \
	beatmap.c beatmap.h \
	graphics.c graphics.h \
	geometry.h geometry.c \
	game.h game.c \
	log.h

oshu_CFLAGS = $(SDL2_CFLAGS) $(LAVF_CFLAGS) $(LAVC_CFLAGS) $(LAVU_CFLAGS)
oshu_LDADD = -lm $(SDL2_LIBS) $(LAVF_LIBS) $(LAVC_LIBS) $(LAVU_LIBS)
