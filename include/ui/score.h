/**
 * \file ui/score.h
 * \ingroup ui_score
 */

#pragma once

struct oshu_beatmap;
struct oshu_display;

/**
 * \defgroup ui_score Score
 * \ingroup ui
 *
 * \brief
 * Show the game score and statistics.
 *
 * It's currently extremly simple, showing a bar filled with green for good
 * notes, and red for bad notes.
 *
 * \{
 */

struct oshu_score_frame {
	struct oshu_display *display;
	struct oshu_beatmap *beatmap;
	int good;
	int bad;
};

/**
 * Create the frame and compute the score.
 *
 * The score is computed once and for all when this function is called.
 *
 * Maybe in a future version, this widget could be permanently shown and
 * updated on every action.
 */
int oshu_create_score_frame(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_score_frame *frame);

/**
 * Show the score frame.
 *
 * It's a simple bar, centered in the bottom of the screen.
 *
 * The opacity argument lets you fade in the bar with #oshu_fade_in.
 */
void oshu_show_score_frame(struct oshu_score_frame *frame, double opacity);

/**
 * Destroy a score frame.
 *
 * In fact, it does nothing, but is there for consistency.
 */
void oshu_destroy_score_frame(struct oshu_score_frame *frame);

/** \} */
