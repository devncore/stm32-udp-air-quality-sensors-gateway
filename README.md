# Air Quality Server

```
  ┌─────────────────────────────────────────────────────────────────────────────┐
  │                        AIR QUALITY MONITORING SYSTEM                        │
  └─────────────────────────────────────────────────────────────────────────────┘

   SENSOR NODES                           SERVER                      OUTPUTS
  ══════════════                    ══════════════════             ══════════════

  ┌──────────┐                      ┌───────────────┐
  │ Sensor 1 │  ~~~ UDP/WiFi ~~~>   │               │             ┌────────────┐
  │  Room A  │                      │  ESP-01S      │  ─── SPI ──>│   OLED     │
  └──────────┘                      │  WiFi module  │             │  128×128   │
                                    │               │             │            │
  ┌──────────┐                      │  ┌─────────┐  │             │ ┌──┬──┬──┬──┐│
  │ Sensor 2 │  ~~~ UDP/WiFi ~~~>   │  │  UART2  │  │             │ │R1│R2│R3│R4││
  │  Room B  │                      │  └────┬────┘  │             │ │23│21│25│19││
  └──────────┘                      │       │       │             │ │48│55│40│62││
                                    │  ┌────▼────┐  │             │ │GD│PF│MD│VG││
  ┌──────────┐                      │  │  STM32  │  │             │ └──┴──┴──┴──┘│
  │ Sensor 3 │  ~~~ UDP/WiFi ~~~>   │  │F401RET6 │  │             └────────────┘
  │  Room C  │                      │  │         │  │
  └──────────┘                      │  │FreeRTOS │  │             ┌────────────┐
                                    │  │         │  │             │  !! ALARM  │
  ┌──────────┐                      │  └─────────┘  │  ─ future ─>│  (thresh.) │
  │ Sensor 4 │  ~~~ UDP/WiFi ~~~>   │               │             │            │
  │  Room D  │                      └───────────────┘             └────────────┘
  └──────────┘
      │
      └─ 10-byte binary frame: [ type | temp(float) | hum | IAQ | CRC16 ]
```

Embedded firmware for a centralized air quality monitoring station. The device receives sensor data from multiple wireless sensor nodes over UDP and displays temperature, humidity, and air quality index for up to 4 rooms on a local OLED screen.

---

## Features

- **Wireless reception** — listens for UDP datagrams on a fixed port via an ESP-01S WiFi module
- **Multi-room monitoring** — displays data from up to 4 sensor nodes simultaneously
- **Real-time OLED display** — shows temperature (°C), humidity (%), and air quality index for each room
- **Air quality classification** — 6-level IAQ index: PERFECT / VERY GOOD / GOOD / MEDIUM / BAD / VERY BAD
- **Alert indication** — inverse-video blinking when air quality exceeds threshold
- **Brokerless architecture** — sensors push UDP frames directly to the server, no MQTT broker needed
- **Frame integrity** — CRC16-CCITT validation on every received binary frame

---

## Technical Stack

| Layer | Technology |
|---|---|
| **MCU** | STM32F401RET6 — ARM Cortex-M4 @ 84 MHz, 512 KB Flash, 96 KB RAM |
| **Board** | NUCLEO-F401RE (built-in ST-Link v2-1 programmer) |
| **WiFi** | ESP-01S (ESP8266EX) — UART AT command interface |
| **Display** | SH1107 1.5" OLED — 128×128 px monochrome, SPI |
| **RTOS** | FreeRTOS (CMSIS-RTOS v2), heap_4, 1000 Hz tick |
| **Network protocol** | Plain UDP — 10-byte binary frames, no handshake |
| **Build system** | CMake + arm-none-eabi-gcc (C11) |
| **Peripheral layer** | STM32Cube HAL (CubeMX-generated) |

### Pin Mapping

| Peripheral | Pins | Function |
|---|---|---|
| USART2 | PA2 (TX), PA3 (RX) | ESP-01S AT commands / data |
| SPI1 | PA5 (SCK), PA7 (MOSI) | OLED display |
| GPIO | PB6 (CS), PA8 (DC), PA9 (RST) | OLED control |

> **Note:** PA9 is shared between OLED_RST and USART1_TX. A dedicated debug UART cannot be used simultaneously with the display reset line. See [doc/requirements/HARDWARE.md](doc/requirements/HARDWARE.md).

---

## Main Libraries

| Library | Location | Purpose |
|---|---|---|
| **STM32F4xx HAL** | `mxcube/Drivers/` | GPIO, UART, SPI, DMA, RCC peripheral drivers |
| **FreeRTOS** | `mxcube/Middlewares/Third_Party/FreeRTOS/` | RTOS kernel — tasks, queues, message buffers, heap_4 |
| **CMSIS-RTOS v2** | `mxcube/Middlewares/` | Portable RTOS API wrapper over FreeRTOS |
| **stm32-ssd1306** | `lib/stm32-ssd1306/` | OLED driver for SSD1306/SH1107, font rendering ([afiskon/stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306)) |

Custom application code under `src/`:

| Module | Location | Purpose |
|---|---|---|
| ESP8266 driver | `src/drivers/esp8266/` | AT command sequencing, WiFi init, UDP socket open |
| HAL interface | `src/hal/hal_interface.c` | UART abstraction used by the ESP8266 driver |
| UART RX | `src/hal/uart_rx.c` | Interrupt-driven `+IPD` frame parser |
| Network task | `src/app/network.c` | WiFi setup, frame reception loop |
| Frame parser | `src/app/frame_parser.c` | CRC16-CCITT computation and 10-byte binary frame decoding |
| Display task | `src/app/display.c` | OLED layout, sensor data rendering, IAQ classification |
| Sensor management | `src/app/displayed_sensor_management.c` | Active sensor slot allocation and timeout tracking |

---

## Requirements Documentation

| Document | Description |
|---|---|
| [doc/requirements/HARDWARE.md](doc/requirements/HARDWARE.md) | MCU selection rationale, pin allocation, BOM, WiFi and display wiring |
| [doc/requirements/SOFTWARE.md](doc/requirements/SOFTWARE.md) | RTOS task design, UDP protocol specification, AT command sequence, frame format |

---

## FreeRTOS Tasks

### `network_task` — HIGH priority (`osPriorityAboveNormal`, 2 KB stack)

**File:** [src/app/network.c](src/app/network.c)

Responsible for all WiFi communication. On startup it runs a blocking AT command sequence to configure the ESP-01S:

1. Test link (`AT`) and disable echo (`ATE0`)
2. Set station mode (`AT+CWMODE=1`)
3. Connect to the configured WiFi network (30 s timeout, retries on failure)
4. Open a UDP listener on `CONFIG_UDP_LOCAL_PORT` (default **4210**)

Once the socket is open it enables the UART interrupt receiver and enters an infinite loop. It blocks on a FreeRTOS **message buffer** waiting for frames posted by the UART ISR, delegates decoding to `frame_parser.c` (type byte check + CRC16-CCITT), then posts decoded `sensor_data_t` structs to the shared **sensor queue** consumed by the display task.

On any initialization failure it retries with a short delay (up to a full system reset if the ESP8266 cannot be reached).

---

### `display_task` — LOW priority (`osPriorityBelowNormal`, 2 KB stack)

**File:** [src/app/display.c](src/app/display.c)

Responsible for all OLED rendering. On startup it clears the framebuffer and draws three vertical separator lines dividing the 128×128 screen into 4 room columns.

It then blocks on the **sensor queue**. For each frame received:

- Calls `displayed_sensor_update()` to map the sender's room name to a display column index (0–3), allocating a new slot on first contact or returning the existing one
- Draws the room name label the first time a slot is assigned
- Overwrites temperature, humidity, and the IAQ label in the correct column
- Periodically calls `displayed_sensor_evaluate_timeout()` to free columns whose sensor has gone silent

**Screen layout** (128×128 px, 3 px margin, Font_6x8):

```
┌──────────┬──────────┬──────────┬──────────┐
│  Room 0  │  Room 1  │  Room 2  │  Room 3  │
│  23°C    │  21°C    │  25°C    │  19°C    │
│  48%     │  55%     │  40%     │  62%     │
│  GOOD    │  PERF    │  MED     │  V.GD    │
└──────────┴──────────┴──────────┴──────────┘
```

**IAQ classification:**

| Range | Label |
|---|---|
| 0 – 50 | PERF |
| 51 – 100 | V.GD |
| 101 – 150 | GOOD |
| 151 – 200 | MED |
| 201 – 300 | BAD |
| 301 – 500 | VBAD |

---

### Default task (CubeMX-generated, terminates on start)

Created by FreeRTOS at kernel start. Calls `app_main()` to initialize HAL wrappers, create the sensor queue, and spawn `network_task` and `display_task`, then exits via `osThreadExit()`.

---

## Data Flow: WiFi → Display

```
Sensor node (UDP client)
        │
        │  UDP datagram via WiFi
        ▼
  ESP-01S module
        │
        │  UART2 (PA3, 115200 baud)
        │  Raw AT response: "+IPD,10:<payload>"
        ▼
  UART2 RX interrupt  [ISR context]
  uart_rx.c — rx_process_byte()
    └─ "+IPD" state machine:
       IDLE → MATCH_IPD → PARSE_LEN → RECV_PAYLOAD
        │
        │  xMessageBufferSendFromISR()
        ▼
  x_message_buffer  (FreeRTOS message buffer)
        │
        │  xMessageBufferReceive()  [blocks network_task]
        ▼
  network_task  [HIGH priority]
  network.c
    └─ frame_parser.c — parse_sensor_frame()
         ├─ Validate type byte (0x01)
         ├─ Verify CRC16-CCITT over bytes [0..7]
         └─ Decode:
              bytes [1..4] → temperature (float)
              byte  [5]    → humidity (uint8, %)
              bytes [6..7] → IAQ index (uint16, 0–500)
        │
        │  osMessageQueuePut(g_sensor_queue)
        ▼
  g_sensor_queue  (FreeRTOS queue, capacity 8)
        │
        │  osMessageQueueGet()  [blocks display_task]
        ▼
  display_task  [LOW priority]
  display.c — display_update_sensor()
    ├─ displayed_sensor_management.c — displayed_sensor_update()
    │    └─ map room name → column index (0–3), allocate slot if new
    ├─ display_iaq_classify()  →  AIQ label string
    ├─ Format strings (snprintf)
    ├─ ssd1306_SetCursor / ssd1306_WriteString
    └─ ssd1306_UpdateScreen()
        │
        │  SPI1 (PA5=SCK, PA7=MOSI)
        │  + GPIO (PB6=CS, PA8=DC, PA9=RST)
        ▼
  SH1107 OLED controller
        │
        ▼
  128×128 monochrome display
```

### Binary frame format (10 bytes)

| Byte(s) | Field | Type |
|---|---|---|
| `[0]` | Frame type (`0x01`) | `uint8_t` |
| `[1..4]` | Temperature | `float` (little-endian) |
| `[5]` | Humidity (0–100) | `uint8_t` |
| `[6..7]` | IAQ index (0–500) | `uint16_t` (little-endian) |
| `[8..9]` | CRC16-CCITT over `[0..7]` | `uint16_t` (little-endian) |

---

## Testing

Unit tests run on the host (no embedded target required) using the [Unity](https://github.com/ThrowTheSwitch/Unity) C unit testing framework, vendored as a git submodule at `tests/unity/`.

Hardware and RTOS dependencies are replaced by thin stubs under `tests/stubs/`, so tests compile with a plain GCC toolchain and can run in any CI environment.

### Test suites

| Executable | Source | What it covers |
|---|---|---|
| `test_frame_parser` | [tests/test_frame_parser.c](tests/test_frame_parser.c) | `crc16_ccitt()` — known vector, empty input, single byte; `parse_sensor_frame()` — valid frame, wrong type byte, corrupted CRC, boundary IAQ values |
| `test_display` | [tests/test_display.c](tests/test_display.c) | `display_iaq_classify()` — equivalence classes (PERFECT → VERY BAD) and boundary values at every threshold (50/51, 100/101, 150/151, 200/201, 300/301) |
| `test_sensor_management` | [tests/test_sensor_management.c](tests/test_sensor_management.c) | `displayed_sensor_update()` — slot allocation, re-lookup, full table, overflow; `displayed_sensor_evaluate_timeout()` — no timeout before deadline, expiry, slot reuse, one-at-a-time freeing |

### Running tests locally

```bash
./run_tests.sh
```

This script runs `cmake --preset tests`, builds, and invokes `ctest --preset tests` from the `tests/` directory. Requires `cmake` ≥ 3.22, `ninja`, and `gcc`.

---

## Quality Gates (CI)

Two GitHub Actions workflows run on every push and pull request to `master`:

### Unit Tests — `.github/workflows/unit-tests.yml`

Installs a native GCC toolchain on `ubuntu-latest` and runs `run_tests.sh`. The workflow fails if any test executable reports a failure.

### Static Analysis — `.github/workflows/static-analysis.yml`

Installs `arm-none-eabi-gcc` and `clang-tidy`, generates `compile_commands.json` with CMake, then runs `run_clang_tidy.sh` against all project source files (third-party directories `lib/` and `mxcube/` are excluded).

The `.clang-tidy` configuration is MISRA C:2012 / CERT C / Barr Embedded C inspired and enables:

- `bugprone-*` — narrowing conversions, integer division, macro parentheses, unused return values, …
- `clang-analyzer-core.*` / `clang-analyzer-deadcode.*` / `clang-analyzer-security.*` — static analysis passes
- `readability-*` — braces around statements (MISRA 15.6), identifier naming (snake_case functions/variables, UPPER_CASE macros and enum constants, `_t` typedef suffix)
- `cert-err34-c`, `cert-flp30-c`, `cert-str34-c` — CERT C rules

Four checks are promoted to **errors** (build-breaking): `bugprone-assignment-in-if-condition`, `bugprone-suspicious-semicolon`, `readability-identifier-naming`, `readability-misleading-indentation`.

---

## Build

### Prerequisites

- `arm-none-eabi-gcc` toolchain
- `cmake` ≥ 3.22
- `ninja` or `make`

### Build commands

```bash
# Debug
cmake --preset debug
cmake --build build/debug

# Release
cmake --preset release
cmake --build build/release
```

Or use the provided script:

```bash
./build.sh
```

Output files are placed in `build/`: `.elf`, `.bin`, `.hex`, `.map`.

Flash with ST-Link via STM32CubeIDE, OpenOCD, or `st-flash`.

---

## Configuration

Key parameters in [include/app/config.h](include/app/config.h):

| Parameter | Default | Description |
|---|---|---|
| `CONFIG_WIFI_SSID` | `"YOUR_SSID"` | WiFi network name |
| `CONFIG_WIFI_PASSWORD` | `"YOUR_PASSWORD"` | WiFi password |
| `CONFIG_UDP_LOCAL_PORT` | `4210` | UDP listening port |
| `CONFIG_WIFI_CONNECT_TIMEOUT_MS` | `30000` | Connection timeout (ms) |
