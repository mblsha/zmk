#include <zephyr/ztest.h>

ZTEST(driver_tests, test_minimal) {
    zassert_true(true, "Sanity");
}

