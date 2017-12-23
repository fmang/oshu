/**
 * \file ui/background.h
 * \ingroup ui_background
 */

#pragma once

#include "graphics/texture.h"

struct oshu_display;

/**
 * \defgroup ui_background Background
 * \ingroup ui
 *
 * \brief
 * Picture background.
 *
 * \{
 */

/**
 * Define the game background.
 *
 * Today, only a static image is supported.
 *
 * Tomorrow, it could be a video, or a pattern, or something else.
 *
 * Depending on the game state, the background can be darkened to make the game
 * objects more visible.
 */
struct oshu_background {
	/**
	 * The display to which the background is attached.
	 *
	 * For best results, it should support color modulation, and linear
	 * scaling.
	 *
	 * \todo
	 * Implement the #OSHU_SHOW_BACKGROUND flag.
	 */
	struct oshu_display *display;
	/**
	 * The background picture.
	 *
	 * For best results, it should be big enough so that it's never
	 * upscaled. Its aspect ratio is ideally 4:3, matching the standard
	 * window size.
	 *
	 * When the ratio doesn't match, the picture is cropped.
	 *
	 * When no texture is loaded, #oshu_show_background is a no-op, so you
	 * can safely assume the background is a valid object.
	 */
	struct oshu_texture picture;
};

/**
 * Load a background picture with SDL2_image.
 *
 * You must free the background with #oshu_destroy_background.
 *
 * On error, returns -1, but the #oshu_background object remains safe to use
 * with #oshu_show_background and #oshu_destroy_background. It is therefore
 * safe to ignore errors here.
 *
 * \todo
 * Pre-scale the background at load time instead of forcing the GPU to scale a
 * huge texture everytime the game is painted.
 */
int oshu_load_background(struct oshu_display *display, const char *filename, struct oshu_background *background);

/**
 * Display the background such that it fills the whole screen.
 *
 * The size of the screen is determined from the display's current view, so you
 * should always call #oshu_reset_view before this function. Otherwise, expect
 * black bars or overflows.
 *
 * You should also fill the screen with a solid color in case the background
 * couldn't be loaded, because if the background picture is missing, nothing is
 * done.
 *
 * The brightness is a number in [0, 1] where 0 is the darkest variation of the
 * background, and 1 is the original picture. 0 is not an absolute zero, but a
 * subjective “anything below that and the background might as well not be
 * there” point.
 *
 * You may use #oshu_trapezium for the brightness to implement a fading-in
 * fading-out effect.
 */
void oshu_show_background(struct oshu_background *background, double brightness);

/**
 * Convex isosceles trapezoid function.
 *
 * This function draws a trapezoid from an arbitrary x-axis and a fixed [0, 1]
 * y-axis. This is meant to handle symmetrical transition effects.
 *
 * This trapezium function is defined such that:
 *
 * - It is continuous.
 * - Before *start*, its image is a constant 0.
 * - It increases linearily in [start, start+transition] from 0 to 1.
 * - Between [start+transition, end-transition], its image is a constant 1.
 * - It decreases linearily in [end, end-transition] from 1 to 0.
 * - After *end*, its image is a constant 0.
 *
 * It assumes that:
 *
 * 1. transition > 0
 * 2. start + transition < end - transition
 *
 *
 * ```
 * 1 ┼      __________
 *   │     /          \
 *   │    /            \
 *   │___/              \___
 * 0 └──┼───┼────────┼───┼───> t
 *      S  S+T      E-T  E
 * ```
 *
 * Because a transition with a very short stable 1 has little meaning, you
 * should check if the difference between start and end is big enough to
 * require a transition effect. Otherwise, you might as well skip the
 * transition.
 *
 * You are discouraged to use this function like a triangle with
 * `transition = (end - start) / 2`, because that won't reflect your intention.
 *
 */
double oshu_trapezium(double start, double end, double transition, double t);

/**
 * Free the background picture.
 */
void oshu_destroy_background(struct oshu_background *background);

/** \} */
