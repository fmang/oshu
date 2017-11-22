Contributing
============

Because the project is really young, please join the \#oshu IRC channel on
[chat.freenode.net](https://freenode.net/kb/answer/chat) if you'd like to
contribute. Someone is probably already working on your idea, and most likely
myself :)

You may also open issues on [GitHub][] or contact me directly by mail.

[GitHub]: https://github.com/fmang/oshu/


Documentation
-------------

To generate the technical documentation of the game with Doxygen, you need to
cd to the `docs/` directory and run `make apidoc`. The documentation will be
accessible from `docs/html/index.html`. By the way, if you didn't have Doxygen
installed when you ran `./configure`, you'll need to call it again for it to
detect Doxygen.


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
kind of thing anyway. Another required feature is the mixing of sound effects,
which don't make much sense in frameworks like gstreamer.

The easy way is relying on the process time and hoping it doesn't deviate too
much from the song. The hard way is using the low-level interfaces to keep in
sync with the music.

Relying on libavcodec (ffmpeg) is the killer option, even though it's quite
manual and relies on a good understanding of the audio decoding process. But
hey, it's an opportunity to learn! Beside, using libavcodec provides an
incredible set of supported audio formats.
