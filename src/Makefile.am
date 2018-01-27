bin_PROGRAMS = oshu

oshu_SOURCES = \
	oshu.cc \
	audio/audio.cc \
	audio/audio.h \
	audio/library.cc \
	audio/library.h \
	audio/sample.cc \
	audio/sample.h \
	audio/stream.cc \
	audio/stream.h \
	audio/track.cc \
	audio/track.h \
	beatmap/beatmap.h \
	beatmap/geometry.cc \
	beatmap/geometry.h \
	beatmap/helpers.cc \
	beatmap/parser.cc \
	beatmap/parser.h \
	beatmap/tokens.h \
	game/actions.cc \
	game/clock.cc \
	game/clock.h \
	game/controls.cc \
	game/controls.h \
	game/game.cc \
	game/game.h \
	game/helpers.cc \
	game/mode.h \
	game/screens/pause.cc \
	game/screens/play.cc \
	game/screens/score.cc \
	game/screens/screens.h \
	game/tty.cc \
	game/tty.h \
	graphics/display.cc \
	graphics/display.h \
	graphics/paint.cc \
	graphics/paint.h \
	graphics/texture.cc \
	graphics/texture.h \
	graphics/transitions.cc \
	graphics/transitions.h \
	graphics/view.cc \
	graphics/view.h \
	log.h \
	osu/draw.cc \
	osu/osu.cc \
	osu/osu.h \
	osu/paint.cc \
	ui/audio.cc \
	ui/audio.h \
	ui/background.cc \
	ui/background.h \
	ui/cursor.cc \
	ui/cursor.h \
	ui/metadata.cc \
	ui/metadata.h \
	ui/score.cc \
	ui/score.h \
	ui/ui.h

oshu_CXXFLAGS = -iquote . -DPKGDATADIR='"$(pkgdatadir)"' $(SDL2_CXXFLAGS) $(SDL2_image_CXXFLAGS) $(LAVF_CXXFLAGS) $(LAVC_CXXFLAGS) $(LSWR_CXXFLAGS) $(LAVU_CXXFLAGS) $(PANGOCAIRO_CXXFLAGS) $(CAIRO_CXXFLAGS)
oshu_LDADD = -lm $(SDL2_LIBS) $(SDL2_image_LIBS) $(LAVF_LIBS) $(LAVC_LIBS) $(LSWR_LIBS) $(LAVU_LIBS) $(PANGOCAIRO_LIBS) $(CAIRO_LIBS)
