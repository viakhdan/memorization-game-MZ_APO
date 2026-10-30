/**
 * @file main.c
 * @brief Memorization game on the MZ_APO board.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 *
 * @details
 * MENU   - rotate blue knob to pick EASY / NORMAL / HARD (3, 4, or 5
 * figures per round; show time 8 / 6 / 4 s). Press any knob
 * to start the game.
 * SHOW   - the round's random figures appear on screen for the
 * selected duration; a countdown bar shrinks at the bottom.
 * SELECT - rotate blue knob = shape, green knob = color. Press any
 * knob to confirm.
 * PLACE  - rotate blue knob = X, green knob = Y. Press any knob to
 * save the figure. SELECT/PLACE repeat until the round is
 * complete.
 * TOTAL  - after 5 rounds the cumulative score and per-round bars
 * are shown. Press any knob to return to MENU.
 *
 * Scoring per figure (best-match across remaining unmatched targets;
 * the order in which the player draws them does NOT matter):
 * - shape match    -> +3 pts
 * - color match    -> +3 exact / +1 adjacent hue / 0 otherwise
 * - position       -> up to +4 pts (quadratic falloff in 90 px tol)
 * - combo bonus    -> +2 for correct shape + exact color + tight place
 * - total miss     -> wrong shape AND wrong color scores 0 (no gimme)
 * Max 12 pts per figure.
 *
 * Exit at any time: press red and green knob buttons simultaneously.
 */

#include "memgame.h"


/* =================================================================
 * Main Application Loop & State Machine
 * ================================================================= */

/**
 * @brief Program entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status code
 */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    /* Allocate the software framebuffer */
    fb = (unsigned short *)malloc(SCREEN_W * SCREEN_H * sizeof(unsigned short));
    if (!fb) {
        fprintf(stderr, "Failed to allocate framebuffer\n");
        return 1;
    }

    printf("Booting Memorization Game...\n");

    /* Map physical hardware addresses to virtual memory */
    unsigned char *parlcd_mem_base =
        map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    if (!parlcd_mem_base) { free(fb); return 1; }

    unsigned char *mem_base =
        map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
    if (!mem_base) { free(fb); return 1; }

    /* Initialize LCD Controller */
    parlcd_hx8357_init(parlcd_mem_base);

    /* Seed RNG with high-resolution clock */
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        srand((unsigned)(ts.tv_nsec ^ ts.tv_sec));
    }

    /* Initialize Hardware to Default State */
    clear_screen(COLOR_BLACK);
    lcd_update(parlcd_mem_base);
    set_led_line(mem_base, 0);
    set_rgb_leds(mem_base, 0);

    /* Frame cap: ~20 fps */
    struct timespec loop_delay = {
        .tv_sec  = 0,
        .tv_nsec = 50 * 1000 * 1000
    };

    /* Game State Machine Definitions */
    enum {
        PHASE_MENU, PHASE_SHOW, PHASE_SELECT, PHASE_PLACE, PHASE_TOTAL
    } phase = PHASE_MENU;

    /* Snapshot initial knob data to prevent 'ghost' input at boot */
    uint32_t r0         = *(volatile uint32_t *)(mem_base + SPILED_REG_KNOBS_8BIT_o);
    uint8_t  prev_blue  = (uint8_t)((r0 >> 0) & 0xFF);
    uint8_t  prev_green = (uint8_t)((r0 >> 8) & 0xFF);
    uint32_t prev_btns  = r0 & BTN_ANY;

    /* Menu & Global vars */
    int diff       = 0;
    int diff_accum = 0;

    int round_idx = 0;
    int total     = 0;
    int n_figs    = difficulties[0].n_figs;
    int show_ms   = difficulties[0].show_ms;
    int max_total = ROUNDS_PER_GAME * n_figs * PTS_PER_FIG;

    /* Round tracking vars */
    int n_guesses   = 0;
    int shape_idx   = 0;
    int color_idx   = 0;
    int shape_accum = 0;
    int color_accum = 0;

    struct timespec show_start;
    clock_gettime(CLOCK_MONOTONIC, &show_start);

    /* --- Primary Game Loop --- */
    while (1) {

        /* 1. Poll Hardware Input */
        uint32_t r = *(volatile uint32_t *)(mem_base + SPILED_REG_KNOBS_8BIT_o);

        uint8_t  blue    = (uint8_t)((r >> 0)  & 0xFF);
        uint8_t  green   = (uint8_t)((r >> 8)  & 0xFF);
        uint32_t btns    = r & BTN_ANY;
        uint32_t pressed = btns & ~prev_btns; /* Detect button rising edge */

        /* Universal Escape To Menu Condition: Press Red and Green simultaneously */
        if ((btns & (BTN_RED | BTN_GREEN)) == (BTN_RED | BTN_GREEN)) {
            /* Reset hardware elements to baseline menu state */
            clear_screen(COLOR_BLACK);
            set_led_line(mem_base, 0);
            set_rgb_leds(mem_base, 0);

            /* Reset menu accumulation and state machine phase */
            diff_accum = 0;
            phase = PHASE_MENU;

            /* Update input state before jumping to the next iteration */
            prev_blue  = blue;
            prev_green = green;
            prev_btns  = btns;

            /* Continue the loop to immediately render the menu */
            continue;
        }
        int8_t d_blue  = (int8_t)(blue  - prev_blue);
        int8_t d_green = (int8_t)(green - prev_green);

        /* 2. Execute Current State */
        if (phase == PHASE_MENU) {

            /* Accumulate rotation delta and switch difficulties */
            diff_accum += d_blue;
            while (diff_accum >=  KNOB_TICKS_PER_STEP) {
                diff       = (diff + 1) % N_DIFFICULTIES;
                diff_accum -= KNOB_TICKS_PER_STEP;
            }
            while (diff_accum <= -KNOB_TICKS_PER_STEP) {
                diff       = (diff + N_DIFFICULTIES - 1) % N_DIFFICULTIES;
                diff_accum += KNOB_TICKS_PER_STEP;
            }

            render_menu(diff);
            set_rgb_leds(mem_base, difficulties[diff].hint24);
            set_led_line(mem_base, 0);

            /* Any button press locks in difficulty and starts the game */
            if (pressed) {
                n_figs    = difficulties[diff].n_figs;
                show_ms   = difficulties[diff].show_ms;
                max_total = ROUNDS_PER_GAME * n_figs * PTS_PER_FIG;
                round_idx = 0;
                total     = 0;

                for (int i = 0; i < ROUNDS_PER_GAME; ++i) round_scores[i] = 0;

                n_guesses = 0;
                shape_accum = color_accum = 0;

                generate_targets(n_figs);
                clock_gettime(CLOCK_MONOTONIC, &show_start);
                phase = PHASE_SHOW;
            }

        } else if (phase == PHASE_SHOW) {

            /* Display targets and compute countdown */
            long el = elapsed_ms(&show_start);
            int  ms_left = show_ms - (int)el;

            if (ms_left <= 0) {
                phase = PHASE_SELECT;
                /* Clear any accumulated knob twists from staring at the screen */
                shape_accum = color_accum = 0;
            } else {
                render_show(ms_left, show_ms, n_figs, round_idx);
                set_rgb_leds(mem_base, 0);
            }

        } else if (phase == PHASE_SELECT) {

            /* Rotate Blue Knob -> Change Shape */
            shape_accum += d_blue;
            while (shape_accum >=  KNOB_TICKS_PER_STEP) {
                shape_idx    = (shape_idx + 1) % N_SHAPES;
                shape_accum -= KNOB_TICKS_PER_STEP;
            }
            while (shape_accum <= -KNOB_TICKS_PER_STEP) {
                shape_idx    = (shape_idx + N_SHAPES - 1) % N_SHAPES;
                shape_accum += KNOB_TICKS_PER_STEP;
            }

            /* Rotate Green Knob -> Change Color */
            color_accum += d_green;
            while (color_accum >=  KNOB_TICKS_PER_STEP) {
                color_idx    = (color_idx + 1) % N_COLORS;
                color_accum -= KNOB_TICKS_PER_STEP;
            }
            while (color_accum <= -KNOB_TICKS_PER_STEP) {
                color_idx    = (color_idx + N_COLORS - 1) % N_COLORS;
                color_accum += KNOB_TICKS_PER_STEP;
            }

            render_select(shape_idx, color_idx, n_guesses, n_figs, round_idx);
            set_rgb_leds(mem_base, led_palette[color_idx]);

            /* Any button press moves to placement phase */
            if (pressed) phase = PHASE_PLACE;

        } else if (phase == PHASE_PLACE) {

            /* Use absolute knob values 0-255 mapped to screen coords */
            int cx = (blue  * SCREEN_W) / 256;
            int cy = (green * SCREEN_H) / 256;

            render_place(shape_idx, color_idx, cx, cy,
                         n_guesses, n_figs, round_idx);
            set_rgb_leds(mem_base, led_palette[color_idx]);

            if (pressed && n_guesses < n_figs) {
                guesses[n_guesses].shape     = shape_idx;
                guesses[n_guesses].x         = cx;
                guesses[n_guesses].y         = cy;
                guesses[n_guesses].color_idx = color_idx;
                n_guesses++;

                if (n_guesses >= n_figs) {
                    /* Round is completely finished */
                    int rs = round_score(n_figs);
                    round_scores[round_idx] = rs;
                    total += rs;
                    round_idx++;

                    printf("Round %d: %d / %d (total %d)\n",
                           round_idx, rs, n_figs * PTS_PER_FIG, total);

                    if (round_idx >= ROUNDS_PER_GAME) {
                        /* End Game: Update LED Line to reflect final score */
                        int led_n = (32 * total + max_total / 2) / max_total;
                        show_led_bar(mem_base, led_n);
                        printf("Game over: %d / %d\n", total, max_total);
                        phase = PHASE_TOTAL;
                    } else {
                        /* Trigger next round */
                        n_guesses = 0;
                        shape_accum = color_accum = 0;
                        generate_targets(n_figs);
                        clock_gettime(CLOCK_MONOTONIC, &show_start);
                        show_led_bar(mem_base, round_idx); /* Show round progress */
                        phase = PHASE_SHOW;
                    }
                } else {
                    /* Start creating next figure */
                    phase = PHASE_SELECT;
                }
            }

        } else { /* PHASE_TOTAL */
            render_total(total, max_total, diff);
            set_rgb_leds(mem_base, 0);

            /* A button press from game over resets the game to the Menu */
            if (pressed) {
                set_led_line(mem_base, 0);
                diff_accum = 0;
                phase = PHASE_MENU;
            }
        }

        /* 3. Flush Framebuffer to Physical Display */
        lcd_update(parlcd_mem_base);

        /* 4. Save input state for next loop iteration */
        prev_blue  = blue;
        prev_green = green;
        prev_btns  = btns;

        /* Enforce frame cap */
        clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }

    /* Clean exit when main loop breaks */
    clear_screen(COLOR_BLACK);
    lcd_update(parlcd_mem_base);
    set_led_line(mem_base, 0);
    set_rgb_leds(mem_base, 0);

    free(fb);
    printf("Goodbye world\n");
    return 0;
}