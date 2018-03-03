Project overview
================

Since oshu! is a rhythm game, it must feature an accurate audio engine on which
it would base its graphics and gameplay. Rather than synchronizing the audio to
the graphics, it relies entirely on the audio stream as a clock. In an ideal
world, this works just fine, but maybe someday someone will come up with a
weird audio stream that messes up the game. By the way, the temporal
information in a stream is usually based on metadata inside the packets. That
kind of information is not necessarily accurate. If it comes to the worst, the
clock should be based on the amount of samples played, where the sample count
divided by the frequency should yield the timestamp.

For maximum compatibility, the audio engine relies heavily on ffmpeg's
libavformat and libavcodec which play about everything. The audio device is
handled by SDL's standard audio module.

The graphics engine is based on SDL2's accelerated rendering API. The game's
visual assets are dynamically generated using the cairo vector graphics
library, and depending on the beatmap's configuration. To optimize the start-up
time, some textures are drawn on-the-fly when required, sometimes causing a
frame to be dropped. The frame rate is fixed at 60 FPS.


Code structure
--------------

\dot
digraph modules {
	rankdir=BT;
	node [shape=rect];
	Beatmap;
	subgraph {
		rank=same;
		Audio;
		UI -> Graphics;
	}
	Audio -> Beatmap;
	Graphics -> Beatmap;
	UI -> Beatmap;
	Game -> Audio;
	Game -> Graphics;
	Game -> Beatmap;
	Game -> UI;
	Osu -> Game;
	Main -> Osu;
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

The \ref graphics module handles the window creation and provides accelerated
drawing primitives. It is not directly related to the game, but uses the
geometric types defined by the beatmap module.

The \ref ui module provides user interface elements as widgets. Widgets are the
basic block for a composable user interface.

The \ref game module joins every module together and runs the main event loop
of the game. It watches the audio and the user's keyboard and mouse events to
manipulate the beatmap state, then schedules the drawing of the window. It is
agnostic to the game mode.

The \ref osu module implements the osu!standard game mode as an extension of
the \ref game module.

The *main* module \ref src/oshu/main.cc is the entry-point of oshu! and provides a
command-line interface to briefly configure the game, and then yields control
to the game module.


Error handling
--------------

Unless mentioned otherwise, the oshu functions that may fail return an int
which is either 0 on success, and a negative value on error. That negative
value will be -1 most of the time.

Error messages are logged with \ref oshu_log_error by the *callee*. This makes
the code clearer for the caller, and the callee has much more details about the
error than the caller.

With C++, more and more functions will starting exceptions rather than handling
errors manually.
