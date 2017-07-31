bin_PROGRAMS = oshu

oshu_SOURCES = \
	oshu.c oshu.h \
	audio.c audio.h \
	beatmap.c beatmap.h \
	graphics.c graphics.h \
	log.h

oshu_CFLAGS = $(SDL2_CFLAGS) $(SDL2_image_CFLAGS) $(LAVF_CFLAGS) $(LAVC_CFLAGS) $(LAVU_CFLAGS)
oshu_LDADD = $(SDL2_LIBS) $(SDL2_image_LIBS) $(LAVF_LIBS) $(LAVC_LIBS) $(LAVU_LIBS)
