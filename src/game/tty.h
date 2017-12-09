#pragma once

struct oshu_game;

void oshu_welcome(struct oshu_game *game);

/**
 * Show the state of the game (paused/playing) and the current song position.
 *
 * Only do that for terminal outputs in order not to spam something if the
 * output is redirected.
 *
 * The state length must not decrease over time, otherwise you end up with
 * glitches. If you write `foo\rx`, you get `xoo`. This is the reason the
 * Paused string literal has an extra space.
 */
void oshu_print_state(struct oshu_game *game);

void oshu_congratulate(struct oshu_game *game);
