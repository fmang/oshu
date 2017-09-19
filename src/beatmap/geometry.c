/**
 * \file geometry.c
 * \ingroup geometry
 */

#include "geometry.h"
#include "log.h"

#include <assert.h>

/**
 * When we get a value under this in our computation, we'll assume the value is
 * almost 0 and possibly trigger an error, depending on the context.
 */
static double epsilon = 0.1;

struct oshu_vector oshu_normalize(struct oshu_vector p)
{
	double norm = sqrt(p.x * p.x + p.y * p.y);
	p.x /= norm;
	p.y /= norm;
	return p;
}

double oshu_distance2(struct oshu_point p, struct oshu_point q)
{
	double dx = p.x - q.x;
	double dy = p.y - q.y;
	return dx * dx + dy * dy;
}

double oshu_distance(struct oshu_point p, struct oshu_point q)
{
	return sqrt(oshu_distance2(p, q));
}

/**
 * Split the [0, 1] range into n equal sub-ranges.
 *
 * For example, splitting [0, 1] in 2 will make [0, 1/2] and [1/2, 1].
 * In that case, if t=1/4, then it belongs to the first segment, and is at 1/2
 * in that segment's sub-range. So t becomes 1/2, and 0 is returned.
 *
 * Expects:
 * - 0 ≤ t ≤ 1
 * - n > 0
 *
 * Guarantees:
 * - 0 ≤ t ≤ 1
 * - 0 ≤ return value < n
 *
 * \sa bezier_map
 */
static int focus(double *t, int n)
{
	assert (n > 0);
	int segment = (int) (*t * n);
	assert (segment >= 0);
	assert (segment <= n);
	/* When t=1, we'd get segment = n */
	if (segment >= n)
		segment = n - 1;
	*t = (*t * n) - segment; /* rescale t */
	return segment;
}

/**
 * Pre-computed factorial values, for speed.
 * We're never going to see 8th-order Bezier curve, right?
 */
static int fac[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800};

/**
 * \brief Map global *t*-coordinates to segment-wise *t*-coordinates.
 *
 * \param path Read the Bézier path object.
 * \param t Read the path's *t*-coordinates and write the segment's.
 * \param degree Write the selected segment's degree.
 * \param control_points Write the address of the first point of the segment.
 *
 * Let's explain a bit how *t* is mapped. First of all, we assume each segment
 * has the same length, and have the same portion of the [0, 1] range for *t*.
 *
 * Let's illustrate with 4 segments, using the *t*-coordinates for the path:
 *
 * ```
 *                        t=0.625
 *                           ↓
 *  ├─────────┼─────────┼─────────┼─────────┤
 * t=0      t=1/4     t=1/2     t=3/4      t=1
 * ```
 *
 * When we multiply it by 4, we get the followed scaled *t*-coordinates:
 *
 * ```
 *                         t=2.5
 *                           ↓
 *  ├─────────┼─────────┼─────────┼─────────┤
 * t=0       t=1       t=2       t=3       t=4
 * ```
 *
 * With these scaled coordinates, we just take the integral part (or floor) and
 * get the segment's index. The floating part will range from 0 to 1 which is
 * just what we need.
 *
 * From the illustration, we see that 0.625 maps to the middle of the third
 * segment, which is what we expect.
 *
 * \sa focus
 */
static void bezier_map(struct oshu_bezier *path, double *t, int *degree, struct oshu_point **control_points)
{
	int segment = focus(t, path->segment_count);
	*degree = path->indices[segment+1] - path->indices[segment] - 1;
	assert (*degree < sizeof(fac) / sizeof(*fac));
	*control_points = path->control_points + path->indices[segment];
}

/**
 * Map a t coordinate to a point in a Bézier segment.
 */
static struct oshu_point segment_at(int degree, struct oshu_point *control_points, double t)
{
	struct oshu_point p = {0, 0};
	for (int i = 0; i <= degree; i++) {
		double bin = fac[degree] / (fac[i] * fac[degree - i]);
		double factor = bin * pow(t, i) * pow(1 - t, degree - i);
		p.x += factor * control_points[i].x;
		p.y += factor * control_points[i].y;
	}
	return p;
}

/**
 * Compute the position of a point expressed in *t*-coordinates.
 *
 * The coordinates are mapped with #bezier_map, and then we just apply the
 * standard explicit definition of Bézier curves.
 */
static struct oshu_point bezier_at(struct oshu_bezier *path, double t)
{
	int degree;
	struct oshu_point *points;
	bezier_map(path, &t, &degree, &points);
	return segment_at(degree, points, t);
}

static struct oshu_vector bezier_derive(struct oshu_bezier *path, double t)
{
	struct oshu_vector d = {0, 0};
	int degree;
	struct oshu_point *points;
	bezier_map(path, &t, &degree, &points);
	for (int i = 0; i <= degree - 1; i++) {
		double bin = fac[degree - 1] / (fac[i] * fac[degree - 1 - i]);
		double factor = bin * pow(t, i) * pow(1 - t, degree - 1 - i);
		d.x += factor * (points[i + 1].x - points[i].x);
		d.y += factor * (points[i + 1].y - points[i].y);
	}
	d.x *= degree;
	d.y *= degree;
	return d;
}

void oshu_normalize_bezier(struct oshu_bezier *bezier)
{
	int num_anchors = sizeof(bezier->anchors) / sizeof(*bezier->anchors);
	double length = 0;
	struct oshu_point prev = bezier->control_points[0];
	for (int i = 0; i < num_anchors; i++) {
		double t = (double) i / (num_anchors - 1);
		struct oshu_point current = bezier_at(bezier, t);
		length += oshu_distance(prev, current);
		bezier->anchors[i] = length;
		prev = current;
	}
	assert (length > 0);
	for (int i = 0; i < num_anchors; i++)
		bezier->anchors[i] /= length;
}

/**
 * Translate t coordinates with respect to the length of the various pieces of
 * the path.
 *
 * First, we have the naive global coordinates, where, if there are two
 * segments, the first one will receive [0, 1/2] and the second one [1/2, 1].
 *
 * Now, assume the first segment is twice as big as the second one, we'd like
 * to have a [0, 2/3] and [2/3, 1] share instead. This kind of problem arises
 * even inside a single segment, where the curve moves much faster on some
 * parts.
 *
 * Let's call the fair coordinates *normalized t-coordinates*.
 *
 * To handle this uniformly, #oshu_bezier::anchors contains all the length
 * information we'll need, disregarding segments. The whole path is split into
 * N pieces ranging from i/(N+1) to (i+1)/(N+1). These two ends are called
 * *anchors*, and the normalized t-coordinates of the anchors of the i'th piece
 * are available as `anchors[i]` to `anchors[i+1]`.
 *
 * \sa oshu_normalize_bezier
 */
static double normalize_t(struct oshu_bezier *bezier, double t)
{
	int num_anchors = sizeof(bezier->anchors) / sizeof(*bezier->anchors);
	for (int i = 1; i < num_anchors; ++i) {
		if (t <= bezier->anchors[i]) {
			assert (t >= bezier->anchors[i - 1]);
			double piece_length = bezier->anchors[i] - bezier->anchors[i - 1];
			assert (piece_length > 0);
			return (i - 1 + (t - bezier->anchors[i - 1]) / piece_length) / (num_anchors - 1);
		}
	}
	return 1.;
}

/**
 * Simple weighted average of the starting point and end point.
 */
static struct oshu_point line_at(struct oshu_line *line, double t)
{
	return (struct oshu_point) {
		.x = (1 - t) * line->start.x + t * line->end.x,
		.y = (1 - t) * line->start.y + t * line->end.y,
	};
}

/**
 * Derive the equation in #line_at to get the derivative.
 */
static struct oshu_vector line_derive(struct oshu_line *line, double t)
{
	return (struct oshu_vector) {
		.x = - line->start.x + line->end.x,
		.y = - line->start.y + line->end.y,
	};
}

static struct oshu_point arc_at(struct oshu_arc *arc, double t)
{
	double angle = (1 - t) * arc->start_angle + t * arc->end_angle;
	return (struct oshu_point) {
		.x = arc->center.x + arc->radius * cos(angle),
		.y = arc->center.y + arc->radius * sin(angle),
	};
}

/**
 * Naive derivative of #arc_at.
 */
static struct oshu_vector arc_derive(struct oshu_arc *arc, double t)
{
	double angle = (1 - t) * arc->start_angle + t * arc->end_angle;
	double deriv_angle = - arc->start_angle + arc->end_angle;
	return (struct oshu_vector) {
		.x = arc->radius * deriv_angle * (- sin(angle)),
		.y = arc->radius * deriv_angle * cos(angle),
	};
}

/**
 * Compute the center of the circle defined by 3 points.
 *
 * This code is inspired by the official osu! client's source code.
 */
int arc_center(struct oshu_point a, struct oshu_point b, struct oshu_point c, struct oshu_point *center)
{
	double a2 = oshu_distance2(b, c);
	double b2 = oshu_distance2(a, c);
	double c2 = oshu_distance2(a, b);
	if (a2 < epsilon || b2 < epsilon || c2 < epsilon)
		return -1;

	double s = a2 * (b2 + c2 - a2);
	double t = b2 * (a2 + c2 - b2);
	double u = c2 * (a2 + b2 - c2);
	double sum = s + t + u;
	if (abs(sum) < epsilon)
		return -1;

	center->x = (s * a.x + t * b.x + u * c.x) / sum;
	center->y = (s * a.y + t * b.y + u * c.y) / sum;
	return 0;
}

int oshu_build_arc(struct oshu_point a, struct oshu_point b, struct oshu_point c, struct oshu_arc *arc)
{
	if (arc_center(a, b, c, &arc->center))
		return -1;

	arc->radius = oshu_distance(a, arc->center);
	arc->start_angle = atan2(a.y - arc->center.y, a.x - arc->center.x);
	arc->end_angle = atan2(c.y - arc->center.y, c.x - arc->center.x);
	int cross = (c.x - a.x) * (b.y - a.y) - (b.x - a.x) * (c.y - a.y);
	if (cross < 0 && arc->start_angle > arc->end_angle)
		arc->end_angle += 2. * M_PI;
	else if (cross > 0 && arc->start_angle < arc->end_angle)
		arc->end_angle -= 2. * M_PI;
	return 0;
}

struct oshu_point oshu_path_at(struct oshu_path *path, double t)
{
	/* map t from ℝ to [0,1] */
	t = fabs(remainder(t, 2.));
	assert (-epsilon <= t && t <= 1 + epsilon);
	switch (path->type) {
	case OSHU_PATH_LINEAR:
		return line_at(&path->line, t);
	case OSHU_PATH_BEZIER:
		t = normalize_t(&path->bezier, t);
		return bezier_at(&path->bezier, t);
	case OSHU_PATH_PERFECT:
		return arc_at(&path->arc, t);
	case OSHU_PATH_CATMULL:
		assert (path->type != OSHU_PATH_CATMULL);
	default:
		assert (path->type != path->type);
	}
	return (struct oshu_point) {};
}

struct oshu_vector oshu_path_derive(struct oshu_path *path, double t)
{
	struct oshu_vector d;
	t = remainder(t, 2.);
	int revert = t < 0;
	t = fabs(t);
	assert (-epsilon <= t && t <= 1 + epsilon);

	switch (path->type) {
	case OSHU_PATH_LINEAR:
		d = line_derive(&path->line, t);
		break;
	case OSHU_PATH_BEZIER:
		t = normalize_t(&path->bezier, t);
		d = bezier_derive(&path->bezier, t);
		break;
	case OSHU_PATH_PERFECT:
		d = arc_derive(&path->arc, t);
		break;
	case OSHU_PATH_CATMULL:
		assert (path->type != OSHU_PATH_CATMULL);
	default:
		assert (path->type != path->type);
	}

	if (revert) {
		d.x = - d.x;
		d.y = - d.y;
	}
	return d;
}
