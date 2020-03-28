/**
 * \file video/texture.cc
 * \ingroup video_texture
 */

#include "video/texture.h"

#include "video/display.h"
#include "core/log.h"

#include <SDL2/SDL_image.h>

int oshu::load_texture(oshu::display *display, const char *filename, oshu::texture *texture)
{
	texture->texture = IMG_LoadTexture(display->renderer, filename);
	if (!texture->texture) {
		oshu_log_error("error loading image: %s", IMG_GetError());
		return -1;
	}
	texture->origin = 0;
	int tw, th;
	SDL_QueryTexture(texture->texture, NULL, NULL, &tw, &th);
	texture->size = oshu::size(tw, th);
	return 0;
}

void oshu::destroy_texture(oshu::texture *texture)
{
	if (texture->texture) {
		SDL_DestroyTexture(texture->texture);
		texture->texture = NULL;
	}
}

void oshu::draw_scaled_texture(oshu::display *display, oshu::texture *texture, oshu::point p, double ratio)
{
	oshu::point top_left = oshu::project(&display->view, p - texture->origin * ratio);
	oshu::size size = texture->size * ratio * display->view.zoom;
	SDL_Rect dest = {
		.x = (int) std::real(top_left), .y = (int) std::imag(top_left),
		.w = (int) std::real(size), .h = (int) std::imag(size),
	};
	SDL_RenderCopy(display->renderer, texture->texture, NULL, &dest);
}

void oshu::draw_texture(oshu::display *display, oshu::texture *texture, oshu::point p)
{
	oshu::draw_scaled_texture(display, texture, p, 1.);
}
