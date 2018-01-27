/**
 * \file graphics/texture.c
 * \ingroup graphics_texture
 */

#include "graphics/texture.h"

#include "graphics/display.h"
#include "log.h"

#include <SDL2/SDL_image.h>

int oshu_load_texture(struct oshu_display *display, const char *filename, struct oshu_texture *texture)
{
	texture->texture = IMG_LoadTexture(display->renderer, filename);
	if (!texture->texture) {
		oshu_log_error("error loading image: %s", IMG_GetError());
		return -1;
	}
	texture->origin = 0;
	int tw, th;
	SDL_QueryTexture(texture->texture, NULL, NULL, &tw, &th);
	texture->size = tw + th * I;
	return 0;
}

void oshu_destroy_texture(struct oshu_texture *texture)
{
	if (texture->texture) {
		SDL_DestroyTexture(texture->texture);
		texture->texture = NULL;
	}
}

void oshu_draw_scaled_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p, double ratio)
{
	oshu_point top_left = oshu_project(&display->view, p - texture->origin * ratio);
	oshu_size size = texture->size * ratio * display->view.zoom;
	SDL_Rect dest = {
		.x = creal(top_left), .y = cimag(top_left),
		.w = creal(size), .h = cimag(size),
	};
	SDL_RenderCopy(display->renderer, texture->texture, NULL, &dest);
}

void oshu_draw_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p)
{
	oshu_draw_scaled_texture(display, texture, p, 1.);
}
