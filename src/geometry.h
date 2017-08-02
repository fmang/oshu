#pragma once

/**
 * @defgroup geometry Geometry
 *
 * @{
 */

struct oshu_point {
	int x;
	int y;
};

enum oshu_curve_type {
	OSHU_CURVE_BEZIER,
};

struct oshu_segment {
	enum oshu_curve_type type;
	/**
	 * The length of a segment is defined relative to the total length of
	 * the \ref oshu_path it is contained in.
	 *
	 * For example, a segment length of 10 inside a path of length 30 means
	 * the segment account for 1/3 of the path.
	 */
	double length;
	/** How many control points the segment has. */
	size_t size;
	struct oshu_point *points;
};

struct oshu_path {
	/**
	 * Length of the segment, in an arbitrary unit.
	 * The sum of the length of all its segment should be equal to this.
	 */
	double length;
	size_t size; /**< How many segments. */
	struct oshu_segment *segments;
};

/** @} */
