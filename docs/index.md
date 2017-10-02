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

The current graphics engine is a prototype complete enough to draw the
essential elements of the game. It is based on SDL's rendering engine and
limited to circles and 1-pixel lines. This is extremely fast but really crude.
In the future, oshu! will most likely use the Cairo vector graphics library to
render all the objects that are going to be required. If it's fast enough, it
could be done dynamically, but at first everything would be generated and
loaded onto the GPU as textures when the beatmap is loaded, and in particular
the sliders. Using that kind of trick, we'll probably reach top performance and
pleasing graphics. To load the background, the simplest option is the
SDL2_image library, which is really lightweight and fits our needs.


Code structure
--------------

\dot
digraph modules {
	rankdir=BT;
	node [shape=rect];
	Beatmap;
	Audio -> Beatmap;
	Graphics -> Beatmap;
	Game -> Audio;
	Game -> Beatmap;
	Game -> Graphics;
	Main -> Game;
}
\enddot

The \ref log module provides a wrapper around SDL's logging facility and is
used about everywhere in the code. It's an implicit dependency of all the
modules.

The \ref audio module handles everything audio-related from audio file loading
down to the actual output to the sound device. It is self-contained. In
particular, it has no relation to the beatmap whatsoever.

The \ref beatmap module handles the complete loading and parsing of the beatmap
files. It is self-contained. In particular, it is not going to load anything
audio or graphical

The \ref graphics module handles the window creation and the drawing of the
game elements. It is able to browse a beatmap object to draw its elements on
the screen.

The \ref game module joins every module together and runs the main event loop
of the game. It watches the audio and the user's keyboard and mouse events to
manipulate the beatmap state, then schedules the drawing of the window.

The *main* module is the entry-point of oshu! and provides a command-line
interface to briefly configure the game, and then yields control to the game
module.


Error handling
--------------

Unless mentioned otherwise, the oshu functions that may fail return an int
which is either 0 on success, and a negative value on error. That negative
value will be -1 most of the time.

Error messages are logged with \ref oshu_log_error by the *callee*. This makes
the code clearer for the caller, and besides, the callee has much more details
about the error than the caller.


Style convention
----------------

Linux kernel coding style full power.

For function names, make them the most natural sounding you can. Use
*oshu_create_thing* than *oshu_thing_create*. This is not how oshu started, so
you'll see the old naming on older code, but as modules get refactored they'll
adopt the new convention.
