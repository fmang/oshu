add_library(
	liboshu STATIC
	audio/audio.cc
	audio/library.cc
	audio/sample.cc
	audio/stream.cc
	audio/track.cc
	beatmap/geometry.cc
	beatmap/helpers.cc
	beatmap/parser.cc
	core/log.cc
	game/actions.cc
	game/clock.cc
	game/controls.cc
	game/game.cc
	game/helpers.cc
	game/tty.cc
	gui/audio.cc
	gui/background.cc
	gui/cursor.cc
	gui/metadata.cc
	gui/osu.cc
	gui/osu_paint.cc
	gui/score.cc
	gui/screens/pause.cc
	gui/screens/play.cc
	gui/screens/score.cc
	gui/window.cc
	library/beatmaps.cc
	library/html.cc
	osu/osu.cc
	video/display.cc
	video/paint.cc
	video/texture.cc
	video/transitions.cc
	video/view.cc
)

set_target_properties(
	liboshu
	PROPERTIES
	OUTPUT_NAME oshu
)

target_compile_options(
	liboshu PUBLIC
	${SDL_CFLAGS}
	${FFMPEG_CFLAGS}
	${CAIRO_CFLAGS}
	${PANGO_CFLAGS}
)
