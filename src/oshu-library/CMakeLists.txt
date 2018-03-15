add_executable(
	oshu-library
	main.cc
	build_index.cc
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

install(
	TARGETS oshu-library
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
