#include "oshu.h"

#include <SDL2/SDL.h>

int main(int argc, char **argv)
{
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
		return 1;
	}
	oshu_log_debug("successfully initialized");
	SDL_Quit();
	return 0;
}
