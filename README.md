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
