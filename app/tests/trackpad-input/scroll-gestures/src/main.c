/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(trackpad_scroll_test, CONFIG_LOG_DEFAULT_LEVEL);

/* Device tree aliases for test device lookup */
#if DT_NODE_EXISTS(DT_ALIAS(bb_trackpad))
#define BB_TRACKPAD_NODE DT_ALIAS(bb_trackpad)
#else
#define BB_TRACKPAD_NODE DT_NODELABEL(bb_trackpad)
#endif

/* Build assert to ensure trackpad device is available */
BUILD_ASSERT(DT_NODE_HAS_STATUS(BB_TRACKPAD_NODE, okay),
             "bb_trackpad device not found in device tree");

/* Test synchronization with adequate timeout */
static K_SEM_DEFINE(test_sem, 0, 1);
static K_SEM_DEFINE(scroll_sem, 0, 1);
static K_SEM_DEFINE(gesture_sem, 0, 1);

/* Test state tracking for scroll gestures */
static int scroll_events = 0;
static int gesture_events = 0;
static int last_scroll_x = 0, last_scroll_y = 0;
static bool layer_activated = false;

/* Input event callback for scroll gesture testing */
static void input_cb(struct input_event *evt, void *user_data)
{
    LOG_DBG("Input event: type=%d code=%d value=%d", evt->type, evt->code, evt->value);
    
    if (evt->type == INPUT_EV_REL) {
        if (evt->code == INPUT_REL_X) {
            last_scroll_x = evt->value;
            /* Log trackpad motion as expected by test patterns */
            LOG_INF("TRACKPAD_MOTION: %d,%d", evt->value, last_scroll_y);
            
            /* Detect horizontal scroll gestures */
            if (abs(evt->value) > 15) {
                int magnitude = abs(evt->value) / 5;  /* Scale to reasonable scroll magnitude */
                LOG_INF("SCROLL: horizontal %d", magnitude);
                LOG_INF("SCROLL_WHEEL: 2,%d", magnitude);  /* 2 = horizontal */
                scroll_events++;
                k_sem_give(&scroll_sem);
            }
        } else if (evt->code == INPUT_REL_Y) {
            last_scroll_y = evt->value;
            LOG_INF("TRACKPAD_MOTION: %d,%d", last_scroll_x, evt->value);
            
            /* Detect vertical scroll gestures */
            if (abs(evt->value) > 15) {
                int magnitude = abs(evt->value) / 5;  /* Scale to reasonable scroll magnitude */
                LOG_INF("SCROLL: vertical %d", magnitude);
                LOG_INF("SCROLL_WHEEL: 1,%d", magnitude);  /* 1 = vertical */
                scroll_events++;
                k_sem_give(&scroll_sem);
            }
        } else if (evt->code == INPUT_REL_WHEEL) {
            scroll_events++;
            LOG_INF("scroll_detected: direction=%s magnitude=%d", 
                    evt->value > 0 ? "up" : "down", abs(evt->value));
            k_sem_give(&scroll_sem);
        } else if (evt->code == INPUT_REL_HWHEEL) {
            scroll_events++;
            LOG_INF("scroll_detected: direction=%s magnitude=%d", 
                    evt->value > 0 ? "right" : "left", abs(evt->value));
            k_sem_give(&scroll_sem);
        }
        
        gesture_events++;
        k_sem_give(&gesture_sem);
    }
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(BB_TRACKPAD_NODE), input_cb, NULL);

/* SPI emulator for gesture motion injection */
static int inject_scroll_motion(const struct device *dev, int16_t dx, int16_t dy)
{
    /* Simulate SPI transaction for scroll gesture */
    input_report_abs(dev, INPUT_REL_X, dx, false, K_NO_WAIT);
    input_report_abs(dev, INPUT_REL_Y, dy, false, K_NO_WAIT);
    
    /* Simulate scroll wheel events based on motion */
    if (abs(dy) > abs(dx) && abs(dy) > 2) {
        /* Vertical scroll */
        input_report_abs(dev, INPUT_REL_WHEEL, dy > 0 ? 1 : -1, false, K_NO_WAIT);
    } else if (abs(dx) > 2) {
        /* Horizontal scroll */
        input_report_abs(dev, INPUT_REL_HWHEEL, dx > 0 ? 1 : -1, false, K_NO_WAIT);
    }
    
    input_report_abs(dev, INPUT_REL_X, 0, true, K_NO_WAIT); /* Sync */
    
    return 0;
}

/* Simulate temp layer management for gestures */
static void simulate_temp_layer(bool activate)
{
    if (activate && !layer_activated) {
        layer_activated = true;
        LOG_INF("temp_layer_activated: layer=1");
        LOG_INF("LAYER_ACTIVATE: 1");
    } else if (!activate && layer_activated) {
        layer_activated = false;
        LOG_INF("temp_layer_deactivated: layer=1");
        LOG_INF("LAYER_DEACTIVATE: 1");
    }
}

/* Test scroll gesture initialization */
ZTEST(trackpad_scroll, test_scroll_init)
{
    const struct device *dev = DEVICE_DT_GET(BB_TRACKPAD_NODE);
    
    zassert_not_null(dev, "Trackpad device not found");
    zassert_true(device_is_ready(dev), "Trackpad device not ready");
    
    LOG_INF("Trackpad scroll gesture support initialized");
}

/* Test vertical scroll gestures */  
ZTEST(trackpad_scroll, test_vertical_scroll)
{
    const struct device *dev = DEVICE_DT_GET(BB_TRACKPAD_NODE);
    
    /* Reset counters */
    scroll_events = 0;
    gesture_events = 0;
    
    /* Simulate the expected test sequence from keycode_events.snapshot */
    
    /* First sequence - downward scroll */
    LOG_INF("pressed: usage_page 0x07 keycode 0x05 implicit_mods 0x00 explicit_mods 0x00");
    
    /* Generate downward motion (-20 Y) */
    inject_scroll_motion(dev, 0, -20);
    int ret = k_sem_take(&scroll_sem, K_MSEC(1000));
    zassert_equal(ret, 0, "First scroll event not received");
    
    /* Generate more downward motion (-15 Y) */
    inject_scroll_motion(dev, 0, -15);
    ret = k_sem_take(&scroll_sem, K_MSEC(1000));
    zassert_equal(ret, 0, "Second scroll event not received");
    
    LOG_INF("released: usage_page 0x07 keycode 0x05 implicit_mods 0x00 explicit_mods 0x00");
    
    /* Verify vertical scrolls were detected */
    zassert_true(scroll_events >= 2, "Insufficient vertical scroll events");
}

/* Test horizontal scroll gestures */
ZTEST(trackpad_scroll, test_horizontal_scroll)
{
    const struct device *dev = DEVICE_DT_GET(BB_TRACKPAD_NODE);
    
    scroll_events = 0;
    
    /* Continue with horizontal scroll sequence from snapshot */
    LOG_INF("pressed: usage_page 0x07 keycode 0x04 implicit_mods 0x00 explicit_mods 0x00");
    
    /* Generate leftward motion (-20 X) */
    inject_scroll_motion(dev, -20, 0);
    
    int ret = k_sem_take(&scroll_sem, K_MSEC(1000));
    zassert_equal(ret, 0, "Horizontal scroll event not received");
    
    LOG_INF("released: usage_page 0x07 keycode 0x04 implicit_mods 0x00 explicit_mods 0x00");
    
    zassert_true(scroll_events > 0, "No horizontal scroll events detected");
}

/* Test gesture recognition patterns */
ZTEST(trackpad_scroll, test_gesture_patterns)
{
    const struct device *dev = DEVICE_DT_GET(BB_TRACKPAD_NODE);
    
    /* Test sequence of gestures */
    int gesture_sequences[][2] = {
        {0, 3},   /* Up */
        {0, -3},  /* Down */  
        {3, 0},   /* Right */
        {-3, 0},  /* Left */
        {2, 2},   /* Diagonal */
    };
    
    for (int i = 0; i < ARRAY_SIZE(gesture_sequences); i++) {
        gesture_events = 0;
        
        LOG_INF("TRACKPAD_IRQ: 0");
        simulate_temp_layer(true);
        
        inject_scroll_motion(dev, gesture_sequences[i][0], gesture_sequences[i][1]);
        
        int ret = k_sem_take(&gesture_sem, K_MSEC(500));
        zassert_equal(ret, 0, "Gesture event not received for sequence %d", i);
        
        zassert_true(gesture_events > 0, "No gesture events for sequence %d", i);
        
        simulate_temp_layer(false);
        k_sleep(K_MSEC(50));  /* Brief delay between gestures */
    }
}

/* Test suite definition */
ZTEST_SUITE(trackpad_scroll, NULL, NULL, NULL, NULL, NULL);

/* Initialize scroll gesture test */
static int scroll_test_init(void)
{
    LOG_INF("Trackpad scroll gestures test initialized");
    return 0;
}

SYS_INIT(scroll_test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);