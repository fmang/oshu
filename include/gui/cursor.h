/**
 * \file gui/cursor.h
 * \ingroup gui_cursor
 */

#pragma once

#include "core/geometry.h"
#include "video/texture.h"

struct oshu_display;

/**
 * \defgroup gui_cursor Cursor
 * \ingroup gui
 *
 * \brief
 * Fancy software mouse cursor.
 *
 * To enable this module, the #OSHU_FANCY_CURSOR flag must be enabled for the
 * display. Otherwise, this module behaves like a stub and do nothing.
 *
 * \{
 */

struct oshu_cursor_widget {
	/**
	 * The display associated to the cursor.
	 *
	 * It is used both to retrieve the mouse position, and to render the
	 * cursor.
	 */
	struct oshu_display *display;
	/**
	 * Keep track of the previous positions of the mouse to display a
	 * fancier cursor, with a trail.
	 *
	 * This is a circular array, starting at #offset. The previous is at
	 * #offset - 1, and so on. When you reach the maximum index, wrap at 0.
	 */
	oshu_point history[4];
	/**
	 * Index of the most recent point in #history.
	 */
	int offset;
	/**
	 * The software mouse cursor picture.
	 *
	 * It's a white disc, which is scaled and whose opacity is adjusted for
	 * the main cursor and all the trailing particles.
	 */
	struct oshu_texture mouse;
};

/**
 * Create a cursor for the display, and paint its texture with cairo.
 *
 * You must free the cursor with #oshu_destroy_cursor.
 */
int oshu_create_cursor(struct oshu_display *display, struct oshu_cursor_widget *cursor);

/**
 * Render the cursor on the display it was created on.
 *
 * The display's current view affects how the cursor position is retrieved, and
 * its zoom factor would change the size of the cursor, so you should make sure
 * this function is always called with the same view.
 *
 * Every call to this function update the mouse position history, affecting the
 * cursor trail.
 */
void oshu_show_cursor(struct oshu_cursor_widget *cursor);

/**
 * Free the cursor's texture.
 *
 * It is safe to call this function on a destroyed cursor, or on a
 * zero-initialized cursor.
 */
void oshu_destroy_cursor(struct oshu_cursor_widget *cursor);

/** \} */
