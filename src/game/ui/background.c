/**
 * \file game/background.c
 * \ingroup game_ui
 */

#include "game/game.h"

#include <assert.h>

int oshu_load_background(struct oshu_game *game)
{
	if (game->beatmap.background_filename)
		return oshu_load_texture(&game->display, game->beatmap.background_filename, &game->ui.background.picture);
	else
		return 0;
}

/**
 * Draw a background image.
 *
 * Scale the texture to the window's size, while preserving the aspect ratio.
 * When the aspects don't match, crop the picture to ensure the window is
 * filled.
 */
static void draw_background(struct oshu_display *display, struct oshu_texture *pic)
{
	SDL_Rect dest;
	int ww, wh;
	SDL_GetWindowSize(display->window, &ww, &wh);
	double window_ratio = (double) ww / wh;
	double pic_ratio = oshu_ratio(pic->size);

	if (window_ratio > pic_ratio) {
		/* the window is too wide */
		dest.w = ww;
		dest.h = dest.w / pic_ratio;
		dest.x = 0;
		dest.y = (wh - dest.h) / 2;
	} else {
		/* the window is too high */
		dest.h = wh;
		dest.w = dest.h * pic_ratio;
		dest.y = 0;
		dest.x = (ww - dest.w) / 2;
	}
	SDL_RenderCopy(display->renderer, pic->texture, NULL, &dest);
}

/**
 * Draw the background, adjusting the brightness.
 *
 * Most of the time, the background will be displayed at 25% of its luminosity,
 * so that hit objects are clear.
 *
 * During breaks, the background is shown at full luminosity. The variation
 * show in the following graph, where *S* is the end time of the previous note
 * and the start of the break, and E the time of the next note and the end of
 * the break.
 *
 * ```
 * 100% ┼      ______________
 *      │     /              \
 *      │    /                \
 *      │___/                  \___
 *  25% │
 *      └──────┼────────────┼─────┼─> t
 *      S     S+2s         E-2s   E
 * ```
 *
 * A break must have a duration of at least 6 seconds, ensuring the animation
 * is never cut in between, or that the background stays lit for less than 2
 * seconds.
 *
 */
void oshu_show_background(struct oshu_game *game)
{
	if (!game->ui.background.picture.texture)
		return;
	assert (game->hit_cursor->previous != NULL);
	double break_start = oshu_hit_end_time(oshu_previous_hit(game));
	double break_end = oshu_next_hit(game)->time;
	double now = game->clock.now;
	double ratio = 0.;
	if (break_end - break_start > 6.) {
		if (now < break_start + 1.)
			ratio = 0.;
		else if (now < break_start + 2.)
			ratio = now - (break_start + 1.);
		else if (now < break_end - 2.)
			ratio = 1.;
		else if (now < break_end - 1.)
			ratio = 1. - (now - (break_end - 2.));
		else
			ratio = 0.;

	}
	int mod = 64 + ratio * 191;
	assert (mod >= 0);
	assert (mod <= 255);
	SDL_SetTextureColorMod(game->ui.background.picture.texture, mod, mod, mod);
	draw_background(&game->display, &game->ui.background.picture);
}

void oshu_free_background(struct oshu_game *game)
{
	oshu_destroy_texture(&game->ui.background.picture);
}
