/**
* @file text.c
 * @brief Scaled bitmap text rendering using font_rom8x16.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/**
 * @brief Renders a single character scaled by a given multiplier
 * @param x Top-left X coordinate of the character bounding box
 * @param y Top-left Y coordinate of the character bounding box
 * @param c Character to draw
 * @param color 16-bit RGB565 color for the text
 * @param font Pointer to the font descriptor structure
 * @param scale Multiplication factor for scaling character size
 */
static void draw_char_scaled(int x, int y, char c, uint16_t color, font_descriptor_t *font, int scale) {
    const uint16_t *bits = font->bits + (c - font->firstchar) * font->height;

    for (int i = 0; i < font->height; i++) {
        uint16_t mask = 1 << 15; /* Start checking at the leftmost bit */
        for (int j = 0; j < font->maxwidth; j++) {
            /* Draw a scaled pixel block if the bit is 1 */
            if (bits[i] & mask) {
                fill_rect(x + j * scale, y + i * scale, scale, scale, color);
            }
            mask >>= 1; /* Shift mask right to test the next pixel */
        }
    }
}

/**
 * @brief Renders a null-terminated string scaled by a given multiplier
 * @param x Top-left X coordinate of the start of the string
 * @param y Top-left Y coordinate of the start of the string
 * @param str Null-terminated string to render
 * @param color 16-bit RGB565 color for the text
 * @param font Pointer to the font descriptor structure
 * @param scale Multiplication factor for scaling text size
 */
void draw_string_scaled(int x, int y, const char *str, uint16_t color, font_descriptor_t *font, int scale) {
    int current_x = x;
    while (*str) {
        draw_char_scaled(current_x, y, *str, color, font, scale);
        current_x += font->maxwidth * scale; /* Move cursor by font width * scale */
        str++;
    }
}