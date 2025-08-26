# SPDX-License-Identifier: Apache-2.0

# Only create the library if we have sources to add
if(CONFIG_DRV2605)
  zephyr_library()
  zephyr_library_sources(drv2605.c)
endif()