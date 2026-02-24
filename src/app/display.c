/**
 * @file display.c
 * @brief OLED display module — layout, rendering, and task stub.
 *
 * ── Screen geometry ──────────────────────────────────────────────────────────
 *
 *   Screen : 128 × 128 px (SH1107)
 *   Margin : 3 px on all sides
 *   Content: x ∈ [3, 124], y ∈ [3, 124]  →  122 × 122 px
 *
 * ── Horizontal layout ────────────────────────────────────────────────────────
 *
 *   4 sensor columns with 1-px separators between them.
 *   Column width is 1/4 of available content (cols 0–2 get 30 px each,
 *   col 3 fills the remainder with 29 px):
 *
 *     col 0 : x =   3 … 32   (30 px)
 *     sep   : x =  33
 *     col 1 : x =  34 … 63   (30 px)
 *     sep   : x =  64
 *     col 2 : x =  65 … 94   (30 px)
 *     sep   : x =  95
 *     col 3 : x =  96 … 124  (29 px)
 *     margin: x = 125 … 127
 *
 *   Verification: 3 + 30 + 1 + 30 + 1 + 30 + 1 + 29 + 3 = 128  ✓
 *
 * ── Vertical layout ──────────────────────────────────────────────────────────
 *
 *   Font: Font_6x8 (6 × 8 px per glyph)
 *   4 fields + 5 equal gaps fill 122 px:
 *     5 × 18 gap + 4 × 8 font = 90 + 32 = 122 px  ✓
 *
 *     field 0  room name   y =  21   (static, drawn once)
 *     field 1  temp °C     y =  47
 *     field 2  humidity %  y =  73
 *     field 3  air quality y =  99
 */

#include "app/display.h"
#include "app/displayed_sensor_management.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "cmsis_os.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*============================================================================
 * Layout constants
 *============================================================================*/

#define DISP_W          128u
#define DISP_H          128u
#define MARGIN          3u

#define CONTENT_X0      MARGIN                               /*   3 */
#define CONTENT_Y0      MARGIN                               /*   3 */
#define CONTENT_W       (DISP_W - 2u * MARGIN)              /* 122 */
#define CONTENT_H       (DISP_H - 2u * MARGIN)              /* 122 */

#define N_AREAS         SENSOR_MAX_ROOMS                     /*   4 */
#define SEP_W           1u

/* Nominal column width for cols 0–2; col 3 fills the remaining pixels. */
#define AREA_W          30u
#define AREA_STRIDE     (AREA_W + SEP_W)                    /*  31 */

/* Font_6x8 glyph dimensions */
#define FONT_W          6u
#define FONT_H          8u

#define N_FIELDS        4u

/*
 * Equal vertical gap between / around text fields:
 *   FIELD_GAP × (N_FIELDS + 1)  +  FONT_H × N_FIELDS  =  CONTENT_H
 *       18    ×      5          +     8    ×    4       =  122 px  ✓
 */
#define FIELD_GAP   ((CONTENT_H - N_FIELDS * FONT_H) / (N_FIELDS + 1u))  /* 18 */

/* Field indices */
#define FIELD_ROOM  0u
#define FIELD_TEMP  1u
#define FIELD_HUM   2u
#define FIELD_IAQ   3u

/*============================================================================
 * Air quality label table
 *============================================================================*/

static const char * const k_aq_label[] = {
    "PERF",   /* AIR_QUALITY_PERFECT   (IAQ   0– 50) */
    "V.GD",   /* AIR_QUALITY_VERY_GOOD (IAQ  51–100) */
    "GOOD",   /* AIR_QUALITY_GOOD      (IAQ 101–150) */
    "MED",    /* AIR_QUALITY_MEDIUM    (IAQ 151–200) */
    "BAD",    /* AIR_QUALITY_BAD       (IAQ 201–300) */
    "VBAD",   /* AIR_QUALITY_VERY_BAD  (IAQ 301–500) */
};

/*============================================================================
 * Internal helpers
 *============================================================================*/

/** X pixel coordinate of the left edge of column @p col. */
static inline uint8_t col_x(uint8_t col)
{
    return (uint8_t)(CONTENT_X0 + (uint16_t)col * AREA_STRIDE);
}

/**
 * Pixel width of column @p col.
 * Cols 0–2 return AREA_W (30 px); col 3 returns the space remaining before
 * the right margin (29 px).
 */
static inline uint8_t col_w(uint8_t col)
{
    uint8_t x     = col_x(col);
    uint8_t avail = (uint8_t)(DISP_W - MARGIN - x);
    return (avail < AREA_W) ? avail : (uint8_t)AREA_W;
}

/** Y pixel coordinate of the top of field @p field. */
static inline uint8_t field_y(uint8_t field)
{
    return (uint8_t)(CONTENT_Y0 + FIELD_GAP * (field + 1u) + FONT_H * field);
}

/**
 * Erase the rectangle occupied by one text field, then move the cursor
 * to its top-left corner, ready for the next ssd1306_WriteString() call.
 */
static void clear_and_set_cursor(uint8_t x, uint8_t y, uint8_t w)
{
    ssd1306_FillRectangle(x, y,
                          (uint8_t)(x + w - 1u),
                          (uint8_t)(y + FONT_H - 1u),
                          Black);
    ssd1306_SetCursor(x, y);
}

/**
 * Write @p str at the current cursor position, truncating to fit within
 * @p w pixels (floor(@p w / FONT_W) glyphs).
 *
 * A local non-const buffer is used because ssd1306_WriteString() takes
 * a non-const @c char* parameter.
 */
static void write_clipped(const char *str, uint8_t w)
{
    uint8_t max_chars = w / FONT_W;
    if (max_chars == 0u) {
        return;
    }

    char buf[8];
    size_t len = strnlen(str, (size_t)max_chars);
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1u;
    }
    memcpy(buf, str, len);
    buf[len] = '\0';

    ssd1306_WriteString(buf, Font_6x8, White);
}

/*============================================================================
 * Public: air quality classification
 *============================================================================*/

air_quality_t display_iaq_classify(uint16_t iaq)
{
    if (iaq <=  50u) return AIR_QUALITY_PERFECT;
    if (iaq <= 100u) return AIR_QUALITY_VERY_GOOD;
    if (iaq <= 150u) return AIR_QUALITY_GOOD;
    if (iaq <= 200u) return AIR_QUALITY_MEDIUM;
    if (iaq <= 300u) return AIR_QUALITY_BAD;
    return AIR_QUALITY_VERY_BAD;
}

/*============================================================================
 * Public: display API
 *============================================================================*/

void display_init(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);

    /* Three 1-px vertical separators spanning the full content height */
    for (uint8_t i = 1u; i < N_AREAS; i++) {
        uint8_t x_sep = (uint8_t)(col_x(i) - 1u);
        ssd1306_Line(x_sep, (uint8_t)CONTENT_Y0,
                     x_sep, (uint8_t)(CONTENT_Y0 + CONTENT_H - 1u),
                     White);
    }

    ssd1306_UpdateScreen();
}

void display_draw_room(uint8_t col, const char *name)
{
    if (col >= N_AREAS || name == NULL) {
        return;
    }

    ssd1306_SetCursor(col_x(col), field_y(FIELD_ROOM));
    write_clipped(name, col_w(col));

    ssd1306_UpdateScreen();
}

void display_update_sensor(uint8_t col, const sensor_data_t *data)
{
    if (col >= N_AREAS || data == NULL || !data->valid) {
        return;
    }

    const uint8_t x = col_x(col);
    const uint8_t w = col_w(col);
    char buf[8];

    /* Temperature  —  e.g. "23°C"
     * Split string literal prevents the 'C' from extending the \xb0 hex
     * escape sequence (\xb0C would be parsed as 0xB0C, overflowing char). */
    clear_and_set_cursor(x, field_y(FIELD_TEMP), w);
    snprintf(buf, sizeof(buf), "%d\xb0" "C", (int)data->temperature);
    ssd1306_WriteString(buf, Font_6x8, White);

    /* Humidity  —  e.g. "48%" */
    clear_and_set_cursor(x, field_y(FIELD_HUM), w);
    snprintf(buf, sizeof(buf), "%d%%", (int)data->humidity);
    ssd1306_WriteString(buf, Font_6x8, White);

    /* Air quality label */
    air_quality_t aq = display_iaq_classify(data->iaq);
    clear_and_set_cursor(x, field_y(FIELD_IAQ), w);
    write_clipped(k_aq_label[aq], w);

    ssd1306_UpdateScreen();
}

/*============================================================================
 * FreeRTOS task
 *============================================================================*/

void display_task(void *argument)
{
    (void)argument;

    display_init();

    /*
     * TODO: wire up g_sensor_queue once the OS data path is ready.
     *
     * When the queue is live, replace the delay loop below with:
     */
    static bool room_drawn[SENSOR_MAX_ROOMS] = {false};
     for (;;) {
         sensor_data_t data;
         osMessageQueueGet(g_sensor_queue, &data, NULL, osWaitForever);

         // if sensor is referenced as 'active', then we update the display.
         // Otherwise, incoming data is discarded.
         const uint8_t col = displayed_sensor_update(data.room);
         if(col!=NO_ACTIVE_INDEX_AVAILABLE)
         {
            if (!room_drawn[col]) {
                display_draw_room(col, data.room);
                room_drawn[col] = true;
            }
            display_update_sensor(col, &data);
        }
     }
}
