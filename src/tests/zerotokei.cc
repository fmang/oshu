#include "beatmap/beatmap.h"

#include <cstring>

#include <iostream>

int main()
{
	int failures = 0;
	int rc;
	oshu_beatmap b;
	rc = oshu_load_beatmap("Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Shining].osu", &b);
	if (rc < 0) {
		++failures;
		goto abort;
	}
	if (std::strcmp(b.metadata.title, "Zero Tokei (Short ver.)")) {
		std::cerr << "unexpected title: " << b.metadata.title << std::endl;
		++failures;
	}
	if (failures > 0)
		std::cerr << "Total: " << failures << " failed tests." << std::endl;
abort:
	oshu_destroy_beatmap(&b);
	return failures;
}
