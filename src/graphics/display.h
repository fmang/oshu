/**
 * \file graphics/display.h
 * \ingroup display
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
 * \{
 */

/**
 * Store everything related to the current display.
 *
 * A display is an SDL window, a renderer, possibly some textures, and also a
 * state for the drawing functions.
 *
 * \sa oshu_open_display
 * \sa oshu_close_display
 */
struct oshu_display {
	struct SDL_Window *window;
	struct SDL_Renderer *renderer;
	/**
	 * The current view, used to project coordinates when drawing.
	 *
	 * When the window is resized, make sure you reset it with
	 * #oshu_reset_view and recreate your view from it.
	 */
	struct oshu_view view;
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
 * *display* is left in an unspecified state.
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
