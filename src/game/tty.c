#include "game/game.h"
#include "game/screens/screens.h"
#include "game/tty.h"

#include <unistd.h>

void print_dual(const char *ascii, const char *unicode)
{
	if (ascii && unicode && strcmp(ascii, unicode))
		printf("  \033[33m%s\033[0m // %s\n", ascii, unicode);
	else if (unicode)
		printf("  \033[33m%s\033[0m\n", unicode);
	else if (ascii)
		printf("  \033[33m%s\033[0m\n", ascii);
	else
		printf("  Unknown\n");
}

void oshu_welcome(struct oshu_game *game)
{
	struct oshu_beatmap *beatmap = &game->beatmap;
	struct oshu_metadata *meta = &beatmap->metadata;
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

void oshu_print_state(struct oshu_game *game)
{
	if (!isatty(fileno(stdout)))
		return;
	int minutes = game->clock.now / 60.;
	double seconds = game->clock.now - minutes * 60.;
	double duration = game->audio.music.duration;
	int duration_minutes = duration / 60.;
	double duration_seconds = duration - duration_minutes * 60;
	printf(
		"%s: %d:%06.3f / %d:%06.3f\r",
		game->screen->name, minutes, seconds,
		duration_minutes, duration_seconds
	);
	fflush(stdout);
}

void oshu_congratulate(struct oshu_game *game)
{
	/* Clear the status line. */
	printf("\r                                        \r");
	/* Compute the score. */
	int good = 0;
	int missed = 0;
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->state == OSHU_MISSED_HIT)
			missed++;
		else if (hit->state == OSHU_GOOD_HIT)
			good++;
	}
	double rate = (double) good / (good + missed);
	printf(
		"  \033[1mScore:\033[0m\n"
		"  \033[%dm%3d\033[0m good\n"
		"  \033[%dm%3d\033[0m miss\n"
		"\n",
		rate >= 0.9 ? 32 : 0, good,
		rate < 0.5  ? 31 : 0, missed
	);
}
