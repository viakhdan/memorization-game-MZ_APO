/**
 * @file render.c
 * @brief Per-phase full-frame renderers for each game state.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/**
 * @brief Small colored line at top of screen to indicate game state
 * @param tint 16-bit RGB565 color for the status bar
 */
static void render_status_bar(uint16_t tint)
{
    fill_rect(0, 0, SCREEN_W, 6, tint);
}

/**
 * @brief Display progress across the 5 game rounds. Filled = Complete
 * @param rounds_done Number of currently completed rounds
 */
static void render_round_dots(int rounds_done)
{
    int r = 6, gap = 14, py = SCREEN_H - 16;
    int total_w = ROUNDS_PER_GAME * 2 * r + (ROUNDS_PER_GAME - 1) * gap;
    int x0 = (SCREEN_W - total_w) / 2 + r;
    for (int i = 0; i < ROUNDS_PER_GAME; ++i) {
        int xi = x0 + i * (2 * r + gap);
        draw_circle(xi, py, r, (i < rounds_done) ? COLOR_GREEN : COLOR_GRAY);
    }
}

/**
 * @brief Display progress for the current round (how many objects placed)
 * @param placed Number of objects currently placed by user
 * @param total Total number of figures needed for the round
 */
static void render_figure_dots(int placed, int total)
{
    int pw = 30, ph = 12, py = 290;
    int pgap = 12;
    int total_w = total * pw + (total - 1) * pgap;
    int px0 = (SCREEN_W - total_w) / 2;
    for (int i = 0; i < total; ++i) {
        int px = px0 + i * (pw + pgap);
        if (i < placed) fill_rect(px, py, pw, ph, COLOR_GREEN);
        else            draw_rect_outline(px, py, pw, ph, 1, COLOR_GRAY);
    }
}

/**
 * @brief Draws the Start Menu / Difficulty Selector
 * @param diff_idx Index of currently selected difficulty mode
 */
void render_menu(int diff_idx)
{
    clear_screen(COLOR_BLACK);
    render_status_bar(COLOR_WHITE);

    /* Three colored panels:
       Top number = figures per round
       Bottom number = seconds of memory time */
    int box_w = 120, box_h = 200;
    int gap   = (SCREEN_W - 3 * box_w) / 4;
    int y0    = 40;

    for (int i = 0; i < N_DIFFICULTIES; ++i) {
        int x0 = gap + i * (box_w + gap);
        fill_rect(x0, y0, box_w, box_h, difficulties[i].hint16);

        /* Draw Figures Count (Big Font) */
        char n_figs_str[2] = { '0' + difficulties[i].n_figs, '\0' };
        int sb = 6; /* Scaled size 6 (48x96 px) */
        int x_n = x0 + (box_w - font_rom8x16.maxwidth * sb) / 2;
        int y_n = y0 + 30;
        draw_string_scaled(x_n, y_n, n_figs_str, COLOR_BLACK, &font_rom8x16, sb);

        /* Draw Memory Time Limit (Small Font) */
        int sec = (difficulties[i].show_ms + 500) / 1000;
        char sec_str[2] = { '0' + sec, '\0' };
        int ss = 3; /* Scaled size 3 (24x48 px) */
        int x_s = x0 + (box_w - font_rom8x16.maxwidth * ss) / 2;
        int y_s = y0 + box_h - font_rom8x16.height * ss - 14;
        draw_string_scaled(x_s, y_s, sec_str, COLOR_BLACK, &font_rom8x16, ss);

        /* Draw highlight outline if selected */
        if (i == diff_idx)
            draw_rect_outline(x0 - 4, y0 - 4, box_w + 8, box_h + 8, 4, COLOR_WHITE);
        else
            draw_rect_outline(x0 - 4, y0 - 4, box_w + 8, box_h + 8, 1, COLOR_GRAY);
    }

    /* --- NEW EXPLANATORY TEXT --- */
    /* Get the seconds for the currently selected difficulty */
    int selected_sec = (difficulties[diff_idx].show_ms + 500) / 1000;
    int selected_figs = difficulties[diff_idx].n_figs;

    char expl_str[32];
    snprintf(expl_str, sizeof(expl_str), "%d FIGURES - %d SECONDS", selected_figs, selected_sec);

    int expl_scale = 2; /* 16x32 px font size */

    /* Calculate string length to perfectly center it */
    int str_len = 0;
    while(expl_str[str_len] != '\0') str_len++;

    int expl_w = str_len * font_rom8x16.maxwidth * expl_scale;
    int expl_x = (SCREEN_W - expl_w) / 2;
    int expl_y = y0 + box_h + 25; /* Positioned nicely below the boxes */

    draw_string_scaled(expl_x, expl_y, expl_str, COLOR_WHITE, &font_rom8x16, expl_scale);
}

/**
 * @brief Renders the "Memorize Phase" showing the targets with a time bar
 * @param ms_left Number of milliseconds left in the memorize phase
 * @param total_show_ms Total duration of the memorize phase
 * @param n_figs Number of objects targeted
 * @param round_idx Current round index
 */
void render_show(int ms_left, int total_show_ms, int n_figs, int round_idx)
{
    clear_screen(COLOR_BLACK);
    render_status_bar(COLOR_MAGENTA);

    /* Draw all targets for the round */
    for (int i = 0; i < n_figs; ++i) {
        draw_shape(targets[i].shape, targets[i].x, targets[i].y,
                   FIG_SIZE, palette[targets[i].color_idx]);
    }

    /* Calculate and draw shrinking countdown bar at the bottom */
    int bar_w = (SCREEN_W * ms_left) / total_show_ms;
    if (bar_w < 0) bar_w = 0;
    if (bar_w > SCREEN_W) bar_w = SCREEN_W;
    fill_rect(0, SCREEN_H - 8, bar_w, 8, COLOR_WHITE);

    render_round_dots(round_idx);
}

/**
 * @brief Renders the "Select Object Phase" (Player chooses shape and color)
 * @param shape_idx Currently highlighted shape index
 * @param color_idx Currently highlighted color index
 * @param placed Number of objects already placed this round
 * @param n_figs Total number of objects expected this round
 * @param round_idx Current round index
 */
void render_select(int shape_idx, int color_idx,
                   int placed, int n_figs, int round_idx)
{
    clear_screen(COLOR_BLACK);
    render_status_bar(COLOR_YELLOW);

    uint16_t col = palette[color_idx];

    /* Render the 3 Shape choices */
    int box_w = 100, box_h = 100;
    int gap   = (SCREEN_W - 3 * box_w) / 4;
    int y0    = 30;
    for (int i = 0; i < N_SHAPES; ++i) {
        int x0 = gap + i * (box_w + gap);
        int cx = x0 + box_w / 2;
        int cy = y0 + box_h / 2;

        draw_shape(i, cx, cy, 35, col);

        /* Highlight selected shape */
        if (i == shape_idx)
            draw_rect_outline(x0 - 4, y0 - 4, box_w + 8, box_h + 8, 3, COLOR_WHITE);
        else
            draw_rect_outline(x0 - 4, y0 - 4, box_w + 8, box_h + 8, 1, COLOR_GRAY);
    }

    /* Render the 6 Color choices */
    int sw = 50, sh = 50;
    int sgap = (SCREEN_W - N_COLORS * sw) / (N_COLORS + 1);
    int sy   = 180;
    for (int i = 0; i < N_COLORS; ++i) {
        int sx = sgap + i * (sw + sgap);
        fill_rect(sx, sy, sw, sh, palette[i]);

        /* Highlight selected color */
        if (i == color_idx)
            draw_rect_outline(sx - 4, sy - 4, sw + 8, sh + 8, 3, COLOR_WHITE);
    }

    render_figure_dots(placed, n_figs);
    render_round_dots(round_idx);
}

/**
 * @brief Renders the "Place Object Phase" (Player positions selected object)
 * @param shape Selected shape index
 * @param color_idx Selected color index
 * @param cx Current crosshair X coordinate
 * @param cy Current crosshair Y coordinate
 * @param placed Number of objects already placed this round
 * @param n_figs Total number of objects expected this round
 * @param round_idx Current round index
 */
void render_place(int shape, int color_idx, int cx, int cy,
                  int placed, int n_figs, int round_idx)
{
    clear_screen(COLOR_BLACK);
    render_status_bar(COLOR_CYAN);

    /* Draw objects that have already been placed this round */
    for (int i = 0; i < placed; ++i) {
        draw_shape(guesses[i].shape, guesses[i].x, guesses[i].y,
                   FIG_SIZE, palette[guesses[i].color_idx]);
    }

    /* Draw the currently selected shape at the cursor location */
    draw_shape(shape, cx, cy, FIG_SIZE, palette[color_idx]);
    draw_crosshair(cx, cy, COLOR_WHITE);

    render_figure_dots(placed, n_figs);
    render_round_dots(round_idx);
}

/**
 * @brief Renders the Game Over / Total Score Screen
 * @param total Cumulative score for the game
 * @param max_total Maximum possible score
 * @param diff Selected difficulty index used to calculate scale
 */
void render_total(int total, int max_total, int diff)
{
    /* Tint the status bar based on performance tier */
    uint16_t tint = COLOR_RED;
    if      (total * 100 >= 80 * max_total) tint = COLOR_GREEN;
    else if (total * 100 >= 50 * max_total) tint = COLOR_YELLOW;

    clear_screen(COLOR_BLACK);
    render_status_bar(tint);

    /* Format and draw the Total Score (e.g., "120/150") */
    char score_str[16];
    snprintf(score_str, sizeof(score_str), "%d/%d", total, max_total);

    int scale = 4; /* Scaled size 4 (32x64 px) */

    /* Calculate string length to properly center the text */
    int str_len = 0;
    for (int i = 0; score_str[i] != '\0'; i++) str_len++;

    int width = str_len * font_rom8x16.maxwidth * scale;
    int x = (SCREEN_W - width) / 2;
    int y = 40;

    draw_string_scaled(x, y, score_str, COLOR_WHITE, &font_rom8x16, scale);

    /* Draw the Per-Round Performance Bar Graph across the bottom */
    int max_per_round = PTS_PER_FIG * difficulties[diff].n_figs;
    int bar_w = 50, bar_h_max = 80;
    int bar_gap = 16;
    int total_bars_w = ROUNDS_PER_GAME * bar_w + (ROUNDS_PER_GAME - 1) * bar_gap;
    int bx0 = (SCREEN_W - total_bars_w) / 2;
    int by_base = SCREEN_H - 30;

    for (int i = 0; i < ROUNDS_PER_GAME; ++i) {
        int bxi = bx0 + i * (bar_w + bar_gap);
        int bh  = (round_scores[i] * bar_h_max) / max_per_round;
        if (bh > bar_h_max) bh = bar_h_max;

        /* Fill bar to represent round score */
        if (bh > 0) fill_rect(bxi, by_base - bh, bar_w, bh, tint);

        /* Outline the maximum possible bar height */
        draw_rect_outline(bxi, by_base - bar_h_max, bar_w, bar_h_max, 1, COLOR_GRAY);
    }
}