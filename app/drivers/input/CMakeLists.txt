# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BLACKBERRY_TRACKPAD)
  zephyr_library()
  zephyr_library_sources(blackberry_trackpad.c)
endif()

