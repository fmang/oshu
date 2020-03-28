/**
 * \file beatmap/helpers.cc
 * \ingroup beatmap
 *
 * \brief
 * Collection of helper functions for beatmaps.
 */

#include "beatmap/beatmap.h"

double oshu::hit_end_time(struct oshu::hit *hit)
{
	if (hit->type & oshu::SLIDER_HIT)
		return hit->time + hit->slider.duration * hit->slider.repeat;
	else
		return hit->time;
}

oshu::point oshu::end_point(struct oshu::hit *hit)
{
	if (hit->type & oshu::SLIDER_HIT)
		return oshu::path_at(&hit->slider.path, hit->slider.repeat);
	else
		return hit->p;
}
