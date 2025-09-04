# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(full_system_integration)

target_sources(app PRIVATE src/main.c)

# Include drivers conditionally
target_sources_ifdef(CONFIG_BLACKBERRY_TRACKPAD app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../drivers/input/blackberry_trackpad.c)
target_sources_ifdef(CONFIG_DRV2605 app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../drivers/misc/drv2605.c)

