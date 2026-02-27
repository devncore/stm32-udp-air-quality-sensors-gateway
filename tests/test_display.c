/**
 * @file test_display.c
 * @brief Unit tests for display_iaq_classify().
 *
 * Strategy: equivalence classes + boundary values at every threshold
 * (50/51, 100/101, 150/151, 200/201, 300/301).
 */

#include "unity.h"
#include "app/display.h"

void setUp(void)    {}
void tearDown(void) {}

/* ── Equivalence classes ─────────────────────────────────────────────────── */

void test_iaq_classify_perfect(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_PERFECT, display_iaq_classify(0U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_PERFECT, display_iaq_classify(25U));
}

void test_iaq_classify_very_good(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_VERY_GOOD, display_iaq_classify(75U));
}

void test_iaq_classify_good(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_GOOD, display_iaq_classify(125U));
}

void test_iaq_classify_medium(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_MEDIUM, display_iaq_classify(175U));
}

void test_iaq_classify_bad(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_BAD, display_iaq_classify(250U));
}

void test_iaq_classify_very_bad(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_VERY_BAD, display_iaq_classify(400U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_VERY_BAD, display_iaq_classify(500U));
}

/* ── Boundary values ─────────────────────────────────────────────────────── */

void test_iaq_classify_boundary_50_51(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_PERFECT,   display_iaq_classify(50U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_VERY_GOOD, display_iaq_classify(51U));
}

void test_iaq_classify_boundary_100_101(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_VERY_GOOD, display_iaq_classify(100U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_GOOD,      display_iaq_classify(101U));
}

void test_iaq_classify_boundary_150_151(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_GOOD,   display_iaq_classify(150U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_MEDIUM, display_iaq_classify(151U));
}

void test_iaq_classify_boundary_200_201(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_MEDIUM, display_iaq_classify(200U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_BAD,    display_iaq_classify(201U));
}

void test_iaq_classify_boundary_300_301(void)
{
    TEST_ASSERT_EQUAL(AIR_QUALITY_BAD,      display_iaq_classify(300U));
    TEST_ASSERT_EQUAL(AIR_QUALITY_VERY_BAD, display_iaq_classify(301U));
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_iaq_classify_perfect);
    RUN_TEST(test_iaq_classify_very_good);
    RUN_TEST(test_iaq_classify_good);
    RUN_TEST(test_iaq_classify_medium);
    RUN_TEST(test_iaq_classify_bad);
    RUN_TEST(test_iaq_classify_very_bad);
    RUN_TEST(test_iaq_classify_boundary_50_51);
    RUN_TEST(test_iaq_classify_boundary_100_101);
    RUN_TEST(test_iaq_classify_boundary_150_151);
    RUN_TEST(test_iaq_classify_boundary_200_201);
    RUN_TEST(test_iaq_classify_boundary_300_301);

    return UNITY_END();
}
