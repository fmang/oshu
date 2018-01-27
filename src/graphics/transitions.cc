/**
 * \file graphics/transitions.cc
 * \ingroup graphics_transitions
 */

#include "graphics/transitions.h"

#include <assert.h>

double oshu_fade_out(double start, double end, double t)
{
	assert (end > start);
	if (t < start)
		return 1;
	else if (t < end)
		return (end - t) / (end - start);
	else
		return 0;
}

double oshu_fade_in(double start, double end, double t)
{
	assert (end > start);
	if (t < start)
		return 0;
	else if (t < end)
		return (t - start) / (end - start);
	else
		return 1;
}

double oshu_trapezium(double start, double end, double transition, double t)
{
	double ratio = 0.;
	if (t <= start)
		ratio = 0.;
	else if (t < start + transition)
		ratio = (t - start) / transition;
	else if (t <= end - transition)
		ratio = 1.;
	else if (t < end)
		ratio = (end - t) / transition;
	else
		ratio = 0.;
	return ratio;
}
