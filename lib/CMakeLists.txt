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
	game/screens/pause.cc
	game/screens/play.cc
	game/screens/score.cc
	game/tty.cc
	graphics/display.cc
	graphics/paint.cc
	graphics/texture.cc
	graphics/transitions.cc
	graphics/view.cc
	gui/audio.cc
	gui/background.cc
	gui/cursor.cc
	gui/metadata.cc
	gui/score.cc
	library/beatmaps.cc
	library/html.cc
	osu/draw.cc
	osu/osu.cc
	osu/paint.cc
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
