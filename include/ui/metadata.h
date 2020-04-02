/**
 * \file ui/metadata.h
 * \ingroup ui_metadata
 */

#pragma once

#include "video/texture.h"

namespace oshu {

struct beatmap;
struct display;
struct metadata;

/**
 * \defgroup ui_metadata Metadata
 * \ingroup ui
 *
 * \brief
 * Display beatmap metadata.
 *
 * \{
 */

/**
 * The metadata frame is a half-transparent bar on top of the game screen,
 * showing information about the beatmap.
 *
 * When both ASCII and Unicode metadata are available, the display switches
 * between each variant with a fading transition.
 */
struct metadata_frame {
	/**
	 * The display storing the textures.
	 */
	oshu::display *display = nullptr;
	/**
	 * The metadata source.
	 */
	oshu::beatmap *beatmap = nullptr;
	/**
	 * The clock points to an ever-increasing continuous time value, in
	 * seconds. The game clock's #oshu::clock::system is a good fit for this
	 * value.
	 *
	 * This clock determines which of #ascii and #unicode is displayed.
	 */
	double *clock = nullptr;
	/**
	 * The ASCII metadata.
	 *
	 * White text on a transparent background.
	 *
	 * 2 lines of text: the first with the title, the second with the
	 * artist name.
	 */
	oshu::texture ascii;
	/**
	 * Unicode variant of #ascii.
	 *
	 * When the Unicode metadata is missing, or identical to the ASCII
	 * ones, this texture is left null.
	 */
	oshu::texture unicode;
	/**
	 * Show difficulty information.
	 *
	 * The first line is the #oshu::metadata::version, and the second the
	 * difficulty value in stars.
	 */
	oshu::texture stars;
};

/**
 * Create a metadata frame.
 *
 * Refer to #oshu::metadata_frame for documentation on the parameters.
 *
 * `frame` needs not be initialized.
 *
 * When done, please destroy the frame with #oshu::destroy_metadata_frame.
 */
int create_metadata_frame(oshu::display *display, oshu::beatmap *beatmap, double *clock, oshu::metadata_frame *frame);

/**
 * Display the metadata frame on the configured display with the given opacity.
 *
 * *opacity* is a number between 0 and 1. You may want to use #oshu::fade_out to
 * compute that value.
 *
 * Metadata are drawn in white text and a translucent black background for
 * readability.
 *
 * Every 3.5 second, the display is switched between Unicode and ASCII, with a
 * 0.2-second fade transititon.
 */
void show_metadata_frame(oshu::metadata_frame *frame, double opacity);

/**
 * Free the allocated textures.
 */
void destroy_metadata_frame(oshu::metadata_frame *frame);

/** \} */

}
