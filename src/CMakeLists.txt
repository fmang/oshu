include_directories(.)

add_library(
	liboshu OBJECT
	audio/audio.cc
	audio/audio.h
	audio/library.cc
	audio/library.h
	audio/sample.cc
	audio/sample.h
	audio/stream.cc
	audio/stream.h
	audio/track.cc
	audio/track.h
	beatmap/beatmap.h
	beatmap/geometry.cc
	beatmap/geometry.h
	beatmap/helpers.cc
	beatmap/parser.cc
	beatmap/parser.h
	beatmap/tokens.h
	game/actions.cc
	game/clock.cc
	game/clock.h
	game/controls.cc
	game/controls.h
	game/game.cc
	game/game.h
	game/helpers.cc
	game/screens/pause.cc
	game/screens/play.cc
	game/screens/score.cc
	game/screens/screens.h
	game/tty.cc
	game/tty.h
	graphics/display.cc
	graphics/display.h
	graphics/paint.cc
	graphics/paint.h
	graphics/texture.cc
	graphics/texture.h
	graphics/transitions.cc
	graphics/transitions.h
	graphics/view.cc
	graphics/view.h
	log.h
	osu/draw.cc
	osu/osu.cc
	osu/osu.h
	osu/paint.cc
	ui/audio.cc
	ui/audio.h
	ui/background.cc
	ui/background.h
	ui/cursor.cc
	ui/cursor.h
	ui/metadata.cc
	ui/metadata.h
	ui/score.cc
	ui/score.h
	ui/ui.h
)

target_compile_options(
	liboshu PUBLIC
	${SDL_CFLAGS}
	${FFMPEG_CFLAGS}
	${CAIRO_CFLAGS}
	${PANGO_CFLAGS}
)

add_executable(
	oshu
	oshu.cc
	$<TARGET_OBJECTS:liboshu>
)

target_compile_options(
	oshu PUBLIC
	${SDL_CFLAGS}
	${FFMPEG_CFLAGS}
	${CAIRO_CFLAGS}
	${PANGO_CFLAGS}
)

target_link_libraries(
	oshu PUBLIC
	${SDL_LIBRARIES}
	${FFMPEG_LIBRARIES}
	${CAIRO_LIBRARIES}
	${PANGO_LIBRARIES}
)

install(
	TARGETS oshu
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
