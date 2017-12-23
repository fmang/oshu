/**
 * \file ui/metadata.c
 * \ingroup ui_metadata
 */

#include "../config.h"

#include "ui/metadata.h"

#include "beatmap/beatmap.h"
#include "graphics/display.h"
#include "graphics/paint.h"
#include "graphics/texture.h"

#include <assert.h>
#include <pango/pangocairo.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <stdlib.h>

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

static int paint_stars(struct oshu_metadata_frame *frame)
{
	oshu_size size = 360 + 60 * I;
	struct oshu_painter p;
	oshu_start_painting(frame->display, size, &p);
	PangoLayout *layout = setup_layout(&p);

	char *sky = " ★ ★ ★ ★ ★ ★ ★ ★ ★ ★";
	int stars = frame->beatmap->difficulty.overall_difficulty;
	assert (stars >= 0);
	assert (stars <= 10);
	int star_length = strlen(sky) / 10;
	char *difficulty = strndup(sky, star_length * stars);

	struct oshu_metadata *meta = &frame->beatmap->metadata;
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

	struct oshu_texture *texture = &frame->stars;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = creal(size);
	return rc;
}

static int paint_metadata(struct oshu_metadata_frame *frame, int unicode)
{
	oshu_size size = 640 + 60 * I;
	struct oshu_painter p;
	oshu_start_painting(frame->display, size, &p);
	PangoLayout *layout = setup_layout(&p);

	struct oshu_metadata *meta = &frame->beatmap->metadata;
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

	struct oshu_texture *texture = unicode ? &frame->unicode : &frame->ascii;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = 0;
	return rc;
}

/**
 * \todo
 * Handle errors.
 */
static int paint(struct oshu_metadata_frame *frame)
{
	struct oshu_metadata *meta = &frame->beatmap->metadata;
	int title_difference = meta->title && meta->title_unicode && strcmp(meta->title, meta->title_unicode);
	int artist_difference = meta->artist && meta->artist_unicode && strcmp(meta->artist, meta->artist_unicode);

	paint_metadata(frame, 0);
	if (title_difference || artist_difference)
		paint_metadata(frame, 1);

	paint_stars(frame);
	return 0;
}

int oshu_create_metadata_frame(struct oshu_display *display, struct oshu_beatmap *beatmap, double *clock, struct oshu_metadata_frame *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->display = display;
	frame->beatmap = beatmap;
	frame->clock = clock;
	return paint(frame);
}

void oshu_show_metadata_frame(struct oshu_metadata_frame *frame, double opacity)
{
	SDL_Rect back = {
		.x = 0,
		.y = 0,
		.w = creal(frame->display->view.size),
		.h = cimag(frame->ascii.size),
	};
	SDL_SetRenderDrawColor(frame->display->renderer, 0, 0, 0, 128 * opacity);
	SDL_SetRenderDrawBlendMode(frame->display->renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(frame->display->renderer, &back);

	double phase = *frame->clock / 3.5;
	double progression = fabs(((phase - (int) phase) - 0.5) * 2.);
	int has_unicode = frame->unicode.texture != NULL;
	int unicode = has_unicode ? (int) phase % 2 == 0 : 0;
	double transition = 1.;
	if (progression > .9 && has_unicode)
		transition = 1. - (progression - .9) * 10.;

	struct oshu_texture *meta = unicode ? &frame->unicode : &frame->ascii;
	SDL_SetTextureAlphaMod(meta->texture, opacity * transition * 255);
	oshu_draw_texture(frame->display, meta, 0);

	SDL_SetTextureAlphaMod(frame->stars.texture, opacity * 255);
	oshu_draw_texture(frame->display, &frame->stars, creal(frame->display->view.size));
}

double oshu_fade_out(double start, double end, double t)
{
	assert (end > start);
	if (t < start)
		return 1;
	else if (t < end)
		return (end - t) / (end - start);
	else
		return 0;
}

void oshu_destroy_metadata_frame(struct oshu_metadata_frame *frame)
{
	oshu_destroy_texture(&frame->ascii);
	oshu_destroy_texture(&frame->unicode);
	oshu_destroy_texture(&frame->stars);
}
