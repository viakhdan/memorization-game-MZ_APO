/**
 * @file graphics.c
 * @brief Low-level drawing primitives operating on the software framebuffer.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/**
 * @brief Safely draw a single pixel within screen boundaries
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 * @param c 16-bit RGB565 color
 */
static void draw_pixel(int x, int y, uint16_t c)
{
    if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        fb[y * SCREEN_W + x] = c;
}

/**
 * @brief Fast solid rectangle fill used for backgrounds and UI elements
 * @param x0 Top-left X coordinate
 * @param y0 Top-left Y coordinate
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param c 16-bit RGB565 color
 */
void fill_rect(int x0, int y0, int w, int h, uint16_t c)
{
    int x1 = x0 + w, y1 = y0 + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > SCREEN_W) x1 = SCREEN_W;
    if (y1 > SCREEN_H) y1 = SCREEN_H;
    for (int y = y0; y < y1; ++y) {
        unsigned short *row = fb + y * SCREEN_W;
        for (int x = x0; x < x1; ++x) row[x] = c;
    }
}

/**
 * @brief Flush the entire framebuffer with a single color
 * @param c 16-bit RGB565 color to fill the screen
 */
void clear_screen(uint16_t c)
{
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) fb[i] = c;
}

/**
 * @brief Integer square root algorithm used for circular drawing math
 * @param n Integer value to square root
 * @return The integer square root of n
 */
int isqrt_i(int n)
{
    if (n <= 0) return 0;
    int x = n, y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + n / x) >> 1; }
    return x;
}

/**
 * @brief Draw a solid square
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param half Half of the total width/height of the square
 * @param c 16-bit RGB565 color
 */
void draw_square(int cx, int cy, int half, uint16_t c)
{
    fill_rect(cx - half, cy - half, 2 * half, 2 * half, c);
}

/**
 * @brief Draw a solid circle using integer bounding boxes
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param r Radius of the circle
 * @param c 16-bit RGB565 color
 */
void draw_circle(int cx, int cy, int r, uint16_t c)
{
    int r2 = r * r;
    for (int dy = -r; dy <= r; ++dy) {
        int y = cy + dy;
        if ((unsigned)y >= SCREEN_H) continue;
        int dx = isqrt_i(r2 - dy * dy);
        int x0 = cx - dx, x1 = cx + dx;
        if (x0 < 0) x0 = 0;
        if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
        unsigned short *row = fb + y * SCREEN_W;
        for (int x = x0; x <= x1; ++x) row[x] = c;
    }
}

/**
 * @brief Draw a filled equilateral triangle (apex pointing UP)
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param size Height scaling parameter for the triangle
 * @param c 16-bit RGB565 color
 */
void draw_triangle(int cx, int cy, int size, uint16_t c)
{
    int top_y    = cy - size;
    int bottom_y = cy + (size >> 1);
    for (int y = top_y; y <= bottom_y; ++y) {
        if ((unsigned)y >= SCREEN_H) continue;
        int dy     = y - top_y;
        int half_w = (dy * 577) / 1000;     /* 1/sqrt(3) ~= 0.577 */
        int x0 = cx - half_w, x1 = cx + half_w;
        if (x0 < 0) x0 = 0;
        if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
        unsigned short *row = fb + y * SCREEN_W;
        for (int x = x0; x <= x1; ++x) row[x] = c;
    }
}

/**
 * @brief Helper to route shape drawing based on shape index
 * @param shape Macro identifier for the shape (SHAPE_SQUARE, SHAPE_CIRCLE, etc.)
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param s Size or radius parameter depending on the shape
 * @param c 16-bit RGB565 color
 */
void draw_shape(int shape, int cx, int cy, int s, uint16_t c)
{
    switch (shape) {
        case SHAPE_SQUARE:   draw_square(cx, cy, s, c);   break;
        case SHAPE_CIRCLE:   draw_circle(cx, cy, s, c);   break;
        case SHAPE_TRIANGLE: draw_triangle(cx, cy, s, c); break;
    }
}

/**
 * @brief Draws a hollow rectangle (used for UI selection boxes)
 * @param x0 Top-left X coordinate
 * @param y0 Top-left Y coordinate
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param t Thickness of the border
 * @param c 16-bit RGB565 color
 */
void draw_rect_outline(int x0, int y0, int w, int h, int t, uint16_t c)
{
    fill_rect(x0,             y0,             w, t, c);     /* Top */
    fill_rect(x0,             y0 + h - t,     w, t, c);     /* Bottom */
    fill_rect(x0,             y0,             t, h, c);     /* Left */
    fill_rect(x0 + w - t,     y0,             t, h, c);     /* Right */
}

/**
 * @brief Visual cursor indicating where the user is placing an object
 * @param cx Center X coordinate of the crosshair
 * @param cy Center Y coordinate of the crosshair
 * @param c 16-bit RGB565 color
 */
void draw_crosshair(int cx, int cy, uint16_t c)
{
    for (int i = -8; i <= 8; ++i) {
        draw_pixel(cx + i, cy,     c);
        draw_pixel(cx,     cy + i, c);
    }
}