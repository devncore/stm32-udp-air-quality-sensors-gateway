/**
 * @file ssd1306.h  [STUB]
 * @brief Minimal SSD1306/SH1107 driver stub for host-side unit tests.
 *
 * Shadows the real lib/stm32-ssd1306/ssd1306.h which has STM32 SPI
 * dependencies.  All functions are no-ops implemented in ssd1306_stubs.c.
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stddef.h>
#include <stdint.h>

typedef enum { Black = 0, White = 1 } ssd1306_Color_t;

typedef struct {
    uint8_t        FontWidth;
    uint8_t        FontHeight;
    const uint16_t *data;
} FontDef;

void ssd1306_Init(void);
void ssd1306_Fill(ssd1306_Color_t color);
void ssd1306_UpdateScreen(void);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
char ssd1306_WriteChar(char ch, FontDef font, ssd1306_Color_t color);
char ssd1306_WriteString(char *str, FontDef font, ssd1306_Color_t color);
void ssd1306_FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                            ssd1306_Color_t color);
void ssd1306_Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                  ssd1306_Color_t color);

#endif /* SSD1306_H */
