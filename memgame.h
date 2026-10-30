/**
 * @file memgame.h
 * @brief Single shared header for the Memorization game.
 * @details Configuration, data types, global state and the public API of every module.
 * Include this (and only this) from each translation unit.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#ifndef MEMGAME_H
#define MEMGAME_H

#define _POSIX_C_SOURCE 200112L

/* ---------------- Standard library ---------------- */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* ---------------- Hardware and display headers ---------------- */
#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "font_types.h"

/** @brief External font descriptor from the compiled font file */
extern font_descriptor_t font_rom8x16;


/* =================================================================
 * Configuration
 * ================================================================= */

/* ---------------- Display Resolution ---------------- */

/** @brief Configured for Landscape mode to match physical LCD boundaries Width */
#define SCREEN_W 480
/** @brief Configured for Landscape mode to match physical LCD boundaries Height */
#define SCREEN_H 320

/* ---------------- 16-bit RGB565 Colors ---------------- */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_GRAY    0x39E7
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

/* ---------------- Shapes & Colors Config ---------------- */
#define SHAPE_SQUARE   0
#define SHAPE_CIRCLE   1
#define SHAPE_TRIANGLE 2
#define N_SHAPES       3

#define N_COLORS  6

/** @brief Palette used for drawing to the LCD (RGB565 format) */
extern const uint16_t palette[N_COLORS];

/** @brief Palette used for the RGB LEDs (24-bit RGB888 format) */
extern const uint32_t led_palette[N_COLORS];

/** @brief Universal sizing parameter for generated shapes */
#define FIG_SIZE   30

/* ---------------- Knobs & Buttons Config ---------------- */

/** @brief Bitmask for the blue knob button state in the SPILED register */
#define BTN_BLUE  (1u << 24)
/** @brief Bitmask for the green knob button state in the SPILED register */
#define BTN_GREEN (1u << 25)
/** @brief Bitmask for the red knob button state in the SPILED register */
#define BTN_RED   (1u << 26)
/** @brief Bitmask for any knob button state */
#define BTN_ANY   (BTN_BLUE | BTN_GREEN | BTN_RED)

/** @brief Knob rotation threshold required to move to the next item */
#define KNOB_TICKS_PER_STEP 4

/* ---------------- Game Logic & Rules ---------------- */
#define MAX_FIGS_PER_ROUND 5
#define ROUNDS_PER_GAME    5
#define N_DIFFICULTIES     3

#define POS_TOLERANCE_PX 90
#define TOP_RESERVED     56

/* Point values */
#define PTS_SHAPE  3
#define PTS_COLOR  3
#define PTS_POS    4
#define PTS_COMBO  2    /**< @brief Bonus for a near-perfect figure (shape + exact color + tight) */
#define PTS_PER_FIG (PTS_SHAPE + PTS_COLOR + PTS_POS + PTS_COMBO)   /**< @brief Max 12 pts per figure */

/**
 * @brief Difficulty configuration structure
 */
typedef struct {
    int      n_figs;     /**< @brief Objects to memorize per round */
    int      show_ms;    /**< @brief Time given to memorize them */
    uint16_t hint16;     /**< @brief Menu color for the LCD */
    uint32_t hint24;     /**< @brief Menu color for the RGB LED */
} difficulty_t;

/** @brief Definitions for Easy, Normal, and Hard modes */
extern const difficulty_t difficulties[N_DIFFICULTIES];

/**
 * @brief Structure representing a single game object on the screen
 */
typedef struct {
    int shape;           /**< @brief Shape type */
    int x, y;            /**< @brief X and Y coordinates */
    int color_idx;       /**< @brief Palette color index */
} figure_t;


/* =================================================================
 * Global mutable state (defined in state.c)
 * ================================================================= */

extern unsigned short *fb;                          /**< @brief Framebuffer pointer */
extern figure_t        targets[MAX_FIGS_PER_ROUND]; /**< @brief The objects to memorize */
extern figure_t        guesses[MAX_FIGS_PER_ROUND]; /**< @brief The player's recreations */
extern int             round_scores[ROUNDS_PER_GAME]; /**< @brief Score tracking */


/* =================================================================
 * Drawing primitives (graphics.c)
 * ================================================================= */

/** @brief Fast solid rectangle fill used for backgrounds and UI elements */
void fill_rect(int x0, int y0, int w, int h, uint16_t c);

/** @brief Flush the entire framebuffer with a single color */
void clear_screen(uint16_t c);

/** @brief Integer square root algorithm used for circular drawing math */
int isqrt_i(int n);

/** @brief Draw a solid square */
void draw_square(int cx, int cy, int half, uint16_t c);

/** @brief Draw a solid circle using integer bounding boxes */
void draw_circle(int cx, int cy, int r, uint16_t c);

/** @brief Draw a filled equilateral triangle (apex pointing UP) */
void draw_triangle(int cx, int cy, int size, uint16_t c);

/** @brief Helper to route shape drawing based on shape index */
void draw_shape(int shape, int cx, int cy, int s, uint16_t c);

/** @brief Draws a hollow rectangle (used for UI selection boxes) */
void draw_rect_outline(int x0, int y0, int w, int h, int t, uint16_t c);

/** @brief Visual cursor indicating where the user is placing an object */
void draw_crosshair(int cx, int cy, uint16_t c);


/* =================================================================
 * Text rendering (text.c)
 * ================================================================= */

/** @brief Renders a null-terminated string scaled by a given multiplier */
void draw_string_scaled(int x, int y, const char *str, uint16_t color,
                        font_descriptor_t *font, int scale);


/* =================================================================
 * Per-phase full-frame renderers (render.c)
 * ================================================================= */

/** @brief Draws the Start Menu / Difficulty Selector */
void render_menu(int diff_idx);

/** @brief Renders the "Memorize Phase" showing the targets with a time bar */
void render_show(int ms_left, int total_show_ms, int n_figs, int round_idx);

/** @brief Renders the "Select Object Phase" (Player chooses shape and color) */
void render_select(int shape_idx, int color_idx,
                   int placed, int n_figs, int round_idx);

/** @brief Renders the "Place Object Phase" (Player positions selected object) */
void render_place(int shape, int color_idx, int cx, int cy,
                  int placed, int n_figs, int round_idx);

/** @brief Renders the Game Over / Total Score Screen */
void render_total(int total, int max_total, int diff);


/* =================================================================
 * LCD hardware blit (lcd.c)
 * ================================================================= */

/** @brief Sends the entire framebuffer to the parallel LCD controller */
void lcd_update(unsigned char *parlcd_mem_base);


/* =================================================================
 * Peripheral helpers - LEDs (leds.c)
 * ================================================================= */

/** @brief Sets the color of the two RGB LEDs on the MZ_APO board */
void set_rgb_leds(unsigned char *mem_base, uint32_t rgb24);

/** @brief Writes directly to the 32-bit LED line register */
void set_led_line(unsigned char *mem_base, uint32_t v);

/** @brief Lights up the lowest `n` bits of the 32-LED line for visual progress */
void show_led_bar(unsigned char *mem_base, int n);


/* =================================================================
 * Game-logic helpers (game.c)
 * ================================================================= */

/** @brief Returns elapsed time in milliseconds relative to a start marker */
long elapsed_ms(const struct timespec *start);

/** @brief Randomly places n objects across the screen, avoiding the status area */
void generate_targets(int n_figs);

/** @brief Best-match (greedy) scoring of the player's guesses against targets */
int round_score(int n_figs);

#endif /* MEMGAME_H */