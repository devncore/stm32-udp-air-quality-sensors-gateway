/**
 * @file test_frame_parser.c
 * @brief Unit tests for crc16_ccitt() and parse_sensor_frame().
 */

#include "unity.h"
#include "app/frame_parser.h"

#include <string.h>

void setUp(void)    {}
void tearDown(void) {}

/* ── Helper ──────────────────────────────────────────────────────────────── */

/**
 * Build a complete 10-byte frame, computing and appending the CRC.
 * Relies on crc16_ccitt() being correct — covered by its own tests below.
 */
static void build_frame(uint8_t *buf, float temp, uint8_t hum, uint16_t iaq)
{
    buf[0] = 0x01U;
    memcpy(buf + 1, &temp, sizeof(float));
    buf[5] = hum;
    buf[6] = (uint8_t)(iaq & 0xFFU);
    buf[7] = (uint8_t)(iaq >> 8U);
    uint16_t crc = crc16_ccitt(buf, FRAME_1_PAYLOAD_LEN - 2U);
    buf[8] = (uint8_t)(crc & 0xFFU);
    buf[9] = (uint8_t)(crc >> 8U);
}

/* ── crc16_ccitt ─────────────────────────────────────────────────────────── */

void test_crc16_ccitt_known_vector(void)
{
    /* CRC-16/CCITT-FALSE standard check value: CRC("123456789") = 0x29B1 */
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    TEST_ASSERT_EQUAL_HEX16(0x29B1U, crc16_ccitt(data, sizeof(data)));
}

void test_crc16_ccitt_empty_input(void)
{
    /* No bytes processed → CRC stays at the init value 0xFFFF */
    const uint8_t dummy = 0U;
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, crc16_ccitt(&dummy, 0U));
}

void test_crc16_ccitt_single_zero_byte(void)
{
    /* CRC16-CCITT(0x00) with init 0xFFFF = 0xE1F0 (hand-verified) */
    const uint8_t data[] = {0x00U};
    TEST_ASSERT_EQUAL_HEX16(0xE1F0U, crc16_ccitt(data, 1U));
}

/* ── parse_sensor_frame ──────────────────────────────────────────────────── */

void test_parse_valid_frame(void)
{
    uint8_t buf[FRAME_1_PAYLOAD_LEN];
    const float    expected_temp = 25.5f;
    const uint8_t  expected_hum  = 60U;
    const uint16_t expected_iaq  = 100U;

    build_frame(buf, expected_temp, expected_hum, expected_iaq);

    sensor_data_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_TRUE(parse_sensor_frame(buf, &out));
    TEST_ASSERT_TRUE(out.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected_temp, out.temperature);
    TEST_ASSERT_EQUAL_UINT8(expected_hum,  (uint8_t)out.humidity);
    TEST_ASSERT_EQUAL_UINT16(expected_iaq, out.iaq);
}

void test_parse_invalid_type_byte(void)
{
    uint8_t buf[FRAME_1_PAYLOAD_LEN];
    build_frame(buf, 20.0f, 50U, 80U);
    buf[0] = 0x02U; /* wrong type — CRC now also invalid, but type is checked first */

    sensor_data_t out;
    TEST_ASSERT_FALSE(parse_sensor_frame(buf, &out));
}

void test_parse_corrupted_crc(void)
{
    uint8_t buf[FRAME_1_PAYLOAD_LEN];
    build_frame(buf, 20.0f, 50U, 80U);
    buf[8] ^= 0xFFU; /* flip all bits in the CRC low byte */

    sensor_data_t out;
    TEST_ASSERT_FALSE(parse_sensor_frame(buf, &out));
}

void test_parse_zero_iaq(void)
{
    uint8_t buf[FRAME_1_PAYLOAD_LEN];
    build_frame(buf, 22.0f, 45U, 0U);

    sensor_data_t out;
    TEST_ASSERT_TRUE(parse_sensor_frame(buf, &out));
    TEST_ASSERT_EQUAL_UINT16(0U, out.iaq);
}

void test_parse_max_iaq(void)
{
    uint8_t buf[FRAME_1_PAYLOAD_LEN];
    build_frame(buf, 30.0f, 80U, 500U);

    sensor_data_t out;
    TEST_ASSERT_TRUE(parse_sensor_frame(buf, &out));
    TEST_ASSERT_EQUAL_UINT16(500U, out.iaq);
}

void test_parse_sets_valid_flag(void)
{
    uint8_t buf[FRAME_1_PAYLOAD_LEN];
    build_frame(buf, 21.0f, 55U, 120U);

    sensor_data_t out;
    out.valid = false;
    parse_sensor_frame(buf, &out);
    TEST_ASSERT_TRUE(out.valid);
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_crc16_ccitt_known_vector);
    RUN_TEST(test_crc16_ccitt_empty_input);
    RUN_TEST(test_crc16_ccitt_single_zero_byte);
    RUN_TEST(test_parse_valid_frame);
    RUN_TEST(test_parse_invalid_type_byte);
    RUN_TEST(test_parse_corrupted_crc);
    RUN_TEST(test_parse_zero_iaq);
    RUN_TEST(test_parse_max_iaq);
    RUN_TEST(test_parse_sets_valid_flag);

    return UNITY_END();
}
