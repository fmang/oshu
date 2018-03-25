/**
 * \file include/ui/widget.h
 * \ingroup ui
 */

#pragma once

namespace oshu {
namespace ui {

/**
 * \ingroup ui
 * \{
 */

struct widget {
	virtual ~widget() = default;
	virtual void draw() = 0;
};

/** \} */

}}
