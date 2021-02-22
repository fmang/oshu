/**
 * \file beatmap/path.cc
 * \ingroup beatmap_path
 */

#include "beatmap/path.h"

#include "core/log.h"

#include <assert.h>

/**
 * When we get a value under this in our computation, we'll assume the value is
 * almost 0 and possibly trigger an error, depending on the context.
 */
static double epsilon = 0.001;

/**
 * Extend a box so that the point *p* fits in.
 */
static void extend_box(oshu::point p, oshu::point *top_left, oshu::point *bottom_right)
{
	if (std::real(p) < std::real(*top_left))
		*top_left = oshu::point(std::real(p), std::imag(*top_left));
	if (std::imag(p) < std::imag(*top_left))
		*top_left = oshu::point(std::real(*top_left), std::imag(p));
	if (std::real(p) > std::real(*bottom_right))
		*bottom_right = oshu::point(std::real(p), std::imag(*bottom_right));
	if (std::imag(p) > std::imag(*bottom_right))
		*bottom_right = oshu::point(std::real(*bottom_right), std::imag(p));
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
static void bezier_map(oshu::bezier *path, double *t, int *degree, oshu::point **control_points)
{
	int segment = focus(t, path->segment_count);
	*degree = path->indices[segment+1] - path->indices[segment] - 1;
	*control_points = path->control_points + path->indices[segment];
}

/**
 * Compute the position of a point expressed in *t*-coordinates.
 *
 * The coordinates are mapped with #bezier_map, and then we just apply the
 * standard explicit definition of Bézier curves.
 *
 * Use de Casteljau's algorithm for numerical stability. I did come across
 * super high-degree Bézier curves on some beatmaps, and the factorial was at
 * its limits.
 */
static oshu::point bezier_at(oshu::bezier *path, double t)
{
	int degree;
	oshu::point *points;
	bezier_map(path, &t, &degree, &points);
	oshu::point pp[degree + 1];
	memcpy(pp, points, (degree + 1) * sizeof(*pp));

	/* l is the logical length of pp.
	 * It begins with all the points, and drops one point at every
	 * iteration. We stop when only 1 point is left */
	for (int l = (degree + 1); l > 1; --l) {
		for (int j = 0; j < (l - 1); ++j)
			pp[j] = (1. - t) * pp[j] + t * pp[j+1];
	}
	return pp[0];
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
static int grow_bezier(oshu::bezier *bezier, double extension)
{
	assert (bezier->segment_count >= 1);
	assert (bezier->indices != NULL);
	assert (bezier->control_points != NULL);
	size_t n = bezier->indices[bezier->segment_count];
	assert (n >= 2);
	oshu::point end = bezier->control_points[n - 1];
	oshu::point before_end = bezier->control_points[n - 2];

	oshu_log_debug("expanding the bezier path by %f pixels", extension);
	oshu::vector direction = end - before_end;
	if (std::abs(direction) < epsilon) {
		oshu_log_warning("cannot grow a bezier path whose end is stationary");
		return -1;
	}

	bezier->segment_count++;
	bezier->indices = (int*) realloc(bezier->indices, (bezier->segment_count + 1) * sizeof(*bezier->indices));
	bezier->indices[bezier->segment_count] = n + 2;

	bezier->control_points = (oshu::point*) realloc(bezier->control_points, (n + 2) * sizeof(*bezier->control_points));
	bezier->control_points[n] = end;
	bezier->control_points[n + 1] = end + direction / std::abs(direction) * extension;
	return 0;
}

/**
 * Approximate the length of the segment and set-up the l-coordinate system.
 *
 * Receives a Bézier path whose #oshu::bezier::segment_count,
 * #oshu::bezier::indices and #oshu::bezier::control_points are filled, and use
 * these data to compute the #oshu::bezier::anchors field.
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
 * 4. Now, let's compute #oshu::bezier::anchors.
 *    For every anchor index `j`, let `l = j / (# of anchors - 1)`, and find
 *    `i` such that `l_i ≤ l ≤ l_(i+1)`.
 *    Compute `k` such that `l = (1-k) * l_i + k * l_(i+1)`.
 *    Hint: `k = (l - l_i) / (l_(i+1) - l_i)`.
 *    Finally, let `anchors[j] = (1-k) * t_i + k * t_(i+1)`.
 *
 */
void normalize_bezier(oshu::bezier *bezier, double target_length)
{
	/* 1. Prepare the field. */
	int n = 64;  /* arbitrary */
	double l[n + 1];
	double length;

begin:
	/* 2. Compute the length of the path. */
	length = 0;
	oshu::point prev = bezier->control_points[0];
	for (int i = 0; i <= n; i++) {
		double t = (double) i / n;
		oshu::point current = bezier_at(bezier, t);
		length += std::abs(prev - current);
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
		double k = (l[i+1] != l[i]) ? (my_l - l[i]) / (l[i+1] - l[i]) : 0;
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
 * \sa normalize_bezier
 */
static double l_to_t(oshu::bezier *bezier, double l)
{
	int n = sizeof(bezier->anchors) / sizeof(*bezier->anchors) - 1;
	int i = focus(&l, n);
	return (1. - l) * bezier->anchors[i] + l * bezier->anchors[i + 1];
}

/**
 * Every point on a Bézier line is an average of all the control points, so
 * it's safe to compute the bounding box of the polyline defined by them.
 */
void bezier_bounding_box(oshu::bezier *bezier, oshu::point *top_left, oshu::point *bottom_right)
{
	assert (bezier->segment_count > 0);
	*top_left = *bottom_right = bezier->control_points[0];
	int count = bezier->indices[bezier->segment_count];
	for (int i = 0; i < count; ++i)
		extend_box(bezier->control_points[i], top_left, bottom_right);
}

/* Lines **********************************************************************/

/**
 * Simple weighted average of the starting point and end point.
 */
static oshu::point line_at(oshu::line *line, double t)
{
	return (1 - t) * line->start + t * line->end;
}

static void normalize_line(oshu::line *line, double target_length)
{
	double actual_length = std::abs(line->start - line->end);
	assert (target_length > 0);
	assert (actual_length > 0);
	double factor = target_length / actual_length;
	line->end = line->start + (line->end - line->start) * factor;
}

void line_bounding_box(oshu::line *line, oshu::point *top_left, oshu::point *bottom_right)
{
	*top_left = *bottom_right = line->start;
	extend_box(line->start, top_left, bottom_right);
	extend_box(line->end, top_left, bottom_right);
}

/* Perfect circle arcs ********************************************************/

static oshu::point arc_at(oshu::arc *arc, double t)
{
	double angle = (1 - t) * arc->start_angle + t * arc->end_angle;
	return arc->center + std::polar(arc->radius, angle);
}

/**
 * Compute the center of the circle defined by 3 points.
 *
 * This code is inspired by the official osu! client's source code.
 */
int arc_center(oshu::point a, oshu::point b, oshu::point c, oshu::point *center)
{
	double a2 = std::norm(b - c);
	double b2 = std::norm(a - c);
	double c2 = std::norm(a - b);
	if (a2 < epsilon || b2 < epsilon || c2 < epsilon)
		return -1;

	double s = a2 * (b2 + c2 - a2);
	double t = b2 * (a2 + c2 - b2);
	double u = c2 * (a2 + b2 - c2);
	double sum = s + t + u;
	if (fabs(sum) < epsilon)
		return -1;

	*center = (s * a + t * b + u * c) / sum;
	return 0;
}

int oshu::build_arc(oshu::point a, oshu::point b, oshu::point c, oshu::arc *arc)
{
	if (arc_center(a, b, c, &arc->center) < 0)
		return -1;

	arc->radius = std::abs(a - arc->center);
	arc->start_angle = std::arg(a - arc->center);
	arc->end_angle = std::arg(c - arc->center);
	double cross = std::imag(conj(c - a) * (b - a));

	if (cross < 0 && arc->start_angle > arc->end_angle)
		arc->end_angle += 2. * M_PI;
	else if (cross > 0 && arc->start_angle < arc->end_angle)
		arc->end_angle -= 2. * M_PI;
	return 0;
}

static void normalize_arc(oshu::arc *arc, double target_length)
{
	double target_angle = target_length / arc->radius;
	double diff = copysign(target_angle, arc->end_angle - arc->start_angle);
	arc->end_angle = arc->start_angle + diff;
}

/**
 * Compute the bounding box for an circle arc.
 *
 * First, normalize the edge angles, such that *min* is in the [0, 2π[
 * interval. Adjust max accordingly.
 *
 * Then, add the two extremities of the arc, and, for any side of the circle
 * reached, extend the box.
 */
void arc_bounding_box(oshu::arc *arc, oshu::point *top_left, oshu::point *bottom_right)
{
	double min = arc->start_angle < arc->end_angle ? arc->start_angle : arc->end_angle;
	double max = arc->start_angle > arc->end_angle ? arc->start_angle : arc->end_angle;
	while (min >= 2. * M_PI) {
		min -= 2. * M_PI;
		max -= 2. * M_PI;
	}
	while (min < 0) {
		min += 2. * M_PI;
		max += 2. * M_PI;
	}
	assert (min >= 0);
	assert (min < 2. * M_PI);
	assert (min <= max);

	*top_left = *bottom_right = arc_at(arc, 0.);
	extend_box(arc_at(arc, 1.), top_left, bottom_right);

	double angle = 0;
	oshu::vector radius = arc->radius;
	for (int i = 1; i < 8; ++i) {
		angle += M_PI / 2.;
		radius *= oshu::vector(0, 1);
		if (min < angle && angle < max)
			extend_box(arc->center + radius, top_left, bottom_right);
	}
}

/* Generic interface **********************************************************/

void oshu::normalize_path(oshu::path *path, double length)
{
	switch (path->type) {
	case oshu::LINEAR_PATH:
		return normalize_line(&path->line, length);
	case oshu::PERFECT_PATH:
		return normalize_arc(&path->arc, length);
	case oshu::BEZIER_PATH:
		return normalize_bezier(&path->bezier, length);
	default:
		return;
	}
}

oshu::point oshu::path_at(oshu::path *path, double t)
{
	/* map t from ℝ to [0,1] */
	t = fabs(remainder(t, 2.));
	assert (-epsilon <= t && t <= 1 + epsilon);
	switch (path->type) {
	case oshu::LINEAR_PATH:
		return line_at(&path->line, t);
	case oshu::BEZIER_PATH:
		t = l_to_t(&path->bezier, t);
		return bezier_at(&path->bezier, t);
	case oshu::PERFECT_PATH:
		return arc_at(&path->arc, t);
	case oshu::CATMULL_PATH:
		assert (path->type != oshu::CATMULL_PATH);
	default:
		assert (path->type != path->type);
	}
	return 0;
}

void oshu::path_bounding_box(oshu::path *path, oshu::point *top_left, oshu::point *bottom_right)
{
	switch (path->type) {
	case oshu::LINEAR_PATH:
		line_bounding_box(&path->line, top_left, bottom_right);
		break;
	case oshu::BEZIER_PATH:
		bezier_bounding_box(&path->bezier, top_left, bottom_right);
		break;
	case oshu::PERFECT_PATH:
		arc_bounding_box(&path->arc, top_left, bottom_right);
		break;
	case oshu::CATMULL_PATH:
		assert (path->type != oshu::CATMULL_PATH);
	default:
		assert (path->type != path->type);
	}
}
