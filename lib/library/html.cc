/**
 * \file lib/library/html.cc
 * \ingroup library_html
 */

#include "config.h"

#include "library/html.h"

#include <iostream>
#include <unordered_map>

namespace oshu {
namespace library {
namespace html {

escape::escape(const char *str)
: data(str)
{
}

escape::escape(const std::string &str)
: data(str.c_str())
{
}

static std::unordered_map<char, std::string> escape_sequences = {
	{'<', "&lt;"},
	{'>', "&gt;"},
	{'&', "&amp;"},
	{'"', "&quot;"}, // for attributes
};

std::ostream& operator<<(std::ostream &os, const escape &e)
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
	os << "<li><a href=\"" << escape{entry.path} << "\">" << escape{entry.version} << "</a></li>";
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
	os << "<h3>" << escape{set.title} << "</h3><ul>";
	for (const beatmap_entry &entry : set.entries)
		generate_entry(entry, os);
	os << "</ul>";
}

void generate_beatmap_set_listing(const std::vector<beatmap_set> &sets, std::ostream &os)
{
	os << head;
	os << "<link rel=\"stylesheet\" href=\"" << escape{OSHU_WEB_DIRECTORY} << "/style.css\" />";
	std::string artist = std::string();
	bool first_set = true;
	for (const beatmap_set &set : sets) {
		if (set.artist.compare(artist) == 0)
			generate_set(set, os);
		else {
			// artist is different, close article if necessary and
			// create a new one
			if (first_set) {
				first_set = false;
				os << "<article>";
			} else
				os << "</article><article>";

			os << "<h2>" << escape{set.artist} << "</h2>";
			artist = set.artist;
			generate_set(set, os);
		}
	}
	if (!first_set)
		// if there was at least one set
		os << "</article>";
}

}}}
