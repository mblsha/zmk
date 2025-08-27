#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>

// clang-format off
#if defined(CONFIG_DRV2605)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(drv2605_emul), okay),
             "drv2605_emul missing: check overlay compatible/version");
#endif

#if defined(CONFIG_BLACKBERRY_TRACKPAD)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(bb_trackpad_emul), okay),
             "bb_trackpad_emul missing: check overlay compatible/version");
#endif
// clang-format on
