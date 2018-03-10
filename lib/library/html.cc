/**
 * \file lib/library/html.cc
 * \ingroup library_html
 */

#include "library/html.h"

#include <iostream>

namespace oshu {
namespace library {
namespace html {

static const char *head = R"html(
<!doctype html>
<meta charset="utf-8" />
)html";

/**
 * \todo
 * Escape the metadata and the path.
 */
static void generate_entry(const beatmap_entry &entry, std::ostream &os)
{
	os << "<li><a href=\"" << entry.path << "\">" << entry.version << "</a></li>";
}

/**
 * \todo
 * Escape.
 *
 * \todo
 * Generate links to the osu! website. Like https://osu.ppy.sh/beatmapsets/729191
 */
static void generate_set(const beatmap_set &set, std::ostream &os)
{
	os << "<article>";
	os << "<h4>" << set.artist << " - " << set.title << "</h4><ul>";
	for (const beatmap_entry &entry : set.entries)
		generate_entry(entry, os);
	os << "</ul></article>";
}

void generate_beatmap_set_listing(const std::vector<beatmap_set> &sets, std::ostream &os)
{
	os << head;
	for (const beatmap_set &set : sets)
		generate_set(set, os);
}

}}}
