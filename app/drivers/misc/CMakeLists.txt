# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DRV2605)
  zephyr_library()
  zephyr_library_sources(drv2605.c)
endif()

