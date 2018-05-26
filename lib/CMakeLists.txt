add_library(
	liboshu STATIC
	audio/audio.cc
	audio/library.cc
	audio/sample.cc
	audio/stream.cc
	audio/track.cc
	beatmap/helpers.cc
	beatmap/parser.cc
	beatmap/path.cc
	core/geometry.cc
	core/log.cc
	game/actions.cc
	game/base.cc
	game/clock.cc
	game/controls.cc
	game/helpers.cc
	game/osu.cc
	game/tty.cc
	library/beatmaps.cc
	library/html.cc
	ui/audio.cc
	ui/background.cc
	ui/cursor.cc
	ui/metadata.cc
	ui/osu.cc
	ui/osu_paint.cc
	ui/score.cc
	ui/screens/pause.cc
	ui/screens/play.cc
	ui/screens/score.cc
	ui/window.cc
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
