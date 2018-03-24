/**
 * \file lib/core/geometry.cc
 * \ingroup core_geometry
 */

#include "core/geometry.h"

double oshu_ratio(oshu_size size)
{
	return std::real(size) / std::imag(size);
}
