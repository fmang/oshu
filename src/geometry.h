/**
 * \file geometry.h
 * \ingroup geometry
 */

#pragma once

/**
 * \defgroup geometry Geometry
 * \ingroup beatmap
 *
 * \brief
 * Bézier paths, among other things.
 *
 * \{
 */

struct oshu_point {
	double x;
	double y;
};

/**
 * Normalize a vector, making its norm equal to 1.
 */
struct oshu_point oshu_normalize(struct oshu_point p);

/**
 * The curve types for a slider.
 *
 * Their value is the letter that appears in the beatmaps to identify the type.
 */
enum oshu_curve_type {
	OSHU_CURVE_LINEAR = 'L',
	OSHU_CURVE_PERFECT = 'P',
	OSHU_CURVE_BEZIER = 'B',
	OSHU_CURVE_CATMULL = 'C',
};

/**
 * A simple line, with a start point and end point.
 *
 * Used by #OSHU_CURVE_LINEAR segments.
 *
 * It's formally the same as a degree 1 Bézier curve, but we'll keep that type
 * to make experiments.
 */
struct oshu_line {
	struct oshu_point start;
	struct oshu_point end;
};

/**
 * An arc, defined as a section of a circle.
 *
 * Used by #OSHU_CURVE_PERFECT segments.
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
 */
struct oshu_arc {
	struct oshu_point center;
	double radius;
	double start_angle;
	double end_angle;
};

/**
 * A Bézier curve is defined by its degree, and a number of control points
 * equal to its degree plus one.
 *
 * Used by #OSHU_CURVE_BEZIER segments.
 *
 * For example, a degree 2 Bézier curve (called *quadratic*) has 3 control
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
 */
struct oshu_bezier {
	/**
	 * The degree of the Bézier curve, which is required to determine how
	 * many points the #control_points array contains. The answer is
	 * `degree + 1` control points.
	 */
	int degree;
	/**
	 * The array of control points, with a fixed size to simplify memory
	 * management.
	 *
	 * Rather than hardcoding the 8 constant when creating the segment, you
	 * should use `sizeof(control_points) / sizeof(*control_points)`.
	 */
	struct oshu_point control_points[8];
};

/**
 * A segment is a piece of a regular curve, be that curve linear or Bézier.
 *
 * The linear segments (#OSHU_CURVE_LINEAR) are the simplest and only have 2
 * points. See #oshu_line.
 *
 * Perfect segments (#OSHU_CURVE_PERFECT) are bits of circle and have 3 points.
 * The 3 non-aligned points define a circle in a unique way. The perfect
 * segment is the part of that circle that starts with the first point, passes
 * through the second point, and ends at the third point. See #oshu_arc.
 *
 * Bézier segments (#OSHU_CURVE_BEZIER) have 2 to an arbitrary large number of
 * control points. A 2-point Bézier segment is nothing but a linear segment. A
 * 3-point Bézier segment is a quadratic curve, also called a parabola. Things
 * get interesting with the 4-point cubic Bézier curve, which is the one you
 * see in most painting tools. See #oshu_bezier.
 *
 * Catmull segments (#OSHU_CURVE_CATMULL) are officially deprecated, but we
 * should support them someday in order to support old beatmaps.
 */
struct oshu_segment {
	enum oshu_curve_type type;
	/**
	 * The length of a segment is defined relative to the total length of
	 * the #oshu_path it is contained in.
	 *
	 * For example, a segment length of 10 inside a path of length 30 means
	 * the segment account for 1/3 of the path.
	 *
	 * \sa oshu_path
	 */
	double length;
	union {
		struct oshu_line line;
		struct oshu_arc arc;
		struct oshu_bezier bezier;
	};
};

/**
 * Express the segment in [0, 1] floating t-coordinates.
 *
 * You should probably use #oshu_path_at.
 *
 * All the segments are merged together in that [0, 1] segment according to
 * their lengths.
 */
struct oshu_point oshu_segment_at(struct oshu_segment *segment, double t);

/**
 * Return the derivative vector of the segment at point t in t-coordinates.
 */
struct oshu_point oshu_segment_derive(struct oshu_segment *segment, double t);

/**
 * A path represents many #oshu_segment joined together.
 *
 * To make a continous path, the end point of a segment should be the same as
 * the beginning point of the next one.
 *
 * The length of the segments are relative to each other, and the length of the
 * path must be the sum of the lengths of all its segments. The lengths are
 * essential to compute t-coordinates, so that t=1 is the end point, and t=0.5
 * is the middle of the whole path.
 *
 * Let's illustrate a bit with paths made up of 2 segments. The number over
 * each segment is the segment's length.
 *
 * ```
 *      10        10
 *  ├─────────┼─────────┤
 * t=0       t=.5      t=1
 *
 *                300                 100
 *  ├─────────────────────────────┼─────────┤
 * t=0                          t=.75      t=1
 * ```
 *
 */
struct oshu_path {
	/**
	 * Length of the segment, in an arbitrary unit.
	 * The sum of the length of all its segment should be equal to this.
	 */
	double length;
	int size; /**< How many segments. */
	struct oshu_segment *segments;
};

/**
 * Express the path in [0, 1] floating t-coordinates.
 *
 * t=0 is the starting point, t=1 is the end point. Calling this function n
 * times with t=k/n will give an approximation of the curve.
 *
 * All the segments are merged together in that [0, 1] segment according to
 * their lengths.
 */
struct oshu_point oshu_path_at(struct oshu_path *path, double t);

/**
 * Return the derivative vector of the path at point t in t-coordinates.
 */
struct oshu_point oshu_path_derive(struct oshu_path *path, double t);

/** \} */
