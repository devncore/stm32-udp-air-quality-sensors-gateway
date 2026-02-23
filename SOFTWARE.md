# Software Requirements — UDP Server Display Unit

## Software Stack

- **RTOS:** FreeRTOS
- **WiFi driver:** AT command parser over UART
- **Network:** UDP over ESP-01S AT commands (no external library needed)
- **Display:** SH1107 SPI driver + simple text rendering (6x8 font)
- **Build:** STM32CubeIDE / CMake + arm-none-eabi-gcc

---

## FreeRTOS

FreeRTOS is used as the real-time operating system. It provides
preemptive multitasking, software timers, and synchronization primitives.

### Planned Tasks

| Task              | Priority | Trigger                  | Role                                                        |
|-------------------|----------|--------------------------|-------------------------------------------------------------|
| Network Task      | High     | On demand (UART IRQ)     | AT command driver + UDP receive, parses incoming sensor data |
| Display Task      | Low      | Periodic (every 500 ms)  | Refresh OLED with latest sensor data                        |
| Blink/Alert Task  | Low      | Periodic (every 500 ms)  | Toggle inverse video on air quality alert                   |

> **Network Task** merges WiFi/AT handling and UDP reception into a single task.
> It blocks on a notification/semaphore and wakes only when the UART ISR
> signals incoming data (e.g. `+IPD` from the ESP-01S).

### Configuration Notes

- Heap scheme: `heap_4` (coalescing allocator, suits variable-size allocs)
- Tick rate: 1000 Hz
- Minimal stack per task: 256 words (tuned per task after profiling)

---

## UDP Protocol

The server uses **plain UDP** (no ACK, no retransmission) to receive
sensor data. This is a brokerless architecture — sensors send datagrams
directly to the server. No external library is required; the ESP-01S
handles the UDP/IP stack internally via AT commands.

### Why UDP without ACK?

- Sensor data is periodic; a missed packet is replaced by the next one
- Minimal RAM footprint (single receive buffer, no per-client connection state)
- Simplest AT command integration (no connection management)

### ESP-01S AT Command Setup

```
AT+CIPMUX=0
AT+CIPSTART="UDP","0.0.0.0",<remote_port>,<local_port>,2
```

- Mode `2`: the destination changes automatically to the last received
  remote IP/port (useful for receiving from multiple sensors)
- Incoming data arrives as `+IPD,<len>:<payload>` on UART

### Packet Format

Each sensor sends all its readings in a single ASCII datagram:

```
<room_name>:<temperature>,<humidity>,<air_quality>\n
```

| Field           | Description                    | Unit / Range | Example   |
|-----------------|--------------------------------|--------------|-----------|
| `room_name`     | Human-readable room identifier | string       | `kitchen` |
| `temperature`   | Temperature                    | °C           | `23.5`    |
| `humidity`      | Relative humidity              | %            | `48.2`    |
| `air_quality`   | IAQ index (Indoor Air Quality) | 0 – 500      | `127`     |

Example payloads:
```
kitchen:23.5,48.2,127
bedroom:19.8,55.0,42
garage:12.1,70.3,310
```

### Interaction Diagram

```
  Sensor 1 --- UDP datagram ("kitchen:23.5,48.2,127\n") ---\
  Sensor 2 --- UDP datagram ("bedroom:19.8,55.0,42\n")  ----|---> STM32 Server
  Sensor 3 --- UDP datagram ("garage:12.1,70.3,310\n")  ---/      (listening on fixed port)
```

No handshake, no subscription, no broker. The server simply listens and
parses whatever arrives.
