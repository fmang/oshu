/**
 * \file ui/audio.h
 * \ingroup ui_audio
 */

#pragma once

namespace oshu {

struct display;
struct stream;

/**
 * \defgroup ui_audio Audio
 * \ingroup ui
 *
 * \brief
 * Audio-related widgets.
 *
 * \{
 */

/**
 * The audio progress bar show what part of a stream is currently playing.
 *
 * It appears as a full-width transparent bar that fills as the song is played.
 *
 * \sa oshu::create_audio_progress_bar
 * \sa oshu::destroy_audio_progress_bar
 * \sa oshu::show_audio_progress_bar
 */
struct audio_progress_bar {
	oshu::display *display;
	oshu::stream *stream;
};

/**
 * Create an audio progress bar for an audio stream.
 *
 * The audio stream must exist at least as long as the widget.
 *
 * The stream must have a non-zero duration.
 */
int create_audio_progress_bar(oshu::display *display, oshu::stream *stream, oshu::audio_progress_bar *bar);

/**
 * Display the progress bar at the bottom of the screen.
 */
void show_audio_progress_bar(oshu::audio_progress_bar *bar);

/**
 * Destroy an audio progress bar.
 *
 * Today, this is a no-op. It is still provided for interface consistency with
 * the other widgets.
 */
void destroy_audio_progress_bar(oshu::audio_progress_bar *bar);

/** \} */

}
