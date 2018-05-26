/**
 * \file video/display.h
 * \ingroup video_display
 */

#pragma once

#include "core/geometry.h"
#include "video/view.h"

struct SDL_Renderer;
struct SDL_Window;

/**
 * \defgroup video_display Display
 * \ingroup video
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
 *
 * \todo
 * Make a higher level object instead of exposing the raw flags.
 */
enum oshu_visual_feature {
	/**
	 * Control the `SDL_HINT_RENDER_SCALE_QUALITY`.
	 *
	 * By default, it is set to *nearest*, which is fast but breaks
	 * antialising when textures are scaled. *linear* gives much nicer
	 * results, but is much more expensive.
	 */
	OSHU_LINEAR_SCALING = 0x1,
	/**
	 * Display a background picture rather than a pitch black screen.
	 *
	 * Implemented by the \ref ui_background module.
	 */
	OSHU_SHOW_BACKGROUND = 0x2,
	/**
	 * Show a fancy software mouse cursor instead of the default one.
	 *
	 * This flag is implemented by the \ref ui_cursor module.
	 */
	OSHU_FANCY_CURSOR = 0x4,
	/**
	 * Prefer hardware rendering over software rendering.
	 *
	 * Since the graphics are quite simple, unless fancy scaling is enabled
	 * with #OSHU_LINEAR_SCALING, hardware video provide little
	 * advantage, and may even worsen the performance.
	 *
	 * One notable example is the Raspberry Pi which runs much smoother
	 * without acceleration.
	 *
	 * Note that enabling this flag doesn't force it on the SDL display,
	 * but disabling it forces software rendering.
	 */
	OSHU_HARDWARE_ACCELERATION = 0x8,
	/**
	 * Increase the frame rate to 60 FPS.
	 *
	 * On low-end hardware, 30 FPS is a good compromise, but most of the
	 * time 60 is much smoother.
	 */
	OSHU_60FPS = 0x10,
};

/**
 * The quality level defines what kind of visual effects are enabled.
 *
 * This is controllable through the `OSHU_QUALITY` environment variable:
 *
 * - `low` for #OSHU_LOW_QUALITY,
 * - `medium` for #OSHU_MEDIUM_QUALITY,
 * - `high` for #OSHU_HIGH_QUALITY.
 *
 * A quality level is actually a combination of visual features from
 * #oshu_visual_feature.
 *
 * In the future, the OSHU_QUALITY variable may be expanded to support finer
 * settings and allow enabling individual features instead of picking a level
 * from a limited set of values. For example, you could imagine
 * `medium-background`, or `low+acceleration`.
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
	OSHU_MEDIUM_QUALITY = OSHU_LOW_QUALITY|OSHU_SHOW_BACKGROUND|OSHU_FANCY_CURSOR|OSHU_60FPS,
	/**
	 * Best quality.
	 *
	 * Use linear scaling, and make textures fit the window size exactly,
	 * whatever the window size is.
	 */
	OSHU_HIGH_QUALITY = OSHU_MEDIUM_QUALITY|OSHU_LINEAR_SCALING|OSHU_HARDWARE_ACCELERATION,
	/**
	 * Default quality level.
	 *
	 * Used when the user didn't specify anything.
	 */
	OSHU_DEFAULT_QUALITY = OSHU_HIGH_QUALITY,
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
	 */
	int features;
	/**
	 * How long a frame should last in seconds.
	 *
	 * 0.01666… is 60 FPS.
	 *
	 * Its value depends on whether the #OSHU_60FPS is enabled or not. If
	 * it isn't, then the game runs at 30 FPS.
	 */
	double frame_duration;
};

/**
 * Create a display structure, open the SDL window and create the renderer.
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
