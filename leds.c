/**
* @file leds.c
 * @brief Peripheral helpers for the MZ_APO RGB LEDs and the 32-LED line.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/**
 * @brief Sets the color of the two RGB LEDs on the MZ_APO board
 * @param mem_base Base memory address of the SPILED peripheral
 * @param rgb24 24-bit RGB888 color value
 */
void set_rgb_leds(unsigned char *mem_base, uint32_t rgb24)
{
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB1_o) = rgb24;
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB2_o) = rgb24;
}

/**
 * @brief Writes directly to the 32-bit LED line register
 * @param mem_base Base memory address of the SPILED peripheral
 * @param v 32-bit state to set on the LED line
 */
void set_led_line(unsigned char *mem_base, uint32_t v)
{
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_LINE_o) = v;
}

/**
 * @brief Lights up the lowest `n` bits of the 32-LED line for visual progress
 * @param mem_base Base memory address of the SPILED peripheral
 * @param n Number of lowest bits/LEDs to illuminate
 */
void show_led_bar(unsigned char *mem_base, int n)
{
    if (n <= 0)        { set_led_line(mem_base, 0); return; }
    if (n >= 32)       { set_led_line(mem_base, 0xFFFFFFFFu); return; }
    set_led_line(mem_base, (1u << n) - 1u); /* Creates a mask of N ones */
}