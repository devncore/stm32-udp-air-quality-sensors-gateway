/**
 * @file ssd1306_stubs.c
 * @brief No-op SSD1306 driver implementations for host-side unit tests.
 */

#include "ssd1306.h"
#include "ssd1306_fonts.h"

FontDef Font_6x8 = {6U, 8U, NULL};

void ssd1306_Init(void) {}
void ssd1306_Fill(ssd1306_Color_t color) { (void)color; }
void ssd1306_UpdateScreen(void) {}
void ssd1306_SetCursor(uint8_t x, uint8_t y) { (void)x; (void)y; }

char ssd1306_WriteChar(char ch, FontDef font, ssd1306_Color_t color)
{
    (void)font; (void)color;
    return ch;
}

char ssd1306_WriteString(char *str, FontDef font, ssd1306_Color_t color)
{
    (void)font; (void)color;
    if (str != NULL) {
        return str[0];
    }
    return 0;
}

void ssd1306_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                            ssd1306_Color_t color)
{
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
}

void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                  ssd1306_Color_t color)
{
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
}
