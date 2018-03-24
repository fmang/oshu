/**
 * \file include/core/geometry.h
 * \ingroup core_geometry
 */

#pragma once

#include <complex>

/**
 * \defgroup core_geometry Geometry
 * \ingroup core
 *
 * \brief
 * Common 2D types.
 */

/**
 * A point in a 2D space.
 *
 * The coordinate system is arbitrary.
 */
using oshu_point = std::complex<double>;

/**
 * A 2D vector.
 *
 * This is distinct from #oshu_point for readability.
 */
using oshu_vector = std::complex<double>;

/**
 * A 2D size.
 */
using oshu_size = std::complex<double>;

/**
 * Return the width / height ratio of a size.
 */
double oshu_ratio(oshu_size size);
