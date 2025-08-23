/*
 * Copyright (c) 2023 My Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(driver_tests, LOG_LEVEL_DBG);

ZTEST_SUITE(driver_tests, NULL, NULL, NULL, NULL, NULL);