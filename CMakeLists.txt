cmake_minimum_required(VERSION 3.10)

project(
	oshu
	VERSION 2.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)

include(FindPkgConfig)
pkg_check_modules(SDL REQUIRED sdl2 SDL2_image)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libswresample libavutil)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(PANGO REQUIRED pangocairo)

include(GNUInstallDirs)

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(include)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(share)

find_package(Doxygen)
if (DOXYGEN_FOUND)
	configure_file(Doxyfile.in Doxyfile @ONLY)
	add_custom_target(html
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		COMMENT "Generating the API documentation in ${CMAKE_BINARY_DIR}/html"
	)
endif (DOXYGEN_FOUND)
