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
	Beatmap;
	Audio;
	Audio -> Beatmap [style=dotted];
	Video -> Beatmap [style=dotted];
	Game -> Audio;
	Game -> Video;
	Game -> Beatmap;
	Osu -> Game;
	GUI -> Game;
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
drawing primitives. It is not directly related to the game, but uses the
geometric types defined by the beatmap module.

The \ref gui module provides user interface elements as widgets. Widgets are the
basic block for a composable user interface.

The \ref game module joins every module together and runs the main event loop
of the game. It watches the audio and the user's keyboard and mouse events to
manipulate the beatmap state, then schedules the drawing of the window. It is
agnostic to the game mode.

The \ref osu module implements the osu!standard game mode as an extension of
the \ref game module.
