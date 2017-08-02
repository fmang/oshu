#include "oshu.h"

/**
 * Pre-computed factorial values, for speed.
 * We're never going to see 8th-order Bezier curve, right?
 */
static int fac[8] = {1, 1, 2, 6, 24, 120, 720, 5040};

struct SDL_Point oshu_segment_at(struct oshu_segment *segment, double t)
{
	SDL_Point p = {0, 0};
	double x = 0, y = 0;
	int n = segment->size - 1;
	if (n >= 8) {
		oshu_log_error("%dth-order curves are not supported", n);
		return p;
	}
	/* explicit bezier curve definition */
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

SDL_Point oshu_normalize(SDL_Point p)
{
	double norm = sqrt(p.x * p.x + p.y * p.y);
	p.x /= norm;
	p.y /= norm;
	return p;
}

struct SDL_Point oshu_segment_derive(struct oshu_segment *segment, double t)
{
	SDL_Point p = {0, 0};
	int n = segment->size - 1;
	double x = 0, y = 0;
	if (segment->size >= 8) {
		oshu_log_error("%dth-order curves are not supported", n);
		return p;
	}
	for (int i = 0; i <= n - 1; i++) {
		double bin = fac[n - 1] / (fac[i] * fac[n - 1 - i]);
		double factor = bin * pow(t, i) * pow(1 - t, n - 1 - i);
		x += factor * (segment->points[i + 1].x - segment->points[i].x);
		y += factor * (segment->points[i + 1].y - segment->points[i].y);
	}
	p.x = n * x;
	p.y = n * y;
	return p;
}
