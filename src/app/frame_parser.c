/**
 * @file frame_parser.c
 * @brief Sensor frame decoding — CRC16-CCITT and binary payload parsing.
 */

#include "app/frame_parser.h"
#include "app/error_manager.h"

#include <stdbool.h>
#include <string.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

bool validate_type(uint8_t type)
{
    // only type 1 is accepted for now
    return type == 0x01;
}

bool validate_crc(const uint8_t *buf)
{
    uint16_t crc_calc = crc16_ccitt(buf, FRAME_1_PAYLOAD_LEN - 2u);
    uint16_t crc_recv = (uint16_t)buf[8] | ((uint16_t)buf[9] << 8);
    const bool crc_valid = crc_calc == crc_recv;
    if(!crc_valid)
    {
        error_set(ERROR_SENSOR_FRAME_CRC_INVALID);
    }
    return crc_valid;
}

void parse_sensor_frame(const uint8_t *buf, sensor_data_t *out)
{
    memcpy(&out->temperature, (const void *)(buf + 1), sizeof(float));
    out->humidity = (float)buf[5];
    out->iaq      = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
    out->valid    = true;
}
