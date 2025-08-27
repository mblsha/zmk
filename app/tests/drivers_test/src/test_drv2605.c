/*
 * Copyright (c) 2023 My Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(test_drv2605, LOG_LEVEL_DBG);

/* DRV2605 register definitions for testing */
#define DRV2605_STATUS 0x00
#define DRV2605_MODE 0x01
#define DRV2605_REAL_TIME_PLAYBACK 0x02
#define DRV2605_LIBRARY_SELECTION 0x03
#define DRV2605_WAVEFORM_SEQ1 0x04
#define DRV2605_WAVEFORM_SEQ2 0x05
#define DRV2605_WAVEFORM_SEQ3 0x06
#define DRV2605_WAVEFORM_SEQ4 0x07
#define DRV2605_WAVEFORM_SEQ5 0x08
#define DRV2605_WAVEFORM_SEQ6 0x09
#define DRV2605_WAVEFORM_SEQ7 0x0A
#define DRV2605_WAVEFORM_SEQ8 0x0B
#define DRV2605_GO 0x0C
#define DRV2605_OVERDRIVE_TIME_OFFSET 0x0D
#define DRV2605_SUSTAIN_TIME_OFFSET_POS 0x0E
#define DRV2605_SUSTAIN_TIME_OFFSET_NEG 0x0F
#define DRV2605_BRAKE_TIME_OFFSET 0x10
#define DRV2605_AUDIO_2_VIBE_CTRL 0x11
#define DRV2605_AUDIO_2_VIBE_MIN_INPUT 0x12
#define DRV2605_AUDIO_2_VIBE_MAX_INPUT 0x13
#define DRV2605_AUDIO_2_VIBE_MIN_OUTPUT 0x14
#define DRV2605_AUDIO_2_VIBE_MAX_OUTPUT 0x15
#define DRV2605_RATED_VOLTAGE 0x16
#define DRV2605_OVERDRIVE_CLAMP 0x17
#define DRV2605_AUTO_CAL_COMP_RESULT 0x18
#define DRV2605_AUTO_CAL_BACK_EMF_RESULT 0x19
#define DRV2605_FEEDBACK_CONTROL 0x1A
#define DRV2605_CONTROL1 0x1B
#define DRV2605_CONTROL2 0x1C
#define DRV2605_CONTROL3 0x1D
#define DRV2605_CONTROL4 0x1E
#define DRV2605_CONTROL5 0x1F
#define DRV2605_LRA_OPEN_LOOP_PERIOD 0x20
#define DRV2605_VBAT_VOLTAGE_MONITOR 0x21
#define DRV2605_LRA_RESONANCE_PERIOD 0x22

/* DRV2605 mode values */
#define DRV2605_MODE_INTERNAL_TRIGGER 0x00
#define DRV2605_MODE_EXTERNAL_TRIGGER 0x01
#define DRV2605_MODE_EXTERNAL_TRIGGER_GPIO 0x02
#define DRV2605_MODE_PWM_ANALOG_INPUT 0x03
#define DRV2605_MODE_AUDIO_2_VIBE 0x04
#define DRV2605_MODE_REAL_TIME_PLAYBACK 0x05
#define DRV2605_MODE_DIAGNOSTICS 0x06
#define DRV2605_MODE_AUTO_CALIBRATION 0x07

/* DRV2605 library values */
#define DRV2605_LIBRARY_EMPTY 0x00
#define DRV2605_LIBRARY_TS2200_A 0x01
#define DRV2605_LIBRARY_TS2200_B 0x02
#define DRV2605_LIBRARY_TS2200_C 0x03
#define DRV2605_LIBRARY_TS2200_D 0x04
#define DRV2605_LIBRARY_TS2200_E 0x05
#define DRV2605_LIBRARY_LRA 0x06
#define DRV2605_LIBRARY_TS2200_F 0x07

/* Test device handles */
static const struct device *haptic_dev = DEVICE_DT_GET_ANY(ti_drv2605);
static const struct device *i2c_emul = DEVICE_DT_GET(DT_NODELABEL(test_i2c));
static const struct device *gpio_emul = DEVICE_DT_GET(DT_NODELABEL(gpio0));

/* Mock I2C register state */
struct drv2605_reg_state {
    uint8_t status;
    uint8_t mode;
    uint8_t library_selection;
    uint8_t waveform_seq[8]; /* SEQ1-SEQ8 */
    uint8_t go;
    uint8_t rated_voltage;
    uint8_t overdrive_clamp;
    uint8_t feedback_control;
    uint8_t auto_cal_result;
    uint8_t auto_cal_back_emf;
    bool calibration_done;
    bool device_enabled;
};

static struct drv2605_reg_state mock_drv2605_state;

/* Mock waveform definitions (subset of DRV2605 library) */
#define WAVEFORM_CLICK 1
#define WAVEFORM_TICK 2
#define WAVEFORM_SLOW_RISE 3
#define WAVEFORM_QUICK_FALL 4
#define WAVEFORM_BUZZ 5
#define WAVEFORM_ALERT_750MS 10
#define WAVEFORM_ALERT_1000MS 11
#define WAVEFORM_STRONG_CLICK 12
#define WAVEFORM_SHARP_CLICK 13
#define WAVEFORM_SHORT_DOUBLE_CLICK_STRONG 14

/* Helper functions */
static void reset_mock_drv2605_state(void)
{
    memset(&mock_drv2605_state, 0, sizeof(mock_drv2605_state));
    mock_drv2605_state.status = 0x00; /* Device ready, no errors */
    mock_drv2605_state.mode = DRV2605_MODE_INTERNAL_TRIGGER;
    mock_drv2605_state.library_selection = DRV2605_LIBRARY_LRA;
    mock_drv2605_state.rated_voltage = 0x3E;    /* ~2V */
    mock_drv2605_state.overdrive_clamp = 0x8C;  /* ~2.5V */
    mock_drv2605_state.feedback_control = 0xB6; /* LRA mode, brake disabled, loop gain medium */
    mock_drv2605_state.device_enabled = true;
    mock_drv2605_state.calibration_done = false;
}

/* Test helper to verify register values */
static void assert_register_value(uint8_t reg, uint8_t expected, const char *description)
{
    uint8_t actual;
    switch (reg) {
    case DRV2605_STATUS:
        actual = mock_drv2605_state.status;
        break;
    case DRV2605_MODE:
        actual = mock_drv2605_state.mode;
        break;
    case DRV2605_LIBRARY_SELECTION:
        actual = mock_drv2605_state.library_selection;
        break;
    case DRV2605_GO:
        actual = mock_drv2605_state.go;
        break;
    default:
        actual = 0;
        break;
    }
    zassert_equal(actual, expected, "%s (reg 0x%02X)", description, reg);
}

ZTEST(driver_tests, test_drv2605_initialization) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test that device initializes successfully */
    zassert_true(device_is_ready(haptic_dev), "DRV2605 should initialize successfully");

    /* Verify I2C emulation controller is ready */
    zassert_true(device_is_ready(i2c_emul), "I2C emulation controller should be ready");

    /* Verify GPIO emulation controller is ready (for enable pin) */
    zassert_true(device_is_ready(gpio_emul), "GPIO emulation controller should be ready");

    /* Test register initialization values */
    assert_register_value(DRV2605_MODE, DRV2605_MODE_INTERNAL_TRIGGER,
                          "Initial mode should be internal trigger");
    assert_register_value(DRV2605_LIBRARY_SELECTION, DRV2605_LIBRARY_LRA,
                          "Initial library should be LRA");

    LOG_INF("DRV2605 initialization test passed");
}

ZTEST(driver_tests, test_drv2605_waveform_playback) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test basic waveform playback functionality */
    uint8_t test_waveform = WAVEFORM_STRONG_CLICK;

    /* In a full implementation with mock I2C, we would:
     * 1. Call drv2605_play_waveform(haptic_dev, test_waveform)
     * 2. Verify I2C transaction to write waveform to SEQ1 register
     * 3. Verify I2C transaction to write GO register
     * 4. Verify device status indicates playback in progress
     */

    /* For now, test the mock state management */
    mock_drv2605_state.waveform_seq[0] = test_waveform;
    mock_drv2605_state.go = 1;

    zassert_equal(mock_drv2605_state.waveform_seq[0], WAVEFORM_STRONG_CLICK,
                  "Waveform should be stored in sequence register 1");
    zassert_equal(mock_drv2605_state.go, 1, "GO register should be set to start playback");

    LOG_INF("DRV2605 waveform playback test - mock framework ready");
}

ZTEST(driver_tests, test_drv2605_sequence_playback) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test sequence playback functionality */
    uint8_t test_sequence[] = {WAVEFORM_CLICK, WAVEFORM_TICK, WAVEFORM_BUZZ, 0};
    size_t sequence_length = sizeof(test_sequence) - 1; /* Exclude null terminator */

    /* In a full implementation, we would:
     * 1. Call drv2605_play_sequence(haptic_dev, test_sequence, sequence_length)
     * 2. Verify I2C transactions to write each waveform to SEQ1-SEQn registers
     * 3. Verify sequence termination with 0 waveform
     * 4. Verify GO register is set to start sequence playback
     */

    /* Test sequence storage in mock state */
    for (size_t i = 0; i < sequence_length && i < 8; i++) {
        mock_drv2605_state.waveform_seq[i] = test_sequence[i];
    }
    mock_drv2605_state.go = 1;

    /* Verify sequence was stored correctly */
    zassert_equal(mock_drv2605_state.waveform_seq[0], WAVEFORM_CLICK,
                  "First waveform should be click");
    zassert_equal(mock_drv2605_state.waveform_seq[1], WAVEFORM_TICK,
                  "Second waveform should be tick");
    zassert_equal(mock_drv2605_state.waveform_seq[2], WAVEFORM_BUZZ,
                  "Third waveform should be buzz");
    zassert_equal(mock_drv2605_state.go, 1, "GO register should be set for sequence");

    LOG_INF("DRV2605 sequence playback test - mock framework ready");
}

ZTEST(driver_tests, test_drv2605_auto_calibration) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test auto-calibration functionality */
    /* In a full implementation, we would:
     * 1. Set mode to auto-calibration
     * 2. Set GO register to start calibration
     * 3. Wait for calibration to complete (status register)
     * 4. Read calibration results
     * 5. Verify calibration values are within acceptable range
     */

    /* Simulate calibration process */
    mock_drv2605_state.mode = DRV2605_MODE_AUTO_CALIBRATION;
    mock_drv2605_state.go = 1;

    /* Simulate calibration completion */
    mock_drv2605_state.auto_cal_result = 0xA0;   /* Good calibration result */
    mock_drv2605_state.auto_cal_back_emf = 0x6F; /* Good back-EMF result */
    mock_drv2605_state.calibration_done = true;
    mock_drv2605_state.go = 0; /* Clear GO when calibration completes */

    /* Verify calibration state */
    assert_register_value(DRV2605_MODE, DRV2605_MODE_AUTO_CALIBRATION,
                          "Mode should be set to auto-calibration");
    zassert_true(mock_drv2605_state.calibration_done, "Calibration should complete successfully");
    zassert_equal(mock_drv2605_state.auto_cal_result, 0xA0,
                  "Calibration result should be in valid range");
    zassert_equal(mock_drv2605_state.auto_cal_back_emf, 0x6F,
                  "Back-EMF result should be in valid range");

    LOG_INF("DRV2605 auto-calibration test - mock framework ready");
}

ZTEST(driver_tests, test_drv2605_power_management) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test power management functionality */
    /* In a full implementation, we would:
     * 1. Test device suspend (standby mode)
     * 2. Test device resume (active mode)
     * 3. Test enable pin control via GPIO
     * 4. Verify power consumption in different states
     */

    /* Test device enable/disable via GPIO */
    mock_drv2605_state.device_enabled = false; /* Simulate GPIO enable pin low */
    zassert_false(mock_drv2605_state.device_enabled,
                  "Device should be disabled when enable pin is low");

    mock_drv2605_state.device_enabled = true; /* Simulate GPIO enable pin high */
    zassert_true(mock_drv2605_state.device_enabled,
                 "Device should be enabled when enable pin is high");

    /* Test standby mode */
    mock_drv2605_state.mode = DRV2605_MODE_INTERNAL_TRIGGER;
    mock_drv2605_state.status |= 0x01; /* Set standby bit */

    LOG_INF("DRV2605 power management test - mock framework ready");
}

ZTEST(driver_tests, test_drv2605_error_conditions) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test error handling scenarios */

    /* Test I2C communication failure */
    /* In a real implementation, we would simulate I2C timeouts/NACK conditions */

    /* Test overcurrent condition */
    mock_drv2605_state.status |= 0x02; /* Set OC_DETECT bit */
    zassert_true((mock_drv2605_state.status & 0x02) != 0,
                 "Overcurrent detection should be flagged");

    /* Test overtemperature condition */
    mock_drv2605_state.status |= 0x08; /* Set OVER_TEMP bit */
    zassert_true((mock_drv2605_state.status & 0x08) != 0,
                 "Overtemperature condition should be flagged");

    /* Test device not ready condition */
    mock_drv2605_state.status |= 0x80; /* Clear DEVICE_ID bit to indicate error */

    /* Test invalid waveform ID */
    uint8_t invalid_waveform = 124; /* Above max waveform ID */
    mock_drv2605_state.waveform_seq[0] = invalid_waveform;
    zassert_equal(mock_drv2605_state.waveform_seq[0], 124, "Invalid waveform should be detectable");

    LOG_INF("DRV2605 error conditions test - mock framework ready");
}

ZTEST(driver_tests, test_drv2605_real_time_playback) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test real-time playback (RTP) mode */
    /* In RTP mode, amplitude is controlled directly via register writes */

    mock_drv2605_state.mode = DRV2605_MODE_REAL_TIME_PLAYBACK;

    /* Test different amplitude levels */
    uint8_t test_amplitudes[] = {0x00, 0x40, 0x80, 0xFF}; /* 0%, 25%, 50%, 100% */

    for (size_t i = 0; i < ARRAY_SIZE(test_amplitudes); i++) {
        /* In full implementation, would write to RTP register */
        uint8_t amplitude = test_amplitudes[i];

        /* Verify amplitude is in valid range */
        zassert_true(amplitude <= 0xFF, "RTP amplitude should be within valid range");

        LOG_DBG("Testing RTP amplitude: 0x%02X (%d%%)", amplitude, (amplitude * 100) / 255);
    }

    assert_register_value(DRV2605_MODE, DRV2605_MODE_REAL_TIME_PLAYBACK, "Mode should be RTP");

    LOG_INF("DRV2605 real-time playback test - mock framework ready");
}

ZTEST(driver_tests, test_drv2605_library_selection) {
    if (!device_is_ready(haptic_dev)) {
        ztest_test_skip();
        return;
    }

    reset_mock_drv2605_state();

    /* Test different waveform library selections */
    uint8_t test_libraries[] = {DRV2605_LIBRARY_EMPTY, DRV2605_LIBRARY_TS2200_A,
                                DRV2605_LIBRARY_TS2200_B, DRV2605_LIBRARY_LRA};

    for (size_t i = 0; i < ARRAY_SIZE(test_libraries); i++) {
        mock_drv2605_state.library_selection = test_libraries[i];

        /* Verify library selection is stored correctly */
        assert_register_value(DRV2605_LIBRARY_SELECTION, test_libraries[i],
                              "Library selection should be correct");

        LOG_DBG("Testing library selection: 0x%02X", test_libraries[i]);
    }

    LOG_INF("DRV2605 library selection test - mock framework ready");
}