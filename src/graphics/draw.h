/**
 * \file graphics/draw.h
 * \ingroup draw
 */

#pragma once

#include "beatmap/geometry.h"

struct oshu_display;
struct oshu_texture;

/**
 * \defgroup draw Draw
 * \ingroup graphics
 *
 * \brief
 * Collection of drawing primitives.
 *
 * This module provides an interface to SDL's drawing routine, and any module
 * external to the graphics module should rely on these primitives rather than
 * call SDL directly.
 *
 * Unlike SDL's routine, this module takes into account the logical coordinate
 * systems of the display.
 *
 * It is okay if some routines are specific to one game mode, but any routine
 * the can be expressed using primitives defined here without using SDL's
 * interface could be moved to its game module instead.
 *
 * \{
 */

/**
 * Draw a 1-pixel aliased stroke following the path.
 */
void oshu_draw_path(struct oshu_display *display, struct oshu_path *path);

/**
 * Draw a thick stroke following the path.
 *
 * Actually draws two more-or-less parallel lines. Follow the curve nicely but
 * might make ugly loops when the path is too... loopy.
 */
void oshu_draw_thick_path(struct oshu_display *display, struct oshu_path *path, double width);

/**
 * Draw a regular polyline that should look like a circle.
 */
void oshu_draw_circle(struct oshu_display *display, oshu_point center, double radius);

/**
 * Draw one line, plain and simple.
 */
void oshu_draw_line(struct oshu_display *display, oshu_point p1, oshu_point p2);

/**
 * Draw a background image.
 *
 * Scale the texture to the window's size, while preserving the aspect ratio.
 * When the aspects don't match, crop the picture to ensure the window is
 * filled.
 */
void oshu_draw_background(struct oshu_display *display, struct oshu_texture *pic);

/**
 * Draw a texture at the specified position.
 *
 * *p* points at the position of the *origin* of the texture, not at the top
 * left corner.
 *
 * The size of the texture is always preserved.
 */
void oshu_draw_texture(struct oshu_display *display, oshu_point p, struct oshu_texture *texture);

/** \} */
