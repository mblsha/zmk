# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BLACKBERRY_TRACKPAD blackberry_trackpad.c)