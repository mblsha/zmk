/*
 * Copyright (c) 2023 My Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(driver_tests, LOG_LEVEL_DBG);

/* Build assertions to ensure emulator nodes are present */
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(drv2605_emul), okay),
             "drv2605_emul missing: check overlay compatible/version");

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(bb_trackpad_emul), okay),
             "bb_trackpad_emul missing: check overlay compatible/version");

ZTEST_SUITE(driver_tests, NULL, NULL, NULL, NULL, NULL);