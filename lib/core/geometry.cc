/**
 * \file lib/core/geometry.cc
 * \ingroup core_geometry
 */

#include "core/geometry.h"

double oshu::ratio(oshu::size size)
{
	return std::real(size) / std::imag(size);
}
