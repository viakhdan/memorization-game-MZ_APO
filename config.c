/**
* @file config.c
 * @brief Definitions of the shared configuration data tables.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/** @brief Palette used for drawing to the LCD (RGB565 format) */
const uint16_t palette[N_COLORS] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE,
    COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
};

/** @brief Palette used for the RGB LEDs (24-bit RGB888 format) */
const uint32_t led_palette[N_COLORS] = {
    0xFF0000u, 0x00FF00u, 0x0000FFu,
    0xFFFF00u, 0x00FFFFu, 0xFF00FFu
};

/** @brief Definitions for Easy, Normal, and Hard modes */
const difficulty_t difficulties[N_DIFFICULTIES] = {
    /* easy   */ { 3, 8000, COLOR_GREEN,  0x00FF00u },
    /* normal */ { 4, 6000, COLOR_YELLOW, 0xFFFF00u },
    /* hard   */ { 5, 4000, COLOR_RED,    0xFF0000u }
};