/**
 * \file include/gui/widget.h
 * \ingroup gui
 */

#pragma once

namespace oshu {
namespace gui {

/**
 * \ingroup gui
 * \{
 */

struct widget {
	virtual ~widget() = default;
	virtual void draw() = 0;
};

/** \} */

}}
