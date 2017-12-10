/**
 * \file game/ui/ui.h
 * \ingroup game_ui
 *
 * \todo
 * Rename this file ui.h
 *
 * \todo
 * Define the oshu_ui structure here.
 */

#pragma once

#include "graphics/texture.h"

struct oshu_game;

/**
 * \defgroup game_ui Widgets
 * \ingroup game
 *
 * \brief
 * Collection of GUI elements.
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

struct oshu_ui {
	struct oshu_background_widget background;
	struct oshu_metadata_widget metadata;
};

/** \} */
