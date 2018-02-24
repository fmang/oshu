/**
 * \file graphics/transitions.h
 * \ingroup graphics_transitions
 */

#pragma once

/**
 * \defgroup graphics_transitions Transitions
 * \ingroup graphics
 *
 * \brief
 * Pure mathematical functions for visual transition effects.
 *
 * A transition is a function taking a time intervals and positions, and
 * returns a floating value between 0 and 1.
 *
 * The last argument is the current time, in whatever reference space you use.
 *
 * \{
 */

/**
 * Fading out function.
 *
 * *start* defines when the fade-out transition starts, and *end* when it ends.
 * *t* is the current time.
 *
 * The return value of this function is 1 when *start < t*, then decreases
 * linearily when *start < t < end*, and finally becomes 0 when *t > end*.
 * It is continous.
 *
 * Assumes *start < end*.
 *
 * ```
 * 1 ┼____
 *   │    \
 *   │     \
 *   │      \____
 * 0 └───┼───┼────> t
 *       S   E
 * ```
 *
 * \sa oshu_fade_in
 */
double oshu_fade_out(double start, double end, double t);

/**
 * Fading in function.
 *
 * It's pretty much the same as #oshu_fade_out, with the property that
 * `fade_in(s, e, t) = 1 - fade_out(s, e, t)`.
 *
 * Assumes *start < end*.
 *
 *
 * ```
 * 1 ┼       ____
 *   │      /
 *   │     /
 *   │____/
 * 0 └───┼───┼────> t
 *       S   E
 * ```
 */
double oshu_fade_in(double start, double end, double t);

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


/** \} */
