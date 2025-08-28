# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(trackpad_basic_test)

# Source files for trackpad basic test
target_sources(app PRIVATE src/main.c)

# Build trackpad driver conditionally
target_sources_ifdef(CONFIG_BLACKBERRY_TRACKPAD app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../drivers/input/blackberry_trackpad.c)