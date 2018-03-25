liboshu {#liboshu}
=======

Modules
-------

liboshu is organized into various interconnected modules. For maximum clarity,
it is important the modules are not circularly linked to each other, and that
the responsibility of every module is clearly defined.

Below is the dependency graph of the modules. Dotted edges represent
undesirable dependencies, and should disappear in the near future.

\dot
digraph modules {
	rankdir=BT;
	node [shape=rect];

	# Module list
	Audio;
	Beatmap;
	Game;
	UI;
	Library;
	Video;

	# Dependencies
	Audio -> Beatmap [style=dotted];
	Game -> Audio;
	Game -> Beatmap;
	Game -> Video [style=dotted];
	UI -> Game;
	UI -> Video;
	Library -> Beatmap;
}
\enddot

The \ref core module provides generic modules used about everywhere in the
code. It's an implicit dependency of all the modules.

The \ref beatmap module handles the complete loading and parsing of the beatmap
files. It is self-contained. In particular, it is not going to load anything
audio or graphical. Since the beatmap is central to the game, most modules
depend on it for one of the structure it defines.

The \ref audio module handles everything audio-related from audio file loading
down to the actual output to the sound device.

The \ref video module handles the window creation and provides accelerated
drawing primitives. It is not directly related to the game in itself.

The \ref game module defines what a mode is, and implements the game mechanics,
reacting to input and controlling the audio playback along with updating the
beatmap.

The \ref ui module manages the main game window and forwards input to the game
module. It implements widgets for displaying the main game, and some other
visual items like the progress bar, software cursor, &c.

The \ref library module scans the filesystem for assets. It is currently only
used by the oshu-library tool, and not part of the main game.
