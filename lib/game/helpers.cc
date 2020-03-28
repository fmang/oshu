/**
 * \file game/helpers.cc
 * \ingroup game_helpers
 */

#include "game/base.h"

struct oshu::hit* oshu::look_hit_back(struct oshu::game_base *game, double offset)
{
	struct oshu::hit *hit = game->hit_cursor;
	double target = game->clock.now - offset;
	/* seek backward */
	while (oshu::hit_end_time(hit) > target)
		hit = hit->previous;
	/* seek forward */
	while (oshu::hit_end_time(hit) < target)
		hit = hit->next;
	/* here we have the guarantee that hit->time >= target */
	return hit;
}

struct oshu::hit* oshu::look_hit_up(struct oshu::game_base *game, double offset)
{
	struct oshu::hit *hit = game->hit_cursor;
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

struct oshu::hit* oshu::next_hit(struct oshu::game_base *game)
{
	struct oshu::hit *hit = game->hit_cursor;
	for (; hit->next; hit = hit->next) {
		if (hit->type & (oshu::CIRCLE_HIT | oshu::SLIDER_HIT))
			break;
	}
	return hit;
}

struct oshu::hit* oshu::previous_hit(struct oshu::game_base *game)
{
	struct oshu::hit *hit = game->hit_cursor;
	if (!hit->previous)
		return hit;
	for (hit = hit->previous; hit->previous; hit = hit->previous) {
		if (hit->type & (oshu::CIRCLE_HIT | oshu::SLIDER_HIT))
			break;
	}
	return hit;
}
