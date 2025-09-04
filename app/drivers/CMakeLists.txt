# SPDX-License-Identifier: Apache-2.0

# Add driver subdirectories when their configs are enabled
add_subdirectory_ifdef(CONFIG_INPUT input)
add_subdirectory_ifdef(CONFIG_DRV2605 misc)

