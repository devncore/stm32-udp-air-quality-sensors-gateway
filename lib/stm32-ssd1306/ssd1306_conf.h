/**
 * @file ssd1306_conf.h
 * @brief SSD1306/SH1107 library configuration for Air Quality Server
 *
 * Hardware: STM32F401RE + SH1107 1.5" OLED 128x128
 *   SPI1  — PA5 (SCK), PA7 (MOSI)
 *   PB6   — OLED_CS  (chip select,  active-low)
 *   PA8   — OLED_DC  (data/command, high=data)
 *   PA9   — OLED_RST (reset,        active-low)
 */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

/* MCU family — selects the correct HAL header inside ssd1306.h */
#define STM32F4

/* Communication bus */
#define SSD1306_USE_SPI

/* SPI peripheral handle (defined in mxcube/Core/Src/main.c) */
#define SSD1306_SPI_PORT        hspi1

/* Chip-select pin — PB6 */
#define SSD1306_CS_Port         GPIOB
#define SSD1306_CS_Pin          GPIO_PIN_6

/* Data/Command pin — PA8 */
#define SSD1306_DC_Port         GPIOA
#define SSD1306_DC_Pin          GPIO_PIN_8

/* Reset pin — PA9 */
#define SSD1306_Reset_Port      GPIOA
#define SSD1306_Reset_Pin       GPIO_PIN_9

/* Screen resolution — SH1107 1.5" is 128x128 */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          128

/* Fonts to include */
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

#endif /* __SSD1306_CONF_H__ */
