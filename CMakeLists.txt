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

add_subdirectory(src)
add_subdirectory(share)
