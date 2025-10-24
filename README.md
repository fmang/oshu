oshu!
=====

<p>
<a href="https://www.mg0.fr/oshu/screenshots/1.5/96neko%20-%20Aimai%20Elegy.png"><img alt="Screenshot #1" src="https://www.mg0.fr/oshu/screenshots/1.5/96neko%20-%20Aimai%20Elegy.thumb.jpg" height="128px" /></a>
<a href="https://www.mg0.fr/oshu/screenshots/1.5/Feint%20-%20We%20Won't%20Be%20Alone%20(feat.%20Laura%20Brehm).png"><img alt="Screenshot #2" src="https://www.mg0.fr/oshu/screenshots/1.5/Feint%20-%20We%20Won't%20Be%20Alone%20(feat.%20Laura%20Brehm).thumb.jpg" height="128px" /></a>
<a href="https://www.mg0.fr/oshu/screenshots/1.5/Rita%20-%20Life%20goes%20on.png"><img alt="Screenshot #3" src="https://www.mg0.fr/oshu/screenshots/1.5/Rita%20-%20Life%20goes%20on.thumb.jpg" height="128px" /></a>
<a href="https://www.mg0.fr/oshu/screenshots/1.5/TeddyLoid%20feat.%20Bonjour%20Suzuki%20-%20Pipo%20Password.png"><img alt="Screenshot #4" src="https://www.mg0.fr/oshu/screenshots/1.5/TeddyLoid%20feat.%20Bonjour%20Suzuki%20-%20Pipo%20Password.thumb.jpg" height="128px" /></a>
<a href="https://www.mg0.fr/oshu/screenshots/1.5/Mastermind(xi+nora2r)%20-%20Dreadnought.png"><img alt="Screenshot #5" src="https://www.mg0.fr/oshu/screenshots/1.5/Mastermind(xi+nora2r)%20-%20Dreadnought.thumb.jpg" height="128px" /></a>
</p>

Welcome to oshu!

This project was born out of frustration from not being able to play osu! on a
little Linux box with no graphics card whatsoever.  It's aimed at minimalistic
people and probably will never contain a hundredth of the official client's
features.

osu! is that PC clone of the osu! Tatakae! Ouendan DS rhythm game,
occidentalized as Elite Beat Agents. osu! has got tons of cool beatmaps, and a
more or less intuitive text file format described at
https://osu.ppy.sh/help/wiki/osu!_File_Formats

The main problem with osu! is that it required Windows and a graphics card.
Even with that, it sometimes get slow unless the computer is a beast… Hell, the
mania and taiko mode could be played in a terminal!

oshu! is currently mostly targeted at Linux users, but it doesn't have to be
that way. All it needs are volunteers to build and test the project on the
various operating systems and desktop environments out there.

By the way, the name comes from osu! but with the s pronounced sh, because it's
a mini-osu.


Getting started
---------------

First, you need to download beatmaps. The official beatmap source is
https://osu.ppy.sh/beatmapsets, but it requires that you create an account from
the official osu! game. If you don’t have an account, you may use a mirror like
https://bloodcat.com/osu/.

The beatmaps are distributed as *.osz* files, which are disguised ZIP files.
You need to extract using *unzip* or a similar tool. Rename them to *.zip* if
your file archiver can't process it. Once extracted, you'll see one or more
*.osu* files with various difficulty levels.

oshu! is meant to be started from the command-line, so go spawn your terminal
and run `oshu path/to/your/beatmap.osu`.

A window will open and you'll see circles appear on the screen. You got to
click the crosses when the orange circle reaches the white one. Hold the button
pressed for long notes. Most players point with the mouse and press the Z/X
keyboard keys instead of clicking.

For more detailed explainations, see:
https://www.youtube.com/results?search_query=osu+tutorial

You can use the experimental beatmap collection browser by extracting your
beatmaps to the `~/.oshu/beatmaps/` directory, the running
`oshu-library build-index`. Point your browser to the path it tells you and
start playing. Note that for your browser to open the beatmaps with oshu!, you
need to make sure the desktop integration is properly set up. See the section
below.


Install
-------

**Note:** oshu! is still in an early stage of development. Don't expect it to
be amazing. It doesn't even have a menu to select songs, you need to call it
from the command line like `$ oshu my_beatmap.osu`.

First of all, make sure you have the required dependencies. These things are so
common that I may assure you they're in your distribution's official
repositories.

- CMake 3.25,
- a C++14 compiler,
- pkg-config,
- SDL2 ≥ 2.0.5,
- SDL2_image 2.0.1,
- ffmpeg 3.3.6,
- cairo 1.14.8,
- pango 1.40.14.

Note that the versions specified above are indicative. It is very likely your
build will work with older or newer versions. If you need support for a
specific version, feel free to ask.

To build oshu!, follow the standard CMake procedure:

	mkdir build
	cd build
	cmake -DCMAKE_INSTALL_PREFIX=~/.local -DOSHU_DEFAULT_SKIN=osu ..
	make && make install

If you do use the `$HOME/.local` prefix, make sure you add `~/.local/bin` to
your PATH, or invoke oshu! by specifying the full path like
`~/.local/bin/oshu BEATMAP.osu`.

The `-DOSHU_DEFAULT_SKIN=osu` causes CMake to download the default osu! skin
and make it the default. If you're satisfied with the minimal skin, or if you
can't connect to Internet, you may leave out this option, or write out
explicitly `-DOSHU_DEFAULT_SKIN=minimal`.

If you want to install skins without making them the default, you can list them
like `-DOSHU_SKINS=minimal;osu`.

The files required for desktop integration are deployed with `make install`,
but you may need to refresh the cache databases yourself. Note that package
managers are usually good at refreshing them automatically.

You may force the refreshing with:

	update-desktop-database $PREFIX/share/applications
	update-mime-database $PREFIX/share/mime


Goals
-----

Let's clarify the goals before we stray too far away from them.

These things are the *raison d'être* of oshu:

- All the Osu beatmaps should be playable.
- The game should run on the lowest-end millenial computer one can find.

This means if you find an unsupported beatmap or a computer too slow to run
oshu, feel free to submit a bug report.

The first versions are going to focus on the regular game mode, and hopefully
oshu will support more game modes someday.

Since it's a hobby project, the feature set will be probably be quite limited
for a while. For example:

- No health bar.
- No spinner. (I hate those.)
- No network play.
- No video background.
- No fancy visual effects.

But it just means I probably won't have the time to work on these. These
limitations are *not* a design choice. Contributions are welcome!


Documentation
-------------

The user manual's man page is installed in the standard man directory, but you
may view it directly from `share/man/`.

For the technical documentation, please look at CONTRIBUTING.md.
