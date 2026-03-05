/**
 * @file frame_parser.h
 * @brief Sensor frame decoding — CRC16-CCITT and binary payload parsing.
 *
 * These utilities are kept in a separate translation unit so they can be
 * compiled and tested on the host without any RTOS or hardware dependencies.
 */

#ifndef FRAME_PARSER_H
#define FRAME_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app/config.h"
#include "app/network_data.h"

/**
 * @brief CRC16-CCITT (poly 0x1021, init 0xFFFF) over @p len bytes.
 *
 * @param data  Pointer to the input bytes.
 * @param len   Number of bytes to process.
 * @return      16-bit CRC.
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

/**
 * @brief Parse a 10-byte binary sensor frame into a sensor_data_t.
 *
 * Frame layout (FRAME_1_PAYLOAD_LEN = 10 bytes):
 *   [0]     type     : uint8_t  — must be 0x01
 *   [1..4]  temp     : float    (little-endian IEEE 754)
 *   [5]     humidity : uint8_t  (0–100 %)
 *   [6..7]  iaq      : uint16_t (little-endian)
 *   [8..9]  crc16    : uint16_t — CRC16-CCITT over bytes [0..7]
 *
 * @param buf  Raw frame bytes (exactly FRAME_1_PAYLOAD_LEN bytes).
 * @param out  Output structure populated on success.
 */
void parse_sensor_frame(const uint8_t *buf, sensor_data_t *out);

/**
 * @brief validate frame type
 *
 * @param type  frame type
 * @return     true if type == 0x01, false otherwise.
 */
bool validate_type(uint8_t type);

/**
 * @brief validate frame crc
 *
 * @param buf  Raw frame bytes (exactly FRAME_1_PAYLOAD_LEN bytes).
 * @return     true on crc success, false otherwise
 */
bool validate_crc(const uint8_t *buf);

#endif /* FRAME_PARSER_H */
