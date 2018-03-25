/**
 * \file ui/metadata.h
 * \ingroup ui_metadata
 */

#pragma once

#include "video/texture.h"

struct oshu_display;
struct oshu_metadata;

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
struct oshu_metadata_frame {
	/**
	 * The display storing the textures.
	 */
	struct oshu_display *display;
	/**
	 * The metadata source.
	 */
	struct oshu_beatmap *beatmap;
	/**
	 * The clock points to an ever-increasing continuous time value, in
	 * seconds. The game clock's #oshu_clock::system is a good fit for this
	 * value.
	 *
	 * This clock determines which of #ascii and #unicode is displayed.
	 */
	double *clock;
	/**
	 * The ASCII metadata.
	 *
	 * White text on a transparent background.
	 *
	 * 2 lines of text: the first with the title, the second with the
	 * artist name.
	 */
	struct oshu_texture ascii;
	/**
	 * Unicode variant of #ascii.
	 *
	 * When the Unicode metadata is missing, or identical to the ASCII
	 * ones, this texture is left null.
	 */
	struct oshu_texture unicode;
	/**
	 * Show difficulty information.
	 *
	 * The first line is the #oshu_metadata::version, and the second the
	 * difficulty value in stars.
	 */
	struct oshu_texture stars;
};

/**
 * Create a metadata frame.
 *
 * Refer to #oshu_metadata_frame for documentation on the parameters.
 *
 * `frame` needs not be initialized.
 *
 * When done, please destroy the frame with #oshu_destroy_metadata_frame.
 */
int oshu_create_metadata_frame(struct oshu_display *display, struct oshu_beatmap *beatmap, double *clock, struct oshu_metadata_frame *frame);

/**
 * Display the metadata frame on the configured display with the given opacity.
 *
 * *opacity* is a number between 0 and 1. You may want to use #oshu_fade_out to
 * compute that value.
 *
 * Metadata are drawn in white text and a translucent black background for
 * readability.
 *
 * Every 3.5 second, the display is switched between Unicode and ASCII, with a
 * 0.2-second fade transititon.
 */
void oshu_show_metadata_frame(struct oshu_metadata_frame *frame, double opacity);

/**
 * Free the allocated textures.
 */
void oshu_destroy_metadata_frame(struct oshu_metadata_frame *frame);

/** \} */
