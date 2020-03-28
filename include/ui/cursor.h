/**
 * \file ui/cursor.h
 * \ingroup ui_cursor
 */

#pragma once

#include "core/geometry.h"
#include "video/texture.h"

namespace oshu {

struct display;

/**
 * \defgroup ui_cursor Cursor
 * \ingroup ui
 *
 * \brief
 * Fancy software mouse cursor.
 *
 * To enable this module, the #oshu::FANCY_CURSOR flag must be enabled for the
 * display. Otherwise, this module behaves like a stub and do nothing.
 *
 * \{
 */

struct cursor_widget {
	/**
	 * The display associated to the cursor.
	 *
	 * It is used both to retrieve the mouse position, and to render the
	 * cursor.
	 */
	oshu::display *display;
	/**
	 * Keep track of the previous positions of the mouse to display a
	 * fancier cursor, with a trail.
	 *
	 * This is a circular array, starting at #offset. The previous is at
	 * #offset - 1, and so on. When you reach the maximum index, wrap at 0.
	 */
	oshu::point history[4];
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
	oshu::texture mouse;
};

/**
 * Create a cursor for the display, and paint its texture with cairo.
 *
 * You must free the cursor with #oshu::destroy_cursor.
 */
int create_cursor(oshu::display *display, oshu::cursor_widget *cursor);

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
void show_cursor(oshu::cursor_widget *cursor);

/**
 * Free the cursor's texture.
 *
 * It is safe to call this function on a destroyed cursor, or on a
 * zero-initialized cursor.
 */
void destroy_cursor(oshu::cursor_widget *cursor);

/** \} */

}
