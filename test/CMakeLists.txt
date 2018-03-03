add_executable(
	zerotokei
	zerotokei.cc
	$<TARGET_OBJECTS:liboshu-core>
	$<TARGET_OBJECTS:liboshu-beatmap>
)

target_compile_options(
	zerotokei PUBLIC
	${SDL_CFLAGS}
)

target_link_libraries(
	zerotokei PUBLIC
	${SDL_LIBRARIES}
)

add_test(
	NAME zerotokei
	COMMAND zerotokei
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
