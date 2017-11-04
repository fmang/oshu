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
static double epsilon = 0.01;

struct oshu_vector oshu_normalize(struct oshu_vector p)
{
	double norm = sqrt(p.x * p.x + p.y * p.y);
	if (norm < epsilon)
		return (struct oshu_vector) {0, 0};
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
 * Split the [0, 1] range into n equal sub-segments.
 *
 * More concretely, compute and `*t * n`, write its fractional part in `*t`,
 * and return the integral part, while guaranteeing it doesn't exceed n - 1.
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
	assert (*t >= 0);
	assert (n > 0);
	int segment = (int) (*t * n);
	/* When t=1, we'd get segment = n */
	assert (segment <= n);
	segment = segment < n ? segment : n - 1;
	*t = (*t * n) - segment; /* rescale t */
	return segment;
}

/**
 * Pre-computed factorial values, for speed.
 * We're never going to see 8th-order Bezier curve, right?
 *
 * Don't add any more value here, it's going to cause numerical instabilities
 * because of overflows and float approximations.
 *
 * Use de Casteljau's algorithm is this is limiting.
 *
 * \sa bezier_at
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
 * With these scaled coordinates, we just take the integral part (or floor) to
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
 * Compute the position of a point expressed in *t*-coordinates.
 *
 * The coordinates are mapped with #bezier_map, and then we just apply the
 * standard explicit definition of Bézier curves.
 *
 * If the factorial gets too big and annoying, we'll switch to de Casteljau's
 * algorithm. However, since I'm not sure it would let us derive, maybe that's
 * gonna wait until we switch to a new Bézier drawing engine.
 */
static struct oshu_point bezier_at(struct oshu_bezier *path, double t)
{
	int degree;
	struct oshu_point *points;
	bezier_map(path, &t, &degree, &points);
	struct oshu_point p = {0, 0};
	for (int i = 0; i <= degree; i++) {
		double bin = fac[degree] / (fac[i] * fac[degree - i]);
		double factor = bin * pow(t, i) * pow(1 - t, degree - i);
		p.x += factor * points[i].x;
		p.y += factor * points[i].y;
	}
	return p;
}

/**
 * Compute the derivative of a Bézier curve in t-coordinates.
 *
 * In practice, it's going to be called with l-coordinates, so the result is
 * formally wrong. However, because both derivative vectors are collinear, and
 * because the resulting vector is normalized at the end anyway, it doesn't
 * matter.
 */
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

/**
 * Approximate the length of the segment and set-up the l-coordinate system.
 *
 * Receives a Bézier path whose #oshu_bezier::segment_count,
 * #oshu_bezier::indices and #oshu_bezier::control_points are filled, and use
 * these data to compute the #oshu_bezier::anchors field.
 *
 * Here are the steps of the normalization process:
 *
 * 1. Pick `n + 1` points `p_0, …, p_n` on the curve, with their t-coordinates
 *    `t_0, … t_n` such that `t_0 = 0`, `t_n = 1`, and for every i ≤ j,
 *    `t_i ≤ t_j`. The sanest choice is to take `t_i = i / n`.
 *
 * 2. For each point, compute its distance from the beginning, following the
 *    curve. `L_0 = 0` and `L_(i+1) = L_i + || p_(i+1) - p_i ||`.
 *    With this, `L_n` is the actual length of the path.
 *
 * 3. Deduce the l-coordinates of the points by normalizing the L-coordinates
 *    above such that `l_0 = 0` and `l_n = 1`: let `l_i = L_i / L`. Note that
 *    here, `L` is the *wanted* length, assuming `L ≤ L_n`. This implies
 *    `l_n ≥ 1`, which in turns implies that the final curve is cut, the
 *    desired effect.
 *
 * 4. Now, let's compute #oshu_bezier::anchors.
 *    For every anchor index `j`, let `l = j / (# of anchors - 1)`, and find
 *    `i` such that `l_i ≤ l ≤ l_(i+1)`.
 *    Compute `k` such that `l = (1-k) * l_i + k * l_(i+1)`.
 *    Hint: `k = (l - l_i) / (l_(i+1) - l_i)`.
 *    Finally, let `anchors[j] = (1-k) * t_i + k * t_(i+1)`.
 *
 */
void normalize_bezier(struct oshu_bezier *bezier, double target_length)
{
	/* 1. Prepare the field. */
	int n = 32;  /* arbitrary */
	double l[n + 1];

	/* 2. Compute the length of the path. */
	double length = 0;
	struct oshu_point prev = bezier->control_points[0];
	for (int i = 0; i <= n; i++) {
		double t = (double) i / n;
		struct oshu_point current = bezier_at(bezier, t);
		length += oshu_distance(prev, current);
		l[i] = length;
		prev = current;
	}
	if (length < target_length) {
		double delta = target_length - length;
		if (delta > length / 100.)
			oshu_log_warning("bezier slider is %f pixels short", delta);
		target_length = length;
	}

	/* 3. Deduce the l-coordinates. */
	assert (length > 0);
	for (int i = 0; i <= n; i++)
		l[i] /= target_length;

	/* 4. Set up the anchors. */
	int i = 0;
	int num_anchors = sizeof(bezier->anchors) / sizeof(*bezier->anchors);
	for (int j = 0; j < num_anchors; j++) {
		double my_l = (double) j / (num_anchors - 1);
		while (!(my_l <= l[i + 1]) && i < n)
			i++;
		assert (l[i] <= my_l);
		double k = (my_l - l[i]) / (l[i+1] - l[i]);
		bezier->anchors[j] = (1. - k) * i / n + k * (i + 1.) / n;
	}
}

/**
 * Translate l-coordinates to t-coordinates.
 *
 * First, the #focus converts our l such that we have
 * `l = (1 - k) * i / n + k * (i + 1) / n`.
 *
 * Then, we get the approximate t by applying a similar relation:
 * `t = (1 - k) * anchors[i] + k * anchors[i + 1]`.
 *
 * \sa oshu_normalize_bezier
 */
static double l_to_t(struct oshu_bezier *bezier, double l)
{
	int n = sizeof(bezier->anchors) / sizeof(*bezier->anchors) - 1;
	int i = focus(&l, n);
	return (1. - l) * bezier->anchors[i] + l * bezier->anchors[i + 1];
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

void oshu_normalize_path(struct oshu_path *path, double length)
{
	switch (path->type) {
	case OSHU_BEZIER_PATH:
		return normalize_bezier(&path->bezier, length);
	default:
		return;
	}
}

struct oshu_point oshu_path_at(struct oshu_path *path, double t)
{
	/* map t from ℝ to [0,1] */
	t = fabs(remainder(t, 2.));
	assert (-epsilon <= t && t <= 1 + epsilon);
	switch (path->type) {
	case OSHU_LINEAR_PATH:
		return line_at(&path->line, t);
	case OSHU_BEZIER_PATH:
		t = l_to_t(&path->bezier, t);
		return bezier_at(&path->bezier, t);
	case OSHU_PERFECT_PATH:
		return arc_at(&path->arc, t);
	case OSHU_CATMULL_PATH:
		assert (path->type != OSHU_CATMULL_PATH);
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
	case OSHU_LINEAR_PATH:
		d = line_derive(&path->line, t);
		break;
	case OSHU_BEZIER_PATH:
		t = l_to_t(&path->bezier, t);
		d = bezier_derive(&path->bezier, t);
		break;
	case OSHU_PERFECT_PATH:
		d = arc_derive(&path->arc, t);
		break;
	case OSHU_CATMULL_PATH:
		assert (path->type != OSHU_CATMULL_PATH);
	default:
		assert (path->type != path->type);
	}

	if (revert) {
		d.x = - d.x;
		d.y = - d.y;
	}
	return d;
}
