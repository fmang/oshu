/**
 * \file graphics/display.h
 * \ingroup graphics_display
 */

#pragma once

#include "beatmap/geometry.h"
#include "graphics/view.h"

struct SDL_Renderer;
struct SDL_Window;

/**
 * \defgroup graphics Graphics
 *
 * The graphics module is an abstraction over SDL2.
 *
 * The \ref graphics_display module defines a game window abstraction. The
 * coordinate system is configured with the display's \ref graphics_view.
 *
 * Drawing consists primarily in pasting textures from the \ref
 * graphics_texture module. Most textures are currently generated using the
 * cairo vector graphics library. The \ref graphics_paint module integrates
 * cairo with SDL2 and the \ref graphics_texture module.
 *
 * To draw text, you will need pango, and more specifically pangocairo. Pango
 * is not directly integrated with this module, but is relatively easy to use
 * as all it requires is a cairo context, which the \ref graphics_paint module
 * creates. Note that for some reason, drawing text on a transparent background
 * causes a visual glitch unless the blend mode is set to *source*.
 *
 * \dot
 * digraph modules {
 * 	rankdir=BT;
 * 	node [shape=rect];
 * 	Paint -> Texture -> Display -> View;
 * 	subgraph {
 * 		rank=same;
 * 		SDL_Window [shape=ellipse];
 * 		SDL_Renderer [shape=ellipse];
 * 		Display -> SDL_Renderer -> SDL_Window;
 * 	}
 * 	subgraph {
 * 		rank=same;
 * 		SDL_Texture [shape=ellipse];
 * 		Texture -> SDL_Texture;
 * 	}
 * 	SDL_Texture -> SDL_Renderer;
 * 	subgraph {
 * 		rank=same;
 * 		Cairo [shape=ellipse];
 * 		Paint -> Cairo [constraint=False];
 * 	}
 * 	Pango [shape=ellipse];
 * 	Pango -> Cairo;
 * }
 * \enddot
 */

/**
 * \defgroup graphics_display Display
 * \ingroup graphics
 *
 * \brief
 * Abstract an SDL2 window.
 *
 * To avoid reinventing the wheel, this module provides only the most basic
 * functions for initializing the window and its renderer together, with the
 * appropriate settings.
 *
 * The main drawing operation on a display is #oshu_draw_texture. The other
 * drawing primitives must be called straight from SDL using the display's
 * underlying renderer. Note that while wrapped operations transform the
 * coordinates using the display's view, you would need to project the
 * coordinates yourself with #oshu_project to get a consistent behavior with
 * the equivalent SDL routines.
 *
 * \{
 */

/**
 * Define the list of customizable visual elements.
 *
 * Enabling a flag is always worse for performance, so the fastest setup is 0,
 * and the fanciest is OR'ing all these flags together.
 */
enum oshu_visual_feature {
	/**
	 * Control the `SDL_HINT_RENDER_SCALE_QUALITY`.
	 *
	 * By default, it is set to *nearest*, which is fast but breaks
	 * antialising when textures are scaled. *linear* gives much nicer
	 * results, but is much more expensive.
	 *
	 * \todo
	 * Implement this flag.
	 */
	OSHU_LINEAR_SCALING,
	/**
	 * Display a background picture rather than a pitch black screen.
	 *
	 * \todo
	 * Implement this flag.
	 */
	OSHU_SHOW_BACKGROUND,
	/**
	 * Show a fancy software mouse cursor instead of the default one.
	 *
	 * \todo
	 * Implement this flag.
	 */
	OSHU_FANCY_CURSOR,
};

/**
 * The quality level defines what kind of visual effects are enabled.
 *
 * This is controllable through the `OSHU_QUALITY` environment variable:
 *
 * - `-` suffix for #OSHU_LOW_QUALITY,
 * - no suffix for #OSHU_MEDIUM_QUALITY,
 * - `+` suffix for #OSHU_HIGH_QUALITY.
 *
 * A quality level is actually a combination of visual features from
 * #oshu_visual_feature.
 */
enum oshu_quality_level {
	/**
	 * Lowest quality for the poorest computers.
	 *
	 * It should be so pure than virtually anything below that point is
	 * unplayable.
	 *
	 * No background picture.
	 */
	OSHU_LOW_QUALITY = 0,
	/**
	 * Quality for pretty bad computers.
	 *
	 * The scaling algorithm is set to *nearest*. Get ready for aliasing
	 * and staircases.
	 */
	OSHU_MEDIUM_QUALITY = OSHU_LOW_QUALITY|OSHU_SHOW_BACKGROUND|OSHU_FANCY_CURSOR,
	/**
	 * Best quality.
	 *
	 * Use linear scaling, and make textures fit the window size exactly,
	 * whatever the window size is.
	 */
	OSHU_HIGH_QUALITY = OSHU_MEDIUM_QUALITY|OSHU_LINEAR_SCALING,
};

/**
 * Store everything related to the current display.
 *
 * A display is an SDL window, a renderer, and a coordinate system.
 *
 * Any textures allocated for a display are specific to that display, and
 * should be freed first.
 *
 * \sa oshu_open_display
 * \sa oshu_close_display
 */
struct oshu_display {
	/**
	 * The one and only SDL game window.
	 */
	struct SDL_Window *window;
	/**
	 * The renderer for displaying accelerated graphics.
	 *
	 * It must be freed after the textures, but before the window.
	 */
	struct SDL_Renderer *renderer;
	/**
	 * The current view, used to project coordinates when drawing.
	 *
	 * When the window is resized, make sure you reset it with
	 * #oshu_reset_view and recreate your view from it.
	 */
	struct oshu_view view;
	/**
	 * Bitmap of visual features, defined in #oshu_visual_feature.
	 *
	 * It is by default loaded from the `OSHU_QUALITY` environment
	 * variable, using the values defined by #oshu_quality_level.
	 *
	 * \todo
	 * Load it from the OSHU_QUALITY environment variable on display
	 * initialization. OSHU_QUALITY will contain values like `1.5+`, `1.5`
	 * or `1.5-`, mapping respectively to high, medium and low quality. By
	 * default, high quality is selected.
	 */
	int features;
	/**
	 * The quality factor defines the texture size, and the default window
	 * size.
	 *
	 * A quality factor of 1 represents a standard 640×480 osu! screen. A
	 * factor of 1.5 maps to oshu!'s default 960×640 screen.
	 *
	 * When the quality factor is 0, texture builders fall back on the
	 * current #view's #oshu_view::zoom. This is usually the best choice,
	 * as textures are generated to fit the screen perfectly.
	 *
	 * You'll get nothing but worse graphics from increasing this value
	 * more than necessary. However, you may want to decrease it when you
	 * want a big window but cannot afford full-resolution textures.
	 *
	 * \todo
	 * Load it from the OSHU_QUALITY environment variable on display
	 * initialization.
	 *
	 * \todo
	 * Use it from the paint submodule when building textures.
	 */
	double quality_factor;
};

/**
 * Create a display structure, open the SDL window and create the renderer.
 *
 * *display* must be null-initialized.
 *
 * \sa oshu_close_display
 */
int oshu_open_display(struct oshu_display *display);

/**
 * Free the display structure and everything associated to it.
 *
 * *display* is left in an unspecified state, but it is okay to call this
 * function multiple times.
 *
 * Calling it on a null-initialized structure is also valid.
 */
void oshu_close_display(struct oshu_display *display);

/**
 * Get the mouse position.
 *
 * SDL provides it in window coordinates, which are unprojected into the
 * current coordinate system for the display.
 *
 * \sa oshu_unproject
 */
oshu_point oshu_get_mouse(struct oshu_display *display);

/** \} */
