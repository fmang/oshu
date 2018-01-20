/**
 * \file ui/score.h
 * \ingroup ui_score
 */

#pragma once

#include "graphics/texture.h"

struct oshu_display;

/**
 * \defgroup ui_score Score
 * \ingroup ui
 *
 * \brief
 * Show the game score and statistics.
 *
 * This widget is under construction. [Insert animated GIF from the 90s]
 *
 * \{
 */

struct oshu_score_widget {
	struct oshu_display *display;
	struct oshu_beatmap *beatmap;
	struct oshu_texture offset_graph;
};

int oshu_create_score_widget(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_score_widget *widget);
void oshu_show_score_widget(struct oshu_score_widget *widget);
void oshu_destroy_score_widget(struct oshu_score_widget *widget);

struct oshu_score_frame {
	struct oshu_display *display;
	struct oshu_beatmap *beatmap;
	int good;
	int bad;
};

int oshu_create_score_frame(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_score_frame *frame);
void oshu_show_score_frame(struct oshu_score_frame *frame, double opacity);
void oshu_destroy_score_frame(struct oshu_score_frame *frame);

/** \} */
