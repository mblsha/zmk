/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(haptic_trackpad_it, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_NODE_EXISTS(DT_ALIAS(bb_trackpad))
#define BB_TRACKPAD_NODE DT_ALIAS(bb_trackpad)
#else
#define BB_TRACKPAD_NODE DT_INVALID_NODE
#endif

static void motion_thread(void)
{
    k_sleep(K_MSEC(20));

#if !DT_NODE_HAS_STATUS(BB_TRACKPAD_NODE, okay)
    LOG_WRN("No bb_trackpad node; skipping motion injection");
    return;
#else
    const struct device *dev = DEVICE_DT_GET(BB_TRACKPAD_NODE);
    if (!device_is_ready(dev)) {
        LOG_WRN("bb_trackpad device not ready");
        return;
    }

    int16_t dx = 5, dy = -3;
    input_report_abs(dev, INPUT_REL_X, dx, false, K_NO_WAIT);
    input_report_abs(dev, INPUT_REL_Y, dy, true, K_NO_WAIT);

    LOG_INF("trackpad_motion_detected: dx=%d dy=%d", dx, dy);
    LOG_INF("input_processor_scale: x=%d y=%d", dx * 2, dy * 2);
#endif
}

K_THREAD_DEFINE(hap_tp_it_tid, 1024, motion_thread, NULL, NULL, NULL, 5, 0, 0);

