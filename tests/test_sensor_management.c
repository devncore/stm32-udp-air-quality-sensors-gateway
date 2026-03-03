/**
 * @file test_sensor_management.c
 * @brief Unit tests for displayed_sensor_update() and displayed_sensor_evaluate_timeout().
 *
 */

#include "unity.h"
#include "app/displayed_sensor_management.h"
#include "app/config.h"
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

/* ── Timeout ─────────────────────────────────────────────────────────────── */

void test_no_timeout_before_period(void)
{
    displayed_sensor_update("RoomA");                        /* registered at tick 0 */
    freertos_stub_set_tick(DISPLAY_SENSOR_TIMEOUT_MS - 1U); /* one tick before expiry */
    TEST_ASSERT_EQUAL_UINT8(NO_TIMEOUT, displayed_sensor_evaluate_timeout());
}

void test_timeout_at_period_returns_index(void)
{
    displayed_sensor_update("RoomA");                   /* slot 0, registered at tick 0 */
    freertos_stub_set_tick(DISPLAY_SENSOR_TIMEOUT_MS);  /* exactly at expiry */
    TEST_ASSERT_EQUAL_UINT8(0U, displayed_sensor_evaluate_timeout());
}

void test_timeout_frees_slot_for_reuse(void)
{
    displayed_sensor_update("RoomA");                  /* slot 0 */
    freertos_stub_set_tick(DISPLAY_SENSOR_TIMEOUT_MS);
    displayed_sensor_evaluate_timeout();               /* slot 0 freed */

    /* New sensor must claim the freed slot 0 */
    TEST_ASSERT_EQUAL_UINT8(0U, displayed_sensor_update("RoomB"));
}

void test_only_one_timeout_per_evaluate_call(void)
{
    displayed_sensor_update("RoomA");  /* slot 0 */
    displayed_sensor_update("RoomB");  /* slot 1 */
    freertos_stub_set_tick(DISPLAY_SENSOR_TIMEOUT_MS);

    /* Each call frees exactly one slot */
    TEST_ASSERT_NOT_EQUAL(NO_TIMEOUT, displayed_sensor_evaluate_timeout());
    TEST_ASSERT_NOT_EQUAL(NO_TIMEOUT, displayed_sensor_evaluate_timeout());
    TEST_ASSERT_EQUAL_UINT8(NO_TIMEOUT, displayed_sensor_evaluate_timeout());
}

void test_no_timeout_when_table_empty(void)
{
    freertos_stub_set_tick(DISPLAY_SENSOR_TIMEOUT_MS * 2U);
    TEST_ASSERT_EQUAL_UINT8(NO_TIMEOUT, displayed_sensor_evaluate_timeout());
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

    RUN_TEST(test_no_timeout_before_period);
    RUN_TEST(test_timeout_at_period_returns_index);
    RUN_TEST(test_timeout_frees_slot_for_reuse);
    RUN_TEST(test_only_one_timeout_per_evaluate_call);
    RUN_TEST(test_no_timeout_when_table_empty);

    return UNITY_END();
}
