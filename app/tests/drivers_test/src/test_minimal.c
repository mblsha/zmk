#include <zephyr/ztest.h>

ZTEST_SUITE(driver_framework, NULL, NULL, NULL, NULL, NULL);

// clang-format off
ZTEST(driver_framework, sanity)
{
    zassert_true(true, "Driver test framework is operational");
}
// clang-format on
