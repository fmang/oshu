/**
 * \file include/library/html.h
 * \ingroup library_html
 */

/**
 * \defgroup library_html HTML
 * \ingroup library
 *
 * \brief
 * Generate an HTML library browser.
 */

#pragma once

#include "library/beatmaps.h"

#include <iosfwd>
#include <vector>

namespace oshu {
namespace library {

/** \ingroup library_html */
namespace html {

/**
 * \ingroup library_html
 * \{
 */

/**
 * Wrap a string to escape HTML special characters.
 *
 * Use it like that:
 *
 * ```c++
 * os << escaped{"I <b>hax</b> you"};
 * ```
 *
 * The string object is taken by reference, so it must not be modified or
 * deleted as long as the #escape object is still alive.
 */
class escape {
public:
	explicit escape(const char*);
	explicit escape(const std::string&);
	friend std::ostream& operator<<(std::ostream&, const escape&);
private:
	const char *data;
};

/**
 * Generate an HTML listing of a list of beatmap sets.
 */
void generate_beatmap_set_listing(const std::vector<oshu::library::beatmap_set>&, std::ostream&);

/** } */

}}}
