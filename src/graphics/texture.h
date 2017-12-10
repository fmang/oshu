/**
 * \file graphics/texture.h
 * \ingroup graphics_texture
 */

#pragma once

#include "beatmap/geometry.h"

struct oshu_display;
struct SDL_Texture;

/**
 * \defgroup graphics_texture Texture
 * \ingroup graphics
 *
 * \{
 */

/**
 * Define a texture loaded on the GPU.
 *
 * Load it with #oshu_load_texture or create it using #oshu_finish_painting,
 * then destroy it with #oshu_destroy_texture.
 */
struct oshu_texture {
	/**
	 * The final size of the texture, specified in logical units.
	 *
	 * Initially, it's the actual size of the texture in pixels, but
	 * typically 1 logical pixel is bigger than a phyiscal pixel, so the
	 * texture will be stretched.
	 *
	 * For example, you will create a 128×128 texture, and then force the
	 * size to 64×64 osu!pixels, which could appear as 96×96 pixels on
	 * screen. Better downscale than upscale, right?
	 */
	oshu_size size;
	/**
	 * The origin defines the anchor of the texture, rather than always using the
	 * top left. Imagine you're representing a circle, you'd rather position it by
	 * its center rather than some imaginary top-left. When drawing a texture at
	 * (x, y), it will be drawn such that the *origin* is at (x, y).
	 */
	oshu_point origin;
	/**
	 * The underlying SDL texture.
	 */
	struct SDL_Texture *texture;
};

/**
 * Load a texture using SDL2_image.
 *
 * Log an error and return -1 on failure.
 */
int oshu_load_texture(struct oshu_display *display, const char *filename, struct oshu_texture *texture);

/**
 * Destroy the SDL texture with `SDL_DestroyTexture`.
 *
 * Note that textures are linked to the renderer they were created for, so make
 * sure you delete the textures first.
 */
void oshu_destroy_texture(struct oshu_texture *texture);

/**
 * Draw a texture at the specified position.
 *
 * *p* points at the position of the *origin* of the texture, not at the top
 * left corner.
 *
 * The size of the texture is always preserved.
 */
void oshu_draw_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p);

/**
 * Draw a texture with a specific size.
 *
 * A ratio of 1 means no scaling is performed. More than 1 and the texture is
 * grown, less than 1 and it is shrinked.
 */
void oshu_draw_scaled_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p, double ratio);

/** \} */
