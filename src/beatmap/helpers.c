/**
 * \file beatmap/helpers.c
 * \ingroup beatmap
 *
 * \brief
 * Collection of helper functions for beatmaps.
 */

#include "beatmap/beatmap.h"

double oshu_hit_end_time(struct oshu_hit *hit)
{
	if (hit->type & OSHU_SLIDER_HIT && hit->slider.path.type)
		return hit->time + hit->slider.duration * hit->slider.repeat;
	else
		return hit->time;
}

struct oshu_point oshu_end_point(struct oshu_hit *hit)
{
	if (hit->type & OSHU_SLIDER_HIT && hit->slider.path.type)
		return oshu_path_at(&hit->slider.path, hit->slider.repeat);
	else
		return hit->p;
}
