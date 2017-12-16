/**
 * \file ui/ui.h
 * \ingroup ui
 */

#pragma once

#include "graphics/texture.h"

struct oshu_game;

/**
 * \defgroup ui UI
 * \ingroup ui
 *
 * \brief
 * Collection of GUI elements.
 *
 * \todo
 * Make the elements more generic. Take specific arguments instead of the full
 * game every time. Then it out of the game module.
 *
 * \{
 */

struct oshu_background_widget {
	struct oshu_texture picture;
};

int oshu_load_background(struct oshu_game *game);
void oshu_show_background(struct oshu_game *game);
void oshu_free_background(struct oshu_game *game);

struct oshu_metadata_widget {
	struct oshu_texture ascii;
	struct oshu_texture unicode;
	struct oshu_texture stars;
};

int oshu_paint_metadata(struct oshu_game *game);
void oshu_show_metadata(struct oshu_game *game);
void oshu_free_metadata(struct oshu_game *game);

void oshu_show_progression_bar(struct oshu_game *game);

struct oshu_score_widget {
	struct oshu_texture offset_graph;
};

int oshu_paint_score(struct oshu_game *game);
void oshu_show_score(struct oshu_game *game);
void oshu_free_score(struct oshu_game *game);

struct oshu_cursor_widget {
	/**
	 * Keep track of the previous positions of the mouse to display a
	 * fancier cursor, with a trail.
	 *
	 * This is a circular array, starting at #offset. The previous is at
	 * #offset - 1, and so on. When you reach the maximum index, wrap at 0.
	 */
	oshu_point history[4];
	/**
	 * Index of the most recent point in #history.
	 */
	int offset;
	/**
	 * The software mouse cursor.
	 */
	struct oshu_texture mouse;
};

int oshu_paint_cursor(struct oshu_game *game);
void oshu_show_cursor(struct oshu_game *game);
void oshu_free_cursor(struct oshu_game *game);

struct oshu_ui {
	struct oshu_background_widget background;
	struct oshu_metadata_widget metadata;
	struct oshu_score_widget score;
	struct oshu_cursor_widget cursor;
};

/** \} */
