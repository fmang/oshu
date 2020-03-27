/**
 * \file lib/library/html.cc
 * \ingroup library_html
 */

#include "config.h"

#include "library/html.h"

#include <iostream>
#include <unordered_map>

namespace oshu {

html_escape::html_escape(const char *str)
: data(str)
{
}

html_escape::html_escape(const std::string &str)
: data(str.c_str())
{
}

static std::unordered_map<char, std::string> escape_sequences = {
	{'<', "&lt;"},
	{'>', "&gt;"},
	{'&', "&amp;"},
	{'"', "&quot;"}, // for attributes
};

std::ostream& operator<<(std::ostream &os, const html_escape &e)
{
	for (const char *c = e.data; *c; ++c) {
		auto i = escape_sequences.find(*c);
		if (i != escape_sequences.end())
			os << i->second;
		else
			os << *c;
	}
	return os;
}

static const char *head = R"html(
<!doctype html>
<meta charset="utf-8" />
<title>oshu! beatmaps listing</title>
<h1>oshu! beatmaps listing</h1>
)html";

static void generate_entry(const beatmap_entry &entry, std::ostream &os)
{
	os << "<li><a href=\"" << html_escape{entry.path} << "\">" << html_escape{entry.version} << "</a></li>";
}

/**
 * \todo
 * Generate links to the osu! website. Like https://osu.ppy.sh/beatmapsets/729191
 *
 * \todo
 * Add a picture. If the background images are too heavy, maybe we'll need to
 * resize them, and possibly also crop them for uniformity.
 */
static void generate_set(const beatmap_set &set, std::ostream &os)
{
	os << "<article>";
	os << "<h4>" << html_escape{set.artist} << " - " << html_escape{set.title} << "</h4><ul>";
	for (const beatmap_entry &entry : set.entries)
		generate_entry(entry, os);
	os << "</ul></article>";
}

void generate_html_beatmap_set_listing(const std::vector<beatmap_set> &sets, std::ostream &os)
{
	os << head;
	os << "<link rel=\"stylesheet\" href=\"" << html_escape{OSHU_WEB_DIRECTORY} << "/style.css\" />";
	for (const beatmap_set &set : sets)
		generate_set(set, os);
}

}
