/**
 * \file ui/audio.h
 * \ingroup ui_audio
 */

#pragma once

struct oshu_display;
struct oshu_stream;

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
 * \sa oshu_create_audio_progress_bar
 * \sa oshu_destroy_audio_progress_bar
 * \sa oshu_show_audio_progress_bar
 */
struct oshu_audio_progress_bar_widget {
	struct oshu_display *display;
	struct oshu_stream *stream;
};

/**
 * Create an audio progress bar for an audio stream.
 *
 * The audio stream must exist at least as long as the widget.
 *
 * The stream must have a non-zero duration.
 */
int oshu_create_audio_progress_bar(struct oshu_display *display, struct oshu_stream *stream, struct oshu_audio_progress_bar_widget *bar);

/**
 * Display the progress bar at the bottom of the screen.
 */
void oshu_show_audio_progress_bar(struct oshu_audio_progress_bar_widget *bar);

/**
 * Destroy an audio progress bar.
 *
 * Today, this is a no-op. It is still provided for interface consistency with
 * the other widgets.
 */
void oshu_destroy_audio_progress_bar(struct oshu_audio_progress_bar_widget *bar);

/** \} */
