add_executable(
	oshu
	main.cc
)

target_compile_options(
	oshu PUBLIC
	${SDL_CFLAGS}
	${FFMPEG_CFLAGS}
	${RESAMPLE_CFLAGS}
	${CAIRO_CFLAGS}
	${PANGO_CFLAGS}
)

target_link_libraries(
	oshu PUBLIC
	liboshu
	${SDL_LIBRARIES}
	${FFMPEG_LIBRARIES}
	${RESAMPLE_LIBRARIES}
	${CAIRO_LIBRARIES}
	${PANGO_LIBRARIES}
)

install(
	TARGETS oshu
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
