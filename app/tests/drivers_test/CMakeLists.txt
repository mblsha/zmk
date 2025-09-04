# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(driver_tests)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/test_minimal.c)
target_sources_ifdef(CONFIG_DRV2605 app PRIVATE src/test_drv2605.c)
target_sources_ifdef(CONFIG_BLACKBERRY_TRACKPAD app PRIVATE src/test_blackberry_trackpad.c)

