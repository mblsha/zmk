# SPDX-License-Identifier: Apache-2.0

# Only create the library if we have sources to add
if(CONFIG_BLACKBERRY_TRACKPAD)
  zephyr_library()
  zephyr_library_sources(blackberry_trackpad.c)
endif()