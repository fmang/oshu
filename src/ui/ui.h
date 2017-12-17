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

/** \} */

#include "ui/cursor.h"
