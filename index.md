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


Directory structure
-------------------

The repository is organized into the usual directory structure, if you're
familiar with the FHS.

```
include/
	Public .h headers files for liboshu.
	They contain most of the Doxygen documentation, and define the
	cross-module interfaces.
lib/
	Implementation of liboshu.
	It may also contain private header files.
src/
	Source of the executables.
	Most of them are light wrappers around liboshu, providing a CLI
	interface.
share/
	Resources and other static assets.
	Includes audio samples and man pages.
test/
	The test suite, ensuring liboshu and the executables work well.
```

\ref liboshu accounts for the biggest part of the project.
