/**
 * \file include/core/geometry.h
 * \ingroup core_geometry
 */

#pragma once

#include <complex>

namespace oshu {

/**
 * \defgroup core_geometry Geometry
 * \ingroup core
 *
 * \brief
 * Common 2D types.
 *
 * \{
 */

/**
 * A point in a 2D space.
 *
 * The coordinate system is arbitrary.
 */
using point = std::complex<double>;

/**
 * A 2D vector.
 *
 * This is distinct from #oshu::point for readability.
 */
using vector = std::complex<double>;

/**
 * A 2D size.
 */
using size = std::complex<double>;

/**
 * Return the width / height ratio of a size.
 */
double ratio(oshu::size size);

/* } */

}
