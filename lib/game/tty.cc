/**
 * \file game/tty.cc
 * \ingroup game_tty
 */

#include "game/tty.h"

#include "game/base.h"

#include <unistd.h>

void print_dual(const char *ascii, const char *unicode)
{
	if (ascii && unicode && strcmp(ascii, unicode))
		printf("  \033[33m%s\033[0m // %s\n", unicode, ascii);
	else if (unicode)
		printf("  \033[33m%s\033[0m\n", unicode);
	else if (ascii)
		printf("  \033[33m%s\033[0m\n", ascii);
	else
		printf("  Unknown\n");
}

void oshu::welcome(oshu::game_base *game)
{
	oshu::beatmap *beatmap = &game->beatmap;
	oshu::metadata *meta = &beatmap->metadata;
	printf("\n");
	print_dual(meta->title, meta->title_unicode);
	print_dual(meta->artist, meta->artist_unicode);
	if (meta->source)
		printf("  From %s\n", meta->source);

	printf("\n  \033[34m%s\033[0m\n", meta->version);
	if (meta->creator)
		printf("  By %s\n", meta->creator);

	int stars = beatmap->difficulty.overall_difficulty;
	double half_star = beatmap->difficulty.overall_difficulty - stars;
	printf("  ");
	for (int i = 0; i < stars; i++)
		printf("★ ");
	if (half_star >= .5)
		printf("☆ ");

	printf("\n\n");
}

/**
 * \todo
 * This function is called in too many locations.
 *
 * \todo
 * This function should be deleted once the window can display the time.
 */
void oshu::print_state(oshu::game_base *game)
{
	if (!isatty(fileno(stdout)))
		return;
	int minutes = game->clock.now / 60.;
	double seconds = game->clock.now - minutes * 60.;
	double duration = game->audio.music.duration;
	int duration_minutes = duration / 60.;
	double duration_seconds = duration - duration_minutes * 60;
	printf(
		"%s %d:%06.3f / %d:%06.3f\r",
		game->paused ? "Paused: " : "Playing:", minutes, seconds,
		duration_minutes, duration_seconds
	);
	fflush(stdout);
}

void oshu::congratulate(oshu::game_base *game)
{
	/* Clear the status line. */
	printf("\r                                        \r");
	double score = oshu::score(&game->beatmap);
        if (std::isnan(score)) return;

        int score_color = 0;
        if (score >= 0.9) score_color = 32;
        else if (score < 0.5) score_color = 31;
	printf(
		"  \033[1mScore: \033[%dm%3.2f\033[0m%%\n\n",
		score_color, score * 100);
}
