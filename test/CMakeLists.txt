add_executable(
	zerotokei
	EXCLUDE_FROM_ALL
	zerotokei.cc
)

target_compile_options(
	zerotokei PUBLIC
	${SDL_CFLAGS}
)

target_link_libraries(
	zerotokei PUBLIC
	liboshu
	${SDL_LIBRARIES}
)

add_test(
	NAME zerotokei
	COMMAND zerotokei
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(check
	COMMAND "${CMAKE_CTEST_COMMAND}"
	DEPENDS zerotokei
)
