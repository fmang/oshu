oshu!
=====

Welcome to oshu!

This project was born out of frustration from not being able to play Osu! on a
little Linux box with no graphics card whatsoever.  It's aimed at minimalistic
people and probably will never contain a hundredth of the official client's
features.

Osu! is that PC clone of the Osu! Tatakae! Ouendan DS rhythm game,
occidentalized as Elite Beat Agents.

Osu! has got tons of cool beatmaps, and a more or less intuitive text file
format described at https://osu.ppy.sh/help/wiki/osu!_File_Formats

The main problem with Osu! is that it required Windows and a graphics card.
Even with that, it sometimes get slow unless the computer is a beast… Hell, the
mania and taiko mode could be played in a terminal!

Let's make a compatible lightweight version of the game!


Getting started
---------------

First, you need to download a few beatmaps from osu's website.
See https://osu.ppy.sh/p/packlist to get packs, or
https://osu.ppy.sh/p/beatmaplist for a finer selection.

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


Install
-------

**Note:** oshu! is still in an early stage of development. Don't expect it to
be amazing. It doesn't even have a menu to select songs, you need to call it
from the command line like `$ oshu my_beatmap.osu`.

First of all, make sure you have the required dependencies. These things are so
common that I may assure you they're in your distribution's official
repositories.

- SDL2 and SDL2_image,
- ffmpeg or libav,
- a C99 compiler,
- pkg-config,
- autoconf, automake (when building from the git repository).

oshu! is a standard autoconf'd project, so install it the regular way with:

	# ./autogen.sh if the configure file is missing.
	./configure --prefix="$HOME/.local"
	make && make install

If you do use the `$HOME/.local` prefix, make sure you add `~/.local/bin` to
your PATH, or invoke oshu! by specifying the full path like `~/.local/bin/oshu
BEATMAP.osu`. Otherwise you may use the default `/usr/local`.

If you're lucky, someone may have packaged it for your distribution.


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
- No fancy graphics.

But it just means I probably won't have the time to work on these. These
limitations are *not* a design choice. Contributions are welcome!


Technical choices
-----------------

C is love, C is life.

Since this project is meant to have fun, let's also have fun while making it!

First of all, the graphical part. Tons of libraries exist, some OpenGL-focused
like glfw, some that are hybrid and able to make do with the CPU only. Of
course, we're going to choose the latter.

So we've got SDL2 and overkill toolkits like Qt and GTK. SDL2 sounds good.

For the audio part, it's more complicated. All the graphical toolkits above
have an audio module, but if we're going for the SDL, might as well use the SDL
audio facility because it will be well integrated.

The SDL2 audio subsystem is quite basic and can only play raw audio. A
higher-level library called SDL2_mixer provides a cool way to load fancy files
like MP3 but does not provide fine control over the stream. Specifically, it
won't tell us the time information in the audio stream's frame, which is
crucial for a rhythm game. Most high-level audio libraries don't provide that
kind of thing anyway.

The easy way is relying on the process time and hoping it doesn't deviate too
much from the song. The hard way is using the low-level interfaces to keep in
sync with the music.

Relying on libavcodec (ffmpeg) is the killer option, even though it's quite
manual and relies on a good understanding of the audio decoding process. But
hey, it's an opportunity to learn! Beside, using libavcodec will provide an
incredible set of supported audio formats.

Even after it's done, remains to see how audio can be mixed between different
files to play the drum, whistle and other kinds of sound effects.


Documentation
-------------

The user manual's man page is installed in the standard man directory, but you
may view it directly from `docs/oshu.1` or straight from the template at
`docs/oshu.1.in`.

To generate the technical documentation of the game with Doxygen, you need to
move to the `docs/` directory and run `make apidoc`. The documentation will be
accessible from `docs/html/index.html`. By the way, if you didn't have Doxygen
installed when you ran `./configure`, you'll need to call it again for it to
detect Doxygen.


Contributing
------------

Because the project is really young, please join the \#oshu IRC channel on
[chat.freenode.net](https://freenode.net/kb/answer/chat) if you'd like to
contribute. Someone is probably already working on your idea, and most likely
myself :)

You may also open issues on [GitHub][] or contact me directly by mail.

[GitHub]: https://github.com/fmang/oshu/


Name
----

A few keywords, from the original games names, and a few other things:

* 燃えろ moero (burn),
* 熱血 nekketsu (hot blood),
* リズム魂 rizumu damashii (rhythm soul)
* 押忍 osu,
* 戦え tatakae (fight),
* 応援団 ouendan (cheerleaders, from the original game)
* beat, pulse,
* fire,
* dash.

Let's go with the name oshu!

It's like osu! but with the s pronounced sh, because it's a mini-osu.
