add_executable(
	oshu-library
	EXCLUDE_FROM_ALL
	main.cc
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
