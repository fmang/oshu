add_executable(
	oshu
	oshu.cc
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
	liboshu
	${SDL_LIBRARIES}
	${FFMPEG_LIBRARIES}
	${CAIRO_LIBRARIES}
	${PANGO_LIBRARIES}
)

install(
	TARGETS oshu
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

add_executable(
	oshu-library
	EXCLUDE_FROM_ALL
	oshu-library.cc
)

target_compile_options(
	oshu-library PUBLIC
	${SDL_CFLAGS}
)

target_link_libraries(
	oshu-library PUBLIC
	liboshu
	${SDL_LIBRARIES}
)
