add_library(
	liboshu-core OBJECT
	core/log.cc
)

target_compile_options(
	liboshu-core PUBLIC
	${SDL_CFLAGS}
)

add_library(
	liboshu-beatmap OBJECT
	beatmap/geometry.cc
	beatmap/helpers.cc
	beatmap/parser.cc
)

target_compile_options(
	liboshu-beatmap PUBLIC
	${SDL_CFLAGS}
)

add_library(
	liboshu OBJECT
	audio/audio.cc
	audio/library.cc
	audio/sample.cc
	audio/stream.cc
	audio/track.cc
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
	osu/draw.cc
	osu/osu.cc
	osu/paint.cc
	ui/audio.cc
	ui/background.cc
	ui/cursor.cc
	ui/metadata.cc
	ui/score.cc
)

target_compile_options(
	liboshu PUBLIC
	${SDL_CFLAGS}
	${FFMPEG_CFLAGS}
	${CAIRO_CFLAGS}
	${PANGO_CFLAGS}
)
