cmake_minimum_required(VERSION 3.9)

project(
	oshu
	VERSION 2.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPkgConfig)
pkg_check_modules(SDL REQUIRED sdl2 SDL2_image)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(PANGO REQUIRED pangocairo)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil)
pkg_check_modules(RESAMPLE libswresample)
if (RESAMPLE_FOUND)
	set(RESAMPLE_BACKEND swr)
else ()
	pkg_check_modules(RESAMPLE libavresample)
	if (RESAMPLE_FOUND)
		set(RESAMPLE_BACKEND avr)
	else ()
		message(FATAL_ERROR "you need one of libswresample or libavresample")
	endif ()
endif ()

include(GNUInstallDirs)
# GNUInstallDirs creates one variable for the install() commands, and one for
# the final absolute path. We'll suffix the former with INSTALL_DIRECTORY.
set(OSHU_DATA_INSTALL_DIRECTORY "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
set(OSHU_SKINS_INSTALL_DIRECTORY "${OSHU_DATA_INSTALL_DIRECTORY}/skins")
set(OSHU_WEB_INSTALL_DIRECTORY "${OSHU_DATA_INSTALL_DIRECTORY}/web")
# Absolute paths for config.h:
set(OSHU_DATA_DIRECTORY "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
set(OSHU_SKINS_DIRECTORY "${OSHU_DATA_DIRECTORY}/skins")
set(OSHU_WEB_DIRECTORY "${OSHU_DATA_DIRECTORY}/web")

configure_file(config.h.in config.h)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

include_directories(include)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(share)
enable_testing()
add_subdirectory(test)

find_package(Doxygen)
if (DOXYGEN_FOUND)
	configure_file(Doxyfile.in Doxyfile)
	add_custom_target(html
		COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
		COMMENT "Generating the API documentation in ${CMAKE_BINARY_DIR}/html"
	)
endif (DOXYGEN_FOUND)
