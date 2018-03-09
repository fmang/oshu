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
	os << "<li><a href=\"" << entry.path << "\">" << entry << "</a></li>";
}

static void generate_set(const beatmap_set &set, std::ostream &os)
{
	os << "<article>";
	for (const beatmap_entry &entry : set.entries)
		generate_entry(entry, os);
	os << "</article>";
}

void generate_beatmap_set_listing(const std::vector<beatmap_set> &sets, std::ostream &os)
{
	os << head;
	for (const beatmap_set &set : sets)
		generate_set(set, os);
}

}}}
