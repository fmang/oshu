/**
 * \file graphics/draw.h
 * \ingroup draw
 */

#include "beatmap/beatmap.h"
#include "graphics/display.h"

/**
 * \defgroup draw Draw
 * \ingroup graphics
 *
 * \brief
 * Collection of drawing primitives.
 *
 * This module provides an interface to SDL's drawing routine, and any module
 * external to the #graphics module should rely on these primitives rather than
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
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 *
 * \todo
 * Since it's too specific to the Osu! mode, it should be moved in the future.
 *
 * `now` is the current position in the playing song, in seconds.
 */
void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, double now);

/**
 * Draw a hit object.
 *
 * \todo
 * Since it's too specific to the Osu! mode, it should be moved in the future.
 */
void oshu_draw_hit(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now);

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
void oshu_draw_circle(struct oshu_display *display, struct oshu_point center, double radius);

/**
 * Draw one line, plain and simple.
 */
void oshu_draw_line(struct oshu_display *display, struct oshu_point p1, struct oshu_point p2);

/** \} */
