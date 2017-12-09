#include "game/game.h"

struct oshu_hit* oshu_look_hit_back(struct oshu_game *game, double offset)
{
	struct oshu_hit *hit = game->hit_cursor;
	double target = game->clock.now - offset;
	/* seek backward */
	while (oshu_hit_end_time(hit) > target)
		hit = hit->previous;
	/* seek forward */
	while (oshu_hit_end_time(hit) < target)
		hit = hit->next;
	/* here we have the guarantee that hit->time >= target */
	return hit;
}

struct oshu_hit* oshu_look_hit_up(struct oshu_game *game, double offset)
{
	struct oshu_hit *hit = game->hit_cursor;
	double target = game->clock.now + offset;
	/* seek forward */
	while (hit->time < target)
		hit = hit->next;
	/* seek backward */
	while (hit->time > target)
		hit = hit->previous;
	/* here we have the guarantee that hit->time <= target */
	return hit;
}
