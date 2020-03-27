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
class html_escape {
public:
	explicit html_escape(const char*);
	explicit html_escape(const std::string&);
	friend std::ostream& operator<<(std::ostream&, const html_escape&);
private:
	const char *data;
};

/**
 * Generate an HTML listing of a list of beatmap sets.
 */
void generate_html_beatmap_set_listing(const std::vector<oshu::beatmap_set>&, std::ostream&);

/** } */

}
