/**
* @file lcd.c
 * @brief LCD hardware blit: pushing the framebuffer to the physical display.
 * @author Danylo Viakhiriev, Igor Podlesnyy
 * * BE5B35APO - Computer Architectures
 * Semestral Project (preliminary task)
 */

#include "memgame.h"

/**
 * @brief Sends the entire framebuffer to the parallel LCD controller
 * @param parlcd_mem_base Base memory address for the LCD controller
 */
void lcd_update(unsigned char *parlcd_mem_base)
{
    /* 1. Reset the Column Address (Width) bounds to 0 -> 479.
       This overrides the bootloader's small drawing window. */
    parlcd_write_cmd(parlcd_mem_base, 0x2A);
    parlcd_write_data(parlcd_mem_base, 0);
    parlcd_write_data(parlcd_mem_base, 0);
    parlcd_write_data(parlcd_mem_base, (SCREEN_W - 1) >> 8);
    parlcd_write_data(parlcd_mem_base, (SCREEN_W - 1) & 0xFF);

    /* 2. Reset the Page Address (Height) bounds to 0 -> 319 */
    parlcd_write_cmd(parlcd_mem_base, 0x2B);
    parlcd_write_data(parlcd_mem_base, 0);
    parlcd_write_data(parlcd_mem_base, 0);
    parlcd_write_data(parlcd_mem_base, (SCREEN_H - 1) >> 8);
    parlcd_write_data(parlcd_mem_base, (SCREEN_H - 1) & 0xFF);

    /* 3. Initiate the Memory Write command */
    parlcd_write_cmd(parlcd_mem_base, 0x2C);

    /* 4. Push the populated framebuffer to the physical hardware */
    for (int i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        parlcd_write_data(parlcd_mem_base, fb[i]);
    }
}