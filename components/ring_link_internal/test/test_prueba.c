#include "unity.h"

TEST_CASE("Test tonto", "[dummy]")
{
    TEST_ASSERT_EQUAL(1, 1);
    TEST_ASSERT_NOT_EQUAL(1, 2);
    TEST_ASSERT_TRUE(true);
    TEST_ASSERT_FALSE(false);
}   