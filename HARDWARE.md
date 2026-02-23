# Hardware Choices — MQTT Server Display Unit

## Overview

This board acts as an MQTT-connected display unit: it connects to WiFi,
subscribes to MQTT topics from up to 4 sensor modules, and shows their
data on a local screen.

---

## MCU — NUCLEO-F401RE

| Parameter       | Value                    |
|-----------------|--------------------------|
| MCU             | STM32F401RET6            |
| Core            | ARM Cortex-M4 @ 84 MHz  |
| Flash           | 512 KB                   |
| RAM             | 96 KB                    |
| Board           | NUCLEO-F401RE            |
| Price (approx)  | ~13 USD                  |
| USB             | Micro-USB (ST-Link v2-1) |

**Rationale:** Cheapest NUCLEO with enough Flash/RAM for the full stack
(MQTT parsing, AT command driver, SPI display). Cortex-M4 is comfortable
for this workload. Well supported by STM32CubeIDE / HAL.

---

## WiFi Module — ESP-01S (ESP8266)

| Parameter       | Value                         |
|-----------------|-------------------------------|
| Chip            | ESP8266EX                     |
| Interface       | UART (AT commands)            |
| Voltage         | 3.3V                          |
| Wiring          | VCC, GND, TX, RX (4 wires)   |
| Price (approx)  | ~2-3 USD                      |

**Rationale:** No NUCLEO board offers built-in WiFi at a low price point.
The ESP-01S is the cheapest external WiFi option. It runs stock AT firmware
so the STM32 drives it over UART — no need to program the ESP separately.

### UART Connection to NUCLEO

| ESP-01S Pin | NUCLEO Pin | Function     |
|-------------|------------|--------------|
| TX          | PA3        | USART2_RX    |
| RX          | PA2        | USART2_TX    |
| VCC         | 3V3        | Power        |
| GND         | GND        | Ground       |
| CH_PD (EN)  | 3V3        | Chip enable  |

> **Note:** USART2 is the default on NUCLEO-F401RE (directly routed to
> ST-Link virtual COM as well). An alternative USART (e.g. USART1 PA9/PA10
> or USART6 PA11/PA12) can be used to keep USART2 free for debug printf.

---

## Display — SH1107 1.5" OLED 128x128 (SPI)

| Parameter       | Value                    |
|-----------------|--------------------------|
| Controller      | SH1107                   |
| Resolution      | 128 x 128 pixels         |
| Type            | OLED, monochrome (white) |
| Interface       | SPI (4-wire + DC + RST)  |
| Voltage         | 3.3V                    |
| Price (approx)  | ~6 USD                   |

**Rationale:** 2x the pixels of a 128x64 screen. Each of the 4 sensor
blocks gets a 128x32 zone — enough for sensor name, 3 values
(temperature, humidity, air quality), and a warning indicator.
SPI is faster than I2C and frees up the I2C bus if needed later.
Driver is in the same family as SSD1306 (similar command set).

### Air Quality Warning

When air quality exceeds a threshold, the corresponding sensor block
switches to **inverse video** (white background, black text) and
**blinks** at ~2 Hz (software timer toggles the region). This gives a
clear visual alert without extra hardware (no LED needed).

### SPI Connection to NUCLEO

| OLED Pin | NUCLEO Pin | Function       |
|----------|------------|----------------|
| SCK      | PA5        | SPI1_SCK       |
| MOSI     | PA7        | SPI1_MOSI      |
| CS       | PB6        | GPIO (CS)      |
| DC       | PA8        | GPIO (Data/Cmd)|
| RST      | PA9        | GPIO (Reset)   |
| VCC      | 3V3        | Power          |
| GND      | GND        | Ground         |

---

## Pin Allocation Summary

| Peripheral | Pins Used              | STM32 Peripheral |
|------------|------------------------|------------------|
| ESP-01S    | PA2 (TX), PA3 (RX)    | USART2           |
| OLED SPI   | PA5, PA7, PB6, PA8, PA9 | SPI1 + GPIO    |
| Debug UART | PA9, PA10 (*)         | USART1           |

> (*) PA9 is shared between OLED RST and USART1_TX — if debug UART is
> needed on USART1, move OLED RST to another free pin (e.g. PB5).

---

## BOM Summary

| Item              | Qty | Unit Price | Total  |
|-------------------|-----|------------|--------|
| NUCLEO-F401RE     | 1   | ~13 USD    | 13 USD |
| ESP-01S           | 1   | ~3 USD     | 3 USD  |
| SH1107 1.5" OLED  | 1  | ~6 USD     | 6 USD  |
| **Total**         |     |            | **~22 USD** |

---

## Software Stack

See [SOFTWARE.md](SOFTWARE.md) for full software requirements.
