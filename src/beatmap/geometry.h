/**
 * \file geometry.h
 * \ingroup geometry
 */

#pragma once

#include <complex.h>

/**
 * \defgroup geometry Geometry
 * \ingroup beatmap
 *
 * \brief
 * Bézier paths, among other things.
 *
 * \{
 */

/**
 * A point in a 2D space.
 *
 * The coordinate system is arbitrary.
 */
typedef double complex oshu_point;

/**
 * A 2D vector.
 *
 * This is distinct from #oshu_point for readability.
 */
typedef double complex oshu_vector;

/**
 * A 2D size.
 */
typedef double complex oshu_size;

/**
 * Return the width / height ratio of a size.
 */
double oshu_ratio(oshu_size size);

/**
 * A simple line, with a start point and end point.
 *
 * Used by #OSHU_LINEAR_PATH segments.
 *
 * It's formally the same as a degree 1 Bézier curve, but we'll keep that type
 * to make experiments.
 */
struct oshu_line {
	oshu_point start;
	oshu_point end;
};

/**
 * An arc, defined as a section of a circle.
 *
 * Used by #OSHU_PERFECT_PATH segments.
 *
 * In the beatmaps, these arc are called *perfect* and are defined with 3
 * points. The first point beging the start of the arc, the second one (called
 * the *grey* one in osu! terminology) is the point the arc must pass through,
 * and the last one is the end of the curve. It's therefore a section of the
 * circumscribed circle defined by the 3 points.
 *
 * Because it's hard to make computation with that kind of representation,
 * we'll use an easier representation made of the center of the circle, its
 * radius, and a pair of angles in radian where 0 is the the rightmost point,
 * like we do in common trigonometry.
 *
 * \sa oshu_build_arc
 */
struct oshu_arc {
	oshu_point center;
	double radius;
	double start_angle;
	double end_angle;
};

/**
 * \brief Compute an arc of circle from 3 points.
 *
 * See #oshu_arc to see how arcs are defined in oshu!.
 *
 * In beatmaps, the arcs are defined in a hard to manipulate way, so this
 * function in the geometry module is meant to help the parser generate arcs.
 *
 * Sometimes, the 3 points won't define a valid circle, for example when two
 * points are equal, or when the three points are aligned. In these case, one
 * might want to fallback on a linear path of something, but that's out of the
 * scope of this function.
 *
 * The first part in building the arc is finding the center. For the gory
 * details, see the implementation. Next is computing the radius and the
 * angles, but that's not even hard. Then, the last tricky point is finding on
 * which side of the circle the pass-through point is. While 3 points define a
 * circle, there are always two ways to go from any point A to any point B:
 * going clockwise or counter-clockwise. To know which side to take, we compute
 * the cross product of the vector AB and AC. Its sign tells whether B is on
 * the left side of AC, or on the right side. If it's on the left side (resp.
 * right side), that means the path goes clockwise (resp. counter-clockwise).
 * Then, to respect that direction in t-coordinates, we must ensure that
 * counter-clockwise (resp. clockwise) arcs have an end angle greater (resp.
 * lesser) than the start angle. If that's not the case, we add or subtract +2π
 * to one of the angles.
 *
 * \param a First point of the arc.
 * \param b Pass-through point of the arc.
 * \param c Last point of the arc.
 * \param arc Where to write the results. It doesn't have to be initialized,
 *            only allocated. Its content is unspecified on failure.
 *
 * \return 0 on success, -1 if the arc computation failed.
 */
int oshu_build_arc(oshu_point a, oshu_point b, oshu_point c, struct oshu_arc *arc);

/**
 * A Bézier path, made up of one or many Bézier segments.
 *
 * A Bézier segment is defined by its degree, and a number of control points
 * equal to its degree plus one.
 *
 * Used by #OSHU_BEZIER_PATH segments.
 *
 * For example, a degree 2 Bézier segment (called *quadratic*) has 3 control
 * points.
 *
 * Here's a complex slider to illustrate:
 * `166,250,154868,2,0,B|186:244|210:242|210:242|232:248|254:250|254:250|279:243|302:242,1,131.25,2|0,1:2|0:0,0:0:0:0:`.
 *
 * The `B|` part tells us it's a Bézier curve. You'll notice the point
 * `254:250` appear twice, meaning it's a *red point* according to the osu!
 * official doc. In better words, it's the end of one segment and the start of
 * the next one.
 *
 * We can therefore split the slider above in the following segments:
 *
 * 1. 166:250 (at the very beginning of the line), 186:244, 210:242. That
 *    makes a quadratic Bézier.
 * 2. 210:242, 232:248, 254:250.
 * 3. 254:250, 279:243, 302:242.
 *
 * The 3 segments joined together form a path.
 *
 * To make a continous path, the end point of a segment must be the same as
 * the beginning point of the next one.
 *
 * Bézier segments in oshu implement two coordinates systems. The first one is
 * the t-coordinate system, which is the one you see in the explicit definition
 * of the Bézier curves everywhere. The second one is more specific and meant
 * for oshu. Let's call it the l-coordinate system, where l ranges between 0
 * and 1, and represents how far the point is from the start. For example,
 * l=1/2 means that the point is at the middle of the curve, as opposed to
 * t=1/2 which means, for quadratic curves, that the point is closest to the
 * middle control point. See #oshu_normalize_path.
 */
struct oshu_bezier {
	/**
	 * How many segments the Bézier path contains.
	 */
	int segment_count;
	/**
	 * \brief Starting position of each segment in control points array.
	 *
	 * More concretely, the *n*th segment of the path starts at
	 * `control_points[indexes[n]]` and finishes at
	 * `control_points[indexes[n+1] - 1]`, where *0 ≤ n < segment_count*.
	 *
	 * This implies the *n*th segment has `indexes[n+1] - indexes[n]`
	 * points, and that its degree is `indexes[n+1] - indexes[n] - 1`.
	 *
	 * To reuse the example in #oshu_bezier, since all the segments are
	 * quadratic, the indices are [0, 3, 6, 9].
	 *
	 * The size of the indices array must be *segment_count + 1*.
	 *
	 * For the normalization process, it must be dynamically allocated.
	 *
	 * \sa segment_count
	 * \sa control_points
	 */
	int *indices;
	/**
	 * \brief The array of all control points, all segments combined.
	 *
	 * Each point belongs to exactly 1 segment, which means that for a
	 * continous multi-segment paths, the same point will be repeated more
	 * than once.
	 *
	 * The content of this array is precisely the control point list
	 * written in the beatmap file, duplicated comprised. Don't forget to
	 * add the very first point which is specified in the first two fields
	 * and not the point list.
	 *
	 * To identify the segments these points belong to, you need to use the
	 * #indices array.
	 *
	 * Its length is specified in `indices[segment_count]`.
	 *
	 * For the normalization process, it must be dynamically allocated.
	 */
	oshu_point *control_points;
	/**
	 * Translation map from l-coordinates to t-coordinates.
	 *
	 * Let's say we have n pieces, and n+1 anchors. Then for every 0 ≤ i ≤ n,
	 * `anchors[i]` holds the t-coordinate for the point at l = i / n.
	 *
	 * For any point such that i / n ≤ l ≤ (i + 1) / n, compute a weighted
	 * average between anchors[i] and anchors[i+1].
	 *
	 * \sa oshu_normalize_path
	 */
	double anchors[64];
};

/**
 * The curve types for a slider.
 *
 * Their value is the letter that appears in the beatmaps to identify the type.
 */
enum oshu_path_type {
	OSHU_LINEAR_PATH = 'L',
	OSHU_PERFECT_PATH = 'P',
	OSHU_BEZIER_PATH = 'B',
	OSHU_CATMULL_PATH = 'C',
};

/**
 * The linear paths (#OSHU_LINEAR_PATH) are the simplest and only have 2
 * points. See #oshu_line.
 *
 * Perfect paths (#OSHU_PERFECT_PATH) are bits of circle and have 3 points.
 * The 3 non-aligned points define a circle in a unique way. The perfect path
 * is the part of that circle that starts with the first point, passes through
 * the second point, and ends at the third point. See #oshu_arc.
 *
 * Bézier paths (#OSHU_BEZIER_PATH) have 2 to an arbitrary large number of
 * control points. A 2-point Bézier path is nothing but a linear path. A
 * 3-point Bézier path is a quadratic curve, also called a parabola. Things get
 * interesting with the 4-point cubic Bézier curve, which is the one you see in
 * most painting tools. See #oshu_bezier.
 *
 * Catmull paths (#OSHU_CATMULL_PATH) are officially deprecated, but we should
 * support them someday in order to support old beatmaps.
 */
struct oshu_path {
	enum oshu_path_type type;
	union {
		struct oshu_line line; /**< For #OSHU_LINEAR_PATH. */
		struct oshu_arc arc; /**< For #OSHU_PERFECT_PATH. */
		struct oshu_bezier bezier; /**< For #OSHU_BEZIER_PATH. */
	};
};

/**
 * Adjust the length-related properties of a path.
 *
 * The path described by its control points may not have the same length as the
 * wanted length specified as argument, in which case the path is shrinked or
 * expanded.
 *
 * In most case, this function will shrink the path, because the actual length
 * is greater than the one specified in the beatmap.
 */
void oshu_normalize_path(struct oshu_path *path, double length);

/**
 * Express the path in floating t-coordinates.
 *
 * t=0 is the starting point, t=1 is the end point. Calling this function n
 * times with t=k/n will give an approximation of the curve.
 *
 * All the segments are merged together in that [0, 1] segment according to
 * their lengths.
 *
 * The coordinates are further generalized by supporting any floating number.
 * Imagine a slider in your mind, one that repeats twice. You should see the
 * ball going from the starting point to the ending point, then back to the
 * starting back, and once again to the ending point. That behavior is modelled
 * in t-coordinates by making the [1, 2] range map to a revered [1, 0] range,
 * and then [2, 3] is the same as [0, 1].
 *
 * Thus, at(t=0) = at(t=2) = at(t=4), and at(t=1) = at(t=3) = at(t=5). In a
 * more general way, at(t + 2k) = at(t), making at 2-periodic, and at(t) =
 * at(-t), making it symetrical. These relations hold for any t in ℝ and k in
 * ℤ.
 *
 *
 * Let's graph it:
 *
 * ```
 * 1 ┼    .         .
 *   │   ╱ ╲       ╱ ╲       ╱
 *   │  ╱   ╲     ╱   ╲     ╱
 *   │ ╱     ╲   ╱     ╲   ╱
 *   │╱       ╲ ╱       ╲ ╱
 *   └────┼────┼────┼────┼────> t
 *  0     1    2    3    4
 * ```
 *
 * This behavior is obtained by taking the absolute value of the `remainder`,
 * defined in the C standard library, by 2.
 *
 */
oshu_point oshu_path_at(struct oshu_path *path, double t);

/**
 * Compute the smallest box such that the path fits in.
 *
 * It doesn't have a notion of path width, so you may need to push each edge
 * with the radius of your objects.
 *
 * The returned pair of points is such that:
 *
 * 1. ∀t real(top_left) ≤ real(at(t)) ≤ real(bottom_right)
 * 2. ∀t imag(top_left) ≤ imag(at(t)) ≤ imag(bottom_right)
 *
 */
void oshu_path_bounding_box(struct oshu_path *path, oshu_point *top_left, oshu_point *bottom_right);

/** \} */
