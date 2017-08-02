#include "oshu.h"

/**
 * Pre-computed factorial values, for speed.
 * We're never going to see 8th-order Bezier curve, right?
 */
static int fac[8] = {1, 1, 2, 6, 24, 120, 720, 5040};

struct oshu_point oshu_segment_at(struct oshu_segment *segment, double t)
{
	struct oshu_point p = {0, 0};
	int n = segment->size - 1;
	if (n >= 8) {
		oshu_log_error("%dth-order curves are not supported", n);
		return p;
	}
	/* explicit bezier curve definition */
	for (int i = 0; i <= n; i++) {
		double bin = fac[n] / (fac[i] * fac[n - i]);
		double factor = bin * pow(t, i) * pow(1 - t, n - i);
		p.x += factor * segment->points[i].x;
		p.y += factor * segment->points[i].y;
	}
	return p;
}

struct oshu_point oshu_normalize(struct oshu_point p)
{
	double norm = sqrt(p.x * p.x + p.y * p.y);
	p.x /= norm;
	p.y /= norm;
	return p;
}

struct oshu_point oshu_segment_derive(struct oshu_segment *segment, double t)
{
	struct oshu_point p = {0, 0};
	int n = segment->size - 1;
	if (segment->size >= 8) {
		oshu_log_error("%dth-order curves are not supported", n);
		return p;
	}
	for (int i = 0; i <= n - 1; i++) {
		double bin = fac[n - 1] / (fac[i] * fac[n - 1 - i]);
		double factor = bin * pow(t, i) * pow(1 - t, n - 1 - i);
		p.x += factor * (segment->points[i + 1].x - segment->points[i].x);
		p.y += factor * (segment->points[i + 1].y - segment->points[i].y);
	}
	p.x *= n;
	p.y *= n;
	return p;
}


struct oshu_point oshu_path_at(struct oshu_path *path, double t)
{
	double cursor = 0;
	double step;
	int i;
	for (i = 0; i < path->size - 1; i++) {
		step = path->segments[i].length / path->length;
		if (cursor + step > t)
			break;
		cursor += step;
	}
	double sub_t = (t - cursor) / step;
	return oshu_segment_at(&path->segments[i], sub_t);
}

struct oshu_point oshu_path_derive(struct oshu_path *path, double t)
{
	double cursor = 0;
	double step;
	int i;
	for (i = 0; i < path->size - 1; i++) {
		step = path->segments[i].length / path->length;
		if (cursor + step > t)
			break;
		cursor += step;
	}
	double sub_t = (t - cursor) / step;
	return oshu_segment_derive(&path->segments[i], sub_t);
}
