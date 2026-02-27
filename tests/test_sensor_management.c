/**
 * @file test_sensor_management.c
 * @brief Unit tests for displayed_sensor_update().
 *
 */

#include "unity.h"
#include "app/displayed_sensor_management.h"
#include "stubs/freertos_test_helpers.h"

void setUp(void)
{
    displayed_sensor_management_reset();
    freertos_stub_set_tick(0U);
}

void tearDown(void) {}

/* ── Registration ────────────────────────────────────────────────────────── */

void test_first_sensor_gets_index_0(void)
{
    TEST_ASSERT_EQUAL_UINT8(0U, displayed_sensor_update("RoomA"));
}

void test_known_sensor_returns_same_index(void)
{
    displayed_sensor_update("RoomA");
    TEST_ASSERT_EQUAL_UINT8(0U, displayed_sensor_update("RoomA"));
}

void test_second_new_sensor_gets_index_1(void)
{
    displayed_sensor_update("RoomA");
    TEST_ASSERT_EQUAL_UINT8(1U, displayed_sensor_update("RoomB"));
}

void test_four_sensors_fill_all_indexes(void)
{
    TEST_ASSERT_EQUAL_UINT8(0U, displayed_sensor_update("RoomA"));
    TEST_ASSERT_EQUAL_UINT8(1U, displayed_sensor_update("RoomB"));
    TEST_ASSERT_EQUAL_UINT8(2U, displayed_sensor_update("RoomC"));
    TEST_ASSERT_EQUAL_UINT8(3U, displayed_sensor_update("RoomD"));
}

void test_fifth_sensor_returns_no_index_available(void)
{
    displayed_sensor_update("RoomA");
    displayed_sensor_update("RoomB");
    displayed_sensor_update("RoomC");
    displayed_sensor_update("RoomD");
    TEST_ASSERT_EQUAL_UINT8(NO_ACTIVE_INDEX_AVAILABLE,
                             displayed_sensor_update("RoomE"));
}

/* ── Re-lookup ───────────────────────────────────────────────────────────── */

void test_relookup_after_full_table(void)
{
    displayed_sensor_update("RoomA");
    displayed_sensor_update("RoomB");
    displayed_sensor_update("RoomC");
    displayed_sensor_update("RoomD");

    /* All four rooms must still resolve to their original index */
    TEST_ASSERT_EQUAL_UINT8(0U, displayed_sensor_update("RoomA"));
    TEST_ASSERT_EQUAL_UINT8(1U, displayed_sensor_update("RoomB"));
    TEST_ASSERT_EQUAL_UINT8(2U, displayed_sensor_update("RoomC"));
    TEST_ASSERT_EQUAL_UINT8(3U, displayed_sensor_update("RoomD"));
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_first_sensor_gets_index_0);
    RUN_TEST(test_known_sensor_returns_same_index);
    RUN_TEST(test_second_new_sensor_gets_index_1);
    RUN_TEST(test_four_sensors_fill_all_indexes);
    RUN_TEST(test_fifth_sensor_returns_no_index_available);
    RUN_TEST(test_relookup_after_full_table);

    return UNITY_END();
}
