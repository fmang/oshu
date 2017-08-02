#include "oshu.h"

/**
 * Pre-computed factorial values, for speed.
 * We're never going to see 8th-order Bezier curve, right?
 */
static int fac[8] = {1, 1, 2, 6, 24, 120, 720, 5040};

struct SDL_Point oshu_segment_at(struct oshu_segment *segment, double t)
{
	SDL_Point p = {0, 0};
	if (segment->size >= 8) {
		oshu_log_error("%dth-order curves are not supported", segment->size - 1);
		return p;
	}
	/* explicit bezier curve definition */
	double x = 0, y = 0;
	int n = segment->size - 1;
	for (int i = 0; i <= n; i++) {
		double bin = fac[n] / (fac[i] * fac[n - i]);
		double factor = bin * pow(t, i) * pow(1 - t, n - i);
		x += factor * segment->points[i].x;
		y += factor * segment->points[i].y;
	}
	p.x = x;
	p.y = y;
	return p;
}
