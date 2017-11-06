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

oshu_vector oshu_normalize(oshu_vector p)
{
	double norm = cabs(p);
	if (norm < epsilon)
		return 0;
	return p / norm;
}

double oshu_distance2(oshu_point p, oshu_point q)
{
	double dx = creal(p - q);
	double dy = cimag(p - q);
	return dx * dx + dy * dy;
}

double oshu_distance(oshu_point p, oshu_point q)
{
	return cabs(p - q);
}

/* Bézier *********************************************************************/

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
static void bezier_map(struct oshu_bezier *path, double *t, int *degree, oshu_point **control_points)
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
static oshu_point bezier_at(struct oshu_bezier *path, double t)
{
	int degree;
	oshu_point *points;
	bezier_map(path, &t, &degree, &points);
	oshu_point p = 0;
	for (int i = 0; i <= degree; i++) {
		double bin = fac[degree] / (fac[i] * fac[degree - i]);
		double factor = bin * pow(t, i) * pow(1 - t, degree - i);
		p += factor * points[i];
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
static oshu_vector bezier_derive(struct oshu_bezier *path, double t)
{
	oshu_vector d = 0;
	int degree;
	oshu_point *points;
	bezier_map(path, &t, &degree, &points);
	for (int i = 0; i <= degree - 1; i++) {
		double bin = fac[degree - 1] / (fac[i] * fac[degree - 1 - i]);
		double factor = bin * pow(t, i) * pow(1 - t, degree - 1 - i);
		d += factor * (points[i + 1] - points[i]);
	}
	d *= degree;
	return d;
}

/**
 * Grow a Bézier path.
 *
 * Concretely, this functions adds a new linear segment at the end. The dynamic
 * arrays are grown to host that segment, then it is filled with the new
 * segment.
 *
 * The start point of the segment is the same as the last point of the original
 * path for continuity. The next point is computed by computing the derivative
 * vector at the end of the path (t=1), then scaling it so that its length is
 * equal to the *extension* argument, and finally adding that vector to the
 * previous point.
 *
 * The memory reallocation implies the memory must have been dynamically
 * allocated. You won't be able to grow a static path.
 */
static int grow_bezier(struct oshu_bezier *bezier, double extension)
{
	oshu_log_debug("expanding the bezier path by %f pixels", extension);
	oshu_vector direction = oshu_normalize(bezier_derive(bezier, 1.));
	if (direction == 0.) {
		oshu_log_warning("cannot grow a bezier path whose end is stationary");
		return -1;
	}

	assert (bezier->segment_count >= 1);
	bezier->segment_count++;
	bezier->indices = realloc(bezier->indices, (bezier->segment_count + 1) * sizeof(*bezier->indices));
	assert (bezier->indices != NULL);
	bezier->indices[bezier->segment_count] = bezier->indices[bezier->segment_count - 1] + 2;

	size_t end = bezier->indices[bezier->segment_count];
	assert (end >= 3);
	bezier->control_points = realloc(bezier->control_points, end * sizeof(*bezier->control_points));
	assert (bezier->control_points != NULL);
	bezier->control_points[end - 2] = bezier->control_points[end - 3];
	bezier->control_points[end - 1] = bezier->control_points[end - 2] + direction * extension;
	return 0;
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
	double length;

begin:
	/* 2. Compute the length of the path. */
	length = 0;
	oshu_point prev = bezier->control_points[0];
	for (int i = 0; i <= n; i++) {
		double t = (double) i / n;
		oshu_point current = bezier_at(bezier, t);
		length += oshu_distance(prev, current);
		l[i] = length;
		prev = current;
	}
	if (length + 5. < target_length) {
		if (grow_bezier(bezier, target_length - length) >= 0)
			goto begin;
	}
	if (length < target_length)
		/* ignore the rounding errors */
		target_length = length;

	/* 3. Deduce the l-coordinates. */
	assert (length > 0);
	for (int i = 0; i <= n; i++)
		l[i] /= target_length;

	/* 4. Set up the anchors. */
	int i = 0;
	int num_anchors = sizeof(bezier->anchors) / sizeof(*bezier->anchors);
	for (int j = 0; j < num_anchors; j++) {
		double my_l = (double) j / (num_anchors - 1);
		while (!(my_l <= l[i + 1]) && i < n - 1)
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

/* Lines **********************************************************************/

/**
 * Simple weighted average of the starting point and end point.
 */
static oshu_point line_at(struct oshu_line *line, double t)
{
	return (1 - t) * line->start + t * line->end;
}

/**
 * Derive the equation in #line_at to get the derivative.
 */
static oshu_vector line_derive(struct oshu_line *line, double t)
{
	return - line->start + line->end;
}

static void normalize_line(struct oshu_line *line, double target_length)
{
	double actual_length = oshu_distance(line->start, line->end);
	assert (target_length > 0);
	assert (actual_length > 0);
	double factor = target_length / actual_length;
	line->end = line->start + (line->end - line->start) * factor;
}

/* Perfect circle arcs ********************************************************/

static oshu_point arc_at(struct oshu_arc *arc, double t)
{
	double angle = (1 - t) * arc->start_angle + t * arc->end_angle;
	return arc->center + arc->radius * cexp(angle * I);
}

/**
 * Naive derivative of #arc_at.
 */
static oshu_vector arc_derive(struct oshu_arc *arc, double t)
{
	double angle = (1 - t) * arc->start_angle + t * arc->end_angle;
	double deriv_angle = - arc->start_angle + arc->end_angle;
	return arc->radius * deriv_angle * I * cexp(angle * I);
}

/**
 * Compute the center of the circle defined by 3 points.
 *
 * This code is inspired by the official osu! client's source code.
 */
int arc_center(oshu_point a, oshu_point b, oshu_point c, oshu_point *center)
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

	*center = (s * a + t * b + u * c) / sum;
	return 0;
}

int oshu_build_arc(oshu_point a, oshu_point b, oshu_point c, struct oshu_arc *arc)
{
	if (arc_center(a, b, c, &arc->center))
		return -1;

	arc->radius = oshu_distance(a, arc->center);
	arc->start_angle = carg(a - arc->center);
	arc->end_angle = carg(c - arc->center);
	double cross = cimag(conj(c - a) * (b - a));

	if (cross < 0 && arc->start_angle > arc->end_angle)
		arc->end_angle += 2. * M_PI;
	else if (cross > 0 && arc->start_angle < arc->end_angle)
		arc->end_angle -= 2. * M_PI;
	return 0;
}

static void normalize_arc(struct oshu_arc *arc, double target_length)
{
	double target_angle = target_length / arc->radius;
	double diff = copysign(target_angle, arc->end_angle - arc->start_angle);
	arc->end_angle = arc->start_angle + diff;
}

/* Generic interface **********************************************************/

void oshu_normalize_path(struct oshu_path *path, double length)
{
	switch (path->type) {
	case OSHU_LINEAR_PATH:
		return normalize_line(&path->line, length);
	case OSHU_PERFECT_PATH:
		return normalize_arc(&path->arc, length);
	case OSHU_BEZIER_PATH:
		return normalize_bezier(&path->bezier, length);
	default:
		return;
	}
}

oshu_point oshu_path_at(struct oshu_path *path, double t)
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
	return 0;
}

oshu_vector oshu_path_derive(struct oshu_path *path, double t)
{
	oshu_vector d;
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

	return revert ? -d : d;
}
