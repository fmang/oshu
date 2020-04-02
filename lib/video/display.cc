/**
 * \file video/display.cc
 * \ingroup video_display
 */

#include "video/display.h"

#include "core/log.h"

#include <SDL2/SDL.h>

/**
 * Read the OSHU_WINDOW_SIZE environment if defined, and falls back on the
 * default 960×720 resolution if the variable is invalid or absent.
 *
 * The value of the variable follows the format `WxH` where W is the width in
 * pixels, and H the height in pixels. The lowest resolution is 320x240.
 *
 * Usually, the format should be 4:3 to match the game, but it's perfectly fine
 * to have a wider window. It's just that the extra width will be nothing but
 * margin. It may fit the background better though.
 */
static oshu::size get_default_window_size()
{
	int width, height;
	char *value = getenv("OSHU_WINDOW_SIZE");
	if (!value || !*value) /* null or empty */
		goto def;
	/* Width */
	char *x;
	width = strtol(value, &x, 10);
	if (*x != 'x')
		goto invalid;
	/* Height */
	char *end;
	height = strtol(x + 1, &end, 10);
	if (*end != '\0')
		goto invalid;
	/* Putting it together */
	if (width < 320 || height < 240) {
		oshu_log_warning("the minimal default window size is 320x240");
		goto invalid;
	} else if (width > 3480 || height > 2160) {
		oshu_log_warning("it's unlikely you have a screen bigger than 4K");
		goto invalid;
	} else {
		return oshu::size(width, height);
	}
invalid:
	oshu_log_warning("rejected OSHU_WINDOW_SIZE value %s, defaulting to 960x720", value);
def:
	return oshu::size{960, 720};
}

/**
 * Return the enabled visual features reading the OSHU_QUALITY environment
 * variable.
 */
static int get_features()
{
	char *value = getenv("OSHU_QUALITY");
	if (!value || !*value) { /* null or empty */
		return oshu::DEFAULT_QUALITY;
	} else if (!strcmp(value, "high")) {
		return oshu::HIGH_QUALITY;
	} else if (!strcmp(value, "medium")) {
		return oshu::MEDIUM_QUALITY;
	} else if (!strcmp(value, "low")) {
		return oshu::LOW_QUALITY;
	} else {
		oshu_log_warning("invalid OSHU_QUALITY value: %s", value);
		oshu_log_warning("supported quality levels are: low, medium, high");
		return oshu::DEFAULT_QUALITY;
	}
}

/**
 * Open the window and create the rendered.
 *
 * The default window size, 960x720 is arbitrary but proportional the the game
 * area. It's just a saner default for most screens.
 */
static int create_window(oshu::display *display)
{
	display->features = get_features();
	oshu::size window_size = get_default_window_size();
	if (display->features & oshu::LINEAR_SCALING)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	display->frame_duration = 1. / ((display->features|oshu::SIXTY_FPS) ? 60. : 30.);
	display->window = SDL_CreateWindow(
		"oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		std::real(window_size), std::imag(window_size),
		SDL_WINDOW_RESIZABLE
	);
	if (display->window == NULL)
		goto fail;
	display->renderer = SDL_CreateRenderer(
		display->window, -1,
		(display->features & oshu::HARDWARE_ACCELERATION) ? 0 : SDL_RENDERER_SOFTWARE
	);
	if (display->renderer == NULL)
		goto fail;
	return 0;
fail:
	oshu_log_error("error creating the display: %s", SDL_GetError());
	return -1;
}

static void close_display(oshu::display *display)
{
	if (display->renderer) {
		SDL_DestroyRenderer(display->renderer);
		display->renderer = NULL;
	}
	if (display->window) {
		SDL_DestroyWindow(display->window);
		display->window = NULL;
	}
}

/**
 * The default window size is read from the OSHU_WINDOW_SIZE environment
 * variable. More details at #get_default_window_size.
 */
oshu::display::display()
{
	if (create_window(this) < 0) {
		close_display(this);
		throw std::runtime_error("could not open display");
	}
	oshu::reset_view(this);
}

oshu::display::~display()
{
	close_display(this);
}

oshu::point oshu::get_mouse(oshu::display *display)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return oshu::unproject(&display->view, oshu::point(x, y));
}
