/**
 * \file game/metadata.c
 * \ingroup game_widgets
 */

#include "../config.h"

#include "game/game.h"
#include "game/screens/screens.h"
#include "graphics/draw.h"
#include "graphics/paint.h"

#include <assert.h>
#include <pango/pangocairo.h>

static const double padding = 10;

static PangoLayout* setup_layout(struct oshu_painter *p)
{
	cairo_set_operator(p->cr, CAIRO_OPERATOR_SOURCE);

	PangoLayout *layout = pango_cairo_create_layout(p->cr);
	pango_layout_set_width(layout, PANGO_SCALE * (creal(p->size) - 2. * padding));
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_spacing(layout, 5 * PANGO_SCALE);

	PangoFontDescription *desc = pango_font_description_from_string("Sans Bold 12");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	return layout;
}

static int paint_stars(struct oshu_game *game)
{
	oshu_size size = 360 + 60 * I;
	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	PangoLayout *layout = setup_layout(&p);

	char *sky = " ★ ★ ★ ★ ★ ★ ★ ★ ★ ★";
	int stars = game->beatmap.difficulty.overall_difficulty;
	assert (stars >= 0);
	assert (stars <= 10);
	int star_length = strlen(sky) / 10;
	char *difficulty = strndup(sky, star_length * stars);

	struct oshu_metadata *meta = &game->beatmap.metadata;
	const char *version = meta->version;
	assert (version != NULL);
	char *text;
	asprintf(&text, "%s\n%s", version, difficulty);
	assert (text != NULL);

	int width, height;
	pango_layout_set_text(layout, text, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
	pango_layout_get_size(layout, &width, &height);
	cairo_set_source_rgba(p.cr, 1, 1, 1, .5);
	cairo_move_to(p.cr, padding, (cimag(size) - height / PANGO_SCALE) / 2.);
	pango_cairo_show_layout(p.cr, layout);

	g_object_unref(layout);
	free(text);
	free(difficulty);

	struct oshu_texture *texture = &game->ui.metadata.stars;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = creal(size);
	return rc;
}

static int paint_metadata(struct oshu_game *game, int unicode)
{
	oshu_size size = 640 + 60 * I;
	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	PangoLayout *layout = setup_layout(&p);

	struct oshu_metadata *meta = &game->beatmap.metadata;
	const char *title = unicode ? meta->title_unicode : meta->title;
	const char *artist = unicode ? meta->artist_unicode : meta->artist;
	char *text;
	asprintf(&text, "%s\n%s", title, artist);
	assert (text != NULL);

	int width, height;
	pango_layout_set_text(layout, text, -1);
	pango_layout_get_size(layout, &width, &height);
	cairo_set_source_rgba(p.cr, 1, 1, 1, 1);
	cairo_move_to(p.cr, padding, (cimag(size) - height / PANGO_SCALE) / 2.);
	pango_cairo_show_layout(p.cr, layout);

	g_object_unref(layout);
	free(text);

	struct oshu_texture *texture = unicode ? &game->ui.metadata.unicode : &game->ui.metadata.ascii;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = 0;
	return rc;
}

/**
 * \todo
 * Handle errors.
 */
int oshu_paint_metadata(struct oshu_game *game)
{
	struct oshu_metadata *meta = &game->beatmap.metadata;
	int title_difference = meta->title && meta->title_unicode && strcmp(meta->title, meta->title_unicode);
	int artist_difference = meta->artist && meta->artist_unicode && strcmp(meta->artist, meta->artist_unicode);
	paint_metadata(game, 0);
	if (title_difference || artist_difference)
		paint_metadata(game, 1);

	paint_stars(game);
	return 0;
}

/**
 * Display the metadata on top of the gaming screen.
 *
 * Metadata are drawn in white text and a translucent black background for
 * readability.
 *
 * The display is shown when the game in paused, and in the 6 first seconds of
 * the game. It fades out from the 5th second to the 6th, where it becomes
 * completely invisible.
 *
 * Every 3.5 second, the display is switched between Unicode and ASCII, with a
 * 0.2-second fade transititon.
 *
 * \todo
 * Don't check the screen here, but instead take a parameter, like when the
 * frame should disappear.
 */
void oshu_show_metadata(struct oshu_game *game)
{
	double ratio;
	if (game->screen == &oshu_pause_screen)
		ratio = 1.;
	else if (game->clock.system < 5.)
		ratio = 1.;
	else if (game->clock.system < 6.)
		ratio = 6. - game->clock.system;
	else
		return;

	SDL_Rect frame = {
		.x = 0,
		.y = 0,
		.w = creal(game->display.view.size),
		.h = cimag(game->ui.metadata.ascii.size),
	};
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 128 * ratio);
	SDL_SetRenderDrawBlendMode(game->display.renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(game->display.renderer, &frame);

	double phase = game->clock.system / 3.5;
	double progression = fabs(((phase - (int) phase) - 0.5) * 2.);
	int has_unicode = game->ui.metadata.unicode.texture != NULL;
	int unicode = has_unicode ? (int) phase % 2 == 0 : 0;
	double transition = 1.;
	if (progression > .9 && has_unicode)
		transition = 1. - (progression - .9) * 10.;

	struct oshu_texture *meta = unicode ? &game->ui.metadata.unicode : &game->ui.metadata.ascii;
	SDL_SetTextureAlphaMod(meta->texture, ratio * transition * 255);
	oshu_draw_texture(&game->display, meta, 0);

	SDL_SetTextureAlphaMod(game->ui.metadata.stars.texture, ratio * 255);
	oshu_draw_texture(&game->display, &game->ui.metadata.stars, creal(game->display.view.size));
}

void oshu_free_metadata(struct oshu_game *game)
{
	oshu_destroy_texture(&game->ui.metadata.ascii);
	oshu_destroy_texture(&game->ui.metadata.unicode);
	oshu_destroy_texture(&game->ui.metadata.stars);
}
