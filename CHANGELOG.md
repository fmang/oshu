Changelog
=========

2.0.1 - 2020-03-29
------------------

Lots of refactoring, and a few bug fixes:

- Fix some display glitches in the console status bar.
- Fix broken handling of empty WAV samples.

2.0.0 - 2018-03-21
------------------

Desktop integration, marking the start of a new direction.

Now that the main gameplay for the standard mode is there, it's time to make it
easier to use for everyone. 2.0 features an experimental beatmap indexer
generating an HTML listing of all your beatmaps. Coupled with the desktop
integrations, you can open oshu! from your web browser.

The codebase was migrated to C++ to benefit from stronger type safety and
automatic memory management. The build system is now CMake instead of the
autotools, preparing for Windows support.

1.6.0 - 2018-01-27
------------------

Featuring an improved user interface.

The game now displays the beatmap's metadata in-game, provides a clearer pause
screen and ends with a score screen. In the short term, the console should only
be targeted at the most curious users.

oshu! supports a few performance settings configurable from the *OSHU_QUALITY*
and *OSHU_WINDOW_SIZE* environment variables. This may be required for the game
to be playable on low hardware, and in particular the Raspberry Pi. More
details in the man page.

1.5.0 - 2017-11-25
------------------

New fancy graphics!

This marks a milestone in the oshu! project, as the graphics engine was the
last remnant of the initial version. The new visuals uses accelerated 2D
graphics to render antialiased shapes and translucent objects. This looks nice
but is a significant load compared to the previous wireframe graphics. All the
objects are dynamically drawn using the cairo vector graphics library.

1.4.0 - 2017-11-11
------------------

Introducing seeking ability, to let you replay specific parts of a beatmap.

Overall, there are no revolutionary features, but the graphics and game modules
have been reorganized and improved for more stability, and with the future
modes in mind. Sliders and hit sounds have been tweaked to get closer to the
official osu! client. The command-line output is nicer, with colors instead of
crude log lines.

The keyboard controls have changed a bit, check out the man page.

1.3.0 - 2017-10-14
------------------

Hit sounds!

While the hi-hat was a perfectly decent generic soft sound, exploiting the full
audio sample range gives a completely different experience. Now you'll hear
rhythmic drums if you play well, and somethings other nice sound effects.

With this release comes a new parser, much more accurate than the previous one.
When a beatmap contains an error, the error is displayed and everyone is
invited to report them as oshu issues, because it's likely something is
missing.

The audio backend is also completely revamped to feature a flexible way to play
many sound effects at once. It should also be much more readable and organized.

1.2.0 - 2017-09-16
------------------

Background pictures!

The overall graphics engine now uses more precise positions, yielding smoother
curves, and also uses a finer game clock, while ensuring a constant frame rate.

It's mostly a technical release though, with a more generic game module
refactored to support more game modes in the future. More refactoring to come
in the next release.

1.1.0 - 2017-09-02
------------------

Full slider support!

The game is now quite playable, and starts getting fun.

Extra features:

- Resizable window.
- Slightly nicer graphics, even though it's still wireframe.
- Dereference the symlinked beatmap files.
- Automatic pause when the window focus is lost.
- `--pause` option to start the game paused.
- Print a crude score on the terminal output at the end of songs.

1.0.0 - 2017-08-07
------------------

This is first working prototype of the project. Let's call it a
proof-of-concept. It is basically playable even though it lacks major features
like sliders and scoring.

Here's what we got so far:

- Extremely basic gameplay.
- Overall support of most of the official beatmaps.
