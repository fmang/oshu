/**
 * \file beatmap/helpers.cc
 * \ingroup beatmap
 *
 * \brief
 * Collection of helper functions for beatmaps.
 */

#include "beatmap/beatmap.h"
#include <assert.h>
#include <limits>

double oshu::hit_end_time(oshu::hit *hit)
{
	if (hit->type & oshu::SLIDER_HIT)
		return hit->time + hit->slider.duration * hit->slider.repeat;
	else
		return hit->time;
}

oshu::point oshu::end_point(oshu::hit *hit)
{
	if (hit->type & oshu::SLIDER_HIT)
		return oshu::path_at(&hit->slider.path, hit->slider.repeat);
	else
		return hit->p;
}

double oshu::score(oshu::beatmap *beatmap)
{
	double score = 0, total = 0;
	auto leniency = beatmap->difficulty.leniency;

	for (oshu::hit *hit = beatmap->hits; hit; hit = hit->next) {
		switch (hit->state) {
		case oshu::GOOD_HIT:
			if (hit->offset < - leniency / 2)
				score += 1.0/3;
			else if (hit->offset > leniency / 2)
				score += 1.0/3;
			else
				score += 1.0;
			[[fallthrough]];
		case oshu::MISSED_HIT:
			++total;
			break;
		}
	}

	score /= total;
	if (total == 0) {
		if (!std::isnan(score))
			score = std::numeric_limits<double>::quiet_NaN();
	}
	else
		assert ((0 <= score) && (score <= 1));
	return score;
}
