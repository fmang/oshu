/**
 * \file video/texture.h
 * \ingroup video_texture
 */

#pragma once

#include "core/geometry.h"

struct SDL_Texture;

namespace oshu {

struct display;

/**
 * \defgroup video_texture Texture
 * \ingroup video
 *
 * \brief
 * Load and draw accelerated textures.
 *
 * This module provides a wrapper over SDL's texture rendering. Its main
 * benefit is that it integrates with an #oshu::display's view.
 *
 * Assume you need to draw a circle hit object at a given position in game
 * coordinates, this module will let you draw it by specifying only the center
 * the circle.
 *
 * Coordinate transformation takes 3 metadata into account:
 *
 * - The #oshu::display's view.
 * - The logical #oshu::texture::size of the texture.
 * - The texture's oshu::texture::origin.
 *
 * While the view may change every time the texture is drawn, the latter two
 * properties are well defined at the texture's creation, and thus need not be
 * specified every time.
 *
 * \{
 */

/**
 * Define a texture loaded on the GPU.
 *
 * Load it from a file with #oshu::load_texture or create it using the
 * \ref video_paint module, then destroy it with #oshu::destroy_texture.
 */
struct texture {
	/**
	 * The final size of the texture, specified in logical units.
	 *
	 * Consider a hit object texture of size 64×64 osu!pixels. If the view
	 * has a zoom factor of 2, it will appear as 128×128 pixels. If you
	 * want the texture to be clean, you would then load a 128×128 texture,
	 * rather than 64×64 texture.
	 *
	 * In the above scenario, 64×64 is the logical size of the texture,
	 * mentionned here in #size. The physical size of the texture is
	 * 128×128. The displayed size varies whenever the window is resized.
	 *
	 * When the displayed size is bigger than the physical size, the
	 * texture is upscaled and may appear blurry, so the bigger the
	 * physical size, the better, but the more expensive in resources.
	 *
	 * You are free to change this property at will, but nothing will
	 * prevent you from breaking the aspect ratio of the texture.
	 */
	oshu::size size = 0;
	/**
	 * The origin defines the anchor of the texture when drawing, rather
	 * than always using the top-left corner.
	 *
	 * Imagine you're representing a circle, you'd rather position it by
	 * its center rather than some imaginary top-left.
	 *
	 * When drawing a texture at (x, y), it will be drawn such that the
	 * #origin is at (x, y).
	 */
	oshu::point origin = 0;
	/**
	 * The underlying SDL texture.
	 */
	struct SDL_Texture *texture = nullptr;
};

/**
 * Load a texture using SDL2_image.
 *
 * The size of the texture is set to the physical size of the image, and its
 * origin is set to the top-left corner.
 *
 * Log an error and return -1 on failure.
 */
int load_texture(oshu::display *display, const char *filename, oshu::texture *texture);

/**
 * Destroy an SDL texture with `SDL_DestroyTexture`.
 *
 * Note that textures are linked to the renderer they were created for, so make
 * sure you delete the textures before the renderer.
 *
 * It is safe to destroy a texture more than once, or destroy a
 * null-initialized texture object.
 */
void destroy_texture(oshu::texture *texture);

/**
 * Draw a texture at the specified position.
 *
 * *p* points at the position of the *origin* of the texture, not at the
 * top-left corner. See #oshu::texture::origin.
 *
 * The logical size of the texture is preserved, according to the display's
 * current view.
 */
void draw_texture(oshu::display *display, oshu::texture *texture, oshu::point p);

/**
 * Draw a texture with a customizable scale factor.
 *
 * The resulting size of the texture is proportional to the ratio. A ratio of 1
 * means no scaling is performed. More than 1 and the texture is grown, less
 * than 1 and it is shrinked.
 *
 * The texture is scaled relative to its origin, preserving the property that
 * *p* always represents the same texture pixel (the origin), for any ratio.
 */
void draw_scaled_texture(oshu::display *display, oshu::texture *texture, oshu::point p, double ratio);

/** \} */

}
