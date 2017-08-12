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

struct oshu_segment {
	enum oshu_curve_type type;
	/**
	 * The length of a segment is defined relative to the total length of
	 * the #oshu_path it is contained in.
	 *
	 * For example, a segment length of 10 inside a path of length 30 means
	 * the segment account for 1/3 of the path.
	 */
	double length;
	/**
	 * How many control points the segment has.
	 *
	 * \sa points
	 */
	int size;
	/**
	 * The array of control points.
	 *
	 * \sa size
	 */
	struct oshu_point *points;
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
 * A path represents many segments joined together.
 *
 * To make a continous path, the end point of a segment should be the same as
 * the beginning point of the next one.
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
