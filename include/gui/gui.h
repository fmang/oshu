/**
 * \file gui/gui.h
 * \ingroup gui
 */

#pragma once

/**
 * \defgroup gui GUI
 *
 * \brief
 * Collection of graphical user interface elements.
 *
 * All the widgets share a common interface:
 *
 * - The widget's state is defined with a structure.
 *
 * - The widget is initialized and configured with a `create` function,
 *   returning an int. The first argument is an #oshu_display, and the last one
 *   a *widget*. All the other parameters are widget-specific.
 *
 * - The widget is destroyed with the symmetrical void `destroy` function. Note
 *   that the object itself isn't freed, because it may be allocated on the
 *   stack, or somewhere else.
 *
 * - You can render the widget with its void `show` function. They usually only
 *   take the widget object, but may receive additional parameters.
 *
 * \todo
 * Merge this module with graphics and call it *gui*?
 *
 * \todo
 * Drop this meta-header. Turn it into a .dox file for documentation.
 *
 */

#include "gui/audio.h"
#include "gui/background.h"
#include "gui/cursor.h"
#include "gui/metadata.h"
#include "gui/score.h"
