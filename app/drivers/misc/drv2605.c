/*
 * Copyright (c) 2023 My Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_drv2605

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(drv2605, CONFIG_DRV2605_LOG_LEVEL);

/* DRV2605 Register Map */
#define DRV2605_STATUS 0x00
#define DRV2605_MODE 0x01
#define DRV2605_RTP_INPUT 0x02
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
#define DRV2605_OVERDRIVE_OFFSET 0x0D
#define DRV2605_SUSTAIN_OFFSET_POS 0x0E
#define DRV2605_SUSTAIN_OFFSET_NEG 0x0F
#define DRV2605_BRAKE_OFFSET 0x10
#define DRV2605_ATV_CONTROL 0x11
#define DRV2605_ATV_INPUT_LEVEL 0x12
#define DRV2605_ATV_OUTPUT_LEVEL 0x13
#define DRV2605_RATED_VOLTAGE 0x16
#define DRV2605_OVERDRIVE_CLAMP 0x17
#define DRV2605_FEEDBACK_CONTROL 0x1A
#define DRV2605_CONTROL1 0x1B
#define DRV2605_CONTROL2 0x1C
#define DRV2605_CONTROL3 0x1D
#define DRV2605_AUTOCAL_MEM 0x18

/* DRV2605 Mode Register Values */
#define DRV2605_MODE_INTTRIG 0x00     /* Internal trigger */
#define DRV2605_MODE_EXTTRIGEDGE 0x01 /* External edge trigger */
#define DRV2605_MODE_EXTTRIGLVL 0x02  /* External level trigger */
#define DRV2605_MODE_PWM 0x03         /* PWM input */
#define DRV2605_MODE_AUDIOVIBE 0x04   /* Audio-to-vibe */
#define DRV2605_MODE_RTP 0x05         /* Real-time playback */
#define DRV2605_MODE_DIAGNOSE 0x06    /* Diagnostics */
#define DRV2605_MODE_AUTOCAL 0x07     /* Auto calibration */
#define DRV2605_MODE_STANDBY 0x40     /* Standby mode */

/* Status register bits */
#define DRV2605_STATUS_DIAG_RESULT 0x08
#define DRV2605_STATUS_OVER_TEMP 0x02
#define DRV2605_STATUS_OC_DETECT 0x01

/* Some common waveform IDs */
#define DRV2605_WAVEFORM_CLICK 1
#define DRV2605_WAVEFORM_DOUBLE_CLICK 10
#define DRV2605_WAVEFORM_TICK 2
#define DRV2605_WAVEFORM_SLOW_RISE 3
#define DRV2605_WAVEFORM_QUICK_RISE 4
#define DRV2605_WAVEFORM_SLOW_FALL 5
#define DRV2605_WAVEFORM_QUICK_FALL 6

struct drv2605_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec enable_gpio;
    uint8_t library;
    uint8_t actuator_type;
    uint16_t rated_voltage;
    uint16_t overdrive_voltage;
    bool auto_calibration;
};

struct drv2605_data {
    const struct device *dev;
    struct k_mutex mutex;
    bool enabled;
};

/* Forward declarations */
int drv2605_play_waveform(const struct device *dev, uint8_t waveform_id);
int drv2605_play_sequence(const struct device *dev, const uint8_t *sequence, size_t len);
int drv2605_stop(const struct device *dev);

static int drv2605_reg_read(const struct device *dev, uint8_t reg, uint8_t *val) {
    const struct drv2605_config *config = dev->config;

    return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

static int drv2605_reg_write(const struct device *dev, uint8_t reg, uint8_t val) {
    const struct drv2605_config *config = dev->config;

    return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

static int drv2605_enable(const struct device *dev, bool enable) {
    const struct drv2605_config *config = dev->config;
    struct drv2605_data *data = dev->data;
    int ret = 0;

    k_mutex_lock(&data->mutex, K_FOREVER);

    if (enable && !data->enabled) {
        /* Enable the device via GPIO if available */
        if (config->enable_gpio.port) {
            gpio_pin_set_dt(&config->enable_gpio, 1);
            k_msleep(1); /* Wait for power up */
        }

        /* Take device out of standby */
        ret = drv2605_reg_write(dev, DRV2605_MODE, DRV2605_MODE_INTTRIG);
        if (ret == 0) {
            data->enabled = true;
        }
    } else if (!enable && data->enabled) {
        /* Put device in standby */
        ret = drv2605_reg_write(dev, DRV2605_MODE, DRV2605_MODE_STANDBY);

        /* Disable via GPIO if available */
        if (config->enable_gpio.port) {
            gpio_pin_set_dt(&config->enable_gpio, 0);
        }

        if (ret == 0) {
            data->enabled = false;
        }
    }

    k_mutex_unlock(&data->mutex);
    return ret;
}

int drv2605_play_waveform(const struct device *dev, uint8_t waveform_id) {
    struct drv2605_data *data = dev->data;
    int ret;

    if (!data->enabled) {
        LOG_WRN("Device not enabled");
        return -ENODEV;
    }

    k_mutex_lock(&data->mutex, K_FOREVER);

    /* Set waveform sequence */
    ret = drv2605_reg_write(dev, DRV2605_WAVEFORM_SEQ1, waveform_id);
    if (ret < 0) {
        goto exit;
    }

    /* Clear remaining sequence slots */
    ret = drv2605_reg_write(dev, DRV2605_WAVEFORM_SEQ2, 0);
    if (ret < 0) {
        goto exit;
    }

    /* Trigger playback */
    ret = drv2605_reg_write(dev, DRV2605_GO, 1);

exit:
    k_mutex_unlock(&data->mutex);
    return ret;
}

int drv2605_play_sequence(const struct device *dev, const uint8_t *sequence, size_t len) {
    struct drv2605_data *data = dev->data;
    int ret = 0;
    size_t i;

    if (!data->enabled) {
        LOG_WRN("Device not enabled");
        return -ENODEV;
    }

    if (len > 8) {
        LOG_ERR("Sequence too long (max 8 waveforms)");
        return -EINVAL;
    }

    k_mutex_lock(&data->mutex, K_FOREVER);

    /* Set waveform sequence */
    for (i = 0; i < 8; i++) {
        uint8_t waveform = (i < len) ? sequence[i] : 0;
        ret = drv2605_reg_write(dev, DRV2605_WAVEFORM_SEQ1 + i, waveform);
        if (ret < 0) {
            goto exit;
        }
    }

    /* Trigger playback */
    ret = drv2605_reg_write(dev, DRV2605_GO, 1);

exit:
    k_mutex_unlock(&data->mutex);
    return ret;
}

int drv2605_stop(const struct device *dev) {
    struct drv2605_data *data = dev->data;

    if (!data->enabled) {
        return 0; /* Already stopped */
    }

    k_mutex_lock(&data->mutex, K_FOREVER);
    int ret = drv2605_reg_write(dev, DRV2605_GO, 0);
    k_mutex_unlock(&data->mutex);

    return ret;
}

static int drv2605_auto_calibrate(const struct device *dev) {
    uint8_t status;
    int ret;
    int timeout = 1000; /* 1 second timeout */

    LOG_INF("Starting auto calibration");

    /* Set mode to auto calibration */
    ret = drv2605_reg_write(dev, DRV2605_MODE, DRV2605_MODE_AUTOCAL);
    if (ret < 0) {
        return ret;
    }

    /* Trigger calibration */
    ret = drv2605_reg_write(dev, DRV2605_GO, 1);
    if (ret < 0) {
        return ret;
    }

    /* Wait for calibration to complete */
    do {
        k_msleep(10);
        ret = drv2605_reg_read(dev, DRV2605_GO, &status);
        if (ret < 0) {
            return ret;
        }
        timeout -= 10;
    } while (status && timeout > 0);

    if (timeout <= 0) {
        LOG_ERR("Auto calibration timeout");
        return -ETIMEDOUT;
    }

    /* Check calibration result */
    ret = drv2605_reg_read(dev, DRV2605_STATUS, &status);
    if (ret < 0) {
        return ret;
    }

    if (status & DRV2605_STATUS_DIAG_RESULT) {
        LOG_ERR("Auto calibration failed");
        return -EIO;
    }

    LOG_INF("Auto calibration successful");
    return 0;
}

static int drv2605_init(const struct device *dev) {
    const struct drv2605_config *config = dev->config;
    struct drv2605_data *data = dev->data;
    uint8_t chip_id;
    int ret;

    data->dev = dev;
    k_mutex_init(&data->mutex);

    /* Check I2C connection */
    if (!i2c_is_ready_dt(&config->i2c)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    /* Initialize enable GPIO if present */
    if (config->enable_gpio.port) {
        if (!gpio_is_ready_dt(&config->enable_gpio)) {
            LOG_ERR("Enable GPIO not ready");
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&config->enable_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure enable GPIO: %d", ret);
            return ret;
        }

        /* Enable the device */
        gpio_pin_set_dt(&config->enable_gpio, 1);
        k_msleep(1);
    }

    /* Check device ID by reading status register */
    ret = drv2605_reg_read(dev, DRV2605_STATUS, &chip_id);
    if (ret < 0) {
        LOG_ERR("Failed to read chip ID: %d", ret);
        return ret;
    }

    LOG_INF("DRV2605 found, status: 0x%02x", chip_id);

    /* Configure the device */
    ret = drv2605_reg_write(dev, DRV2605_MODE, DRV2605_MODE_INTTRIG);
    if (ret < 0) {
        LOG_ERR("Failed to set mode: %d", ret);
        return ret;
    }

    /* Set waveform library */
    ret = drv2605_reg_write(dev, DRV2605_LIBRARY_SELECTION, config->library);
    if (ret < 0) {
        LOG_ERR("Failed to set library: %d", ret);
        return ret;
    }

    /* Configure feedback control for LRA/ERM */
    uint8_t feedback_val = 0;
    if (config->actuator_type == 1) { /* LRA */
        feedback_val = 0x80;          /* N_LRA_IN=0, FB_BRAKE_FACTOR=1, LOOP_GAIN=0 */
    }
    ret = drv2605_reg_write(dev, DRV2605_FEEDBACK_CONTROL, feedback_val);
    if (ret < 0) {
        LOG_ERR("Failed to set feedback control: %d", ret);
        return ret;
    }

    /* Set rated voltage */
    uint8_t rated_val = (config->rated_voltage * 255) / 5500; /* Convert mV to register value */
    ret = drv2605_reg_write(dev, DRV2605_RATED_VOLTAGE, rated_val);
    if (ret < 0) {
        LOG_ERR("Failed to set rated voltage: %d", ret);
        return ret;
    }

    /* Set overdrive voltage */
    uint8_t overdrive_val =
        (config->overdrive_voltage * 255) / 5500; /* Convert mV to register value */
    ret = drv2605_reg_write(dev, DRV2605_OVERDRIVE_CLAMP, overdrive_val);
    if (ret < 0) {
        LOG_ERR("Failed to set overdrive voltage: %d", ret);
        return ret;
    }

    /* Perform auto calibration if requested */
    if (config->auto_calibration) {
        ret = drv2605_auto_calibrate(dev);
        if (ret < 0) {
            LOG_WRN("Auto calibration failed: %d", ret);
            /* Continue initialization even if calibration fails */
        }

        /* Return to internal trigger mode */
        ret = drv2605_reg_write(dev, DRV2605_MODE, DRV2605_MODE_INTTRIG);
        if (ret < 0) {
            return ret;
        }
    }

    data->enabled = true;

    LOG_INF("DRV2605 haptic driver initialized");
    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int drv2605_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return drv2605_enable(dev, false);
    case PM_DEVICE_ACTION_RESUME:
        return drv2605_enable(dev, true);
    default:
        return -ENOTSUP;
    }
}
#endif

#define DRV2605_INIT(inst)                                                                         \
    static const struct drv2605_config drv2605_config_##inst = {                                   \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                         \
        .enable_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),                          \
        .library = DT_INST_PROP(inst, library),                                                    \
        .actuator_type = DT_INST_PROP(inst, actuator_type),                                        \
        .rated_voltage = DT_INST_PROP(inst, rated_voltage),                                        \
        .overdrive_voltage = DT_INST_PROP(inst, overdrive_voltage),                                \
        .auto_calibration = DT_INST_PROP(inst, auto_calibration),                                  \
    };                                                                                             \
                                                                                                   \
    static struct drv2605_data drv2605_data_##inst;                                                \
                                                                                                   \
    PM_DEVICE_DT_INST_DEFINE(inst, drv2605_pm_action);                                             \
                                                                                                   \
    DEVICE_DT_INST_DEFINE(inst, drv2605_init, PM_DEVICE_DT_INST_GET(inst), &drv2605_data_##inst,   \
                          &drv2605_config_##inst, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(DRV2605_INIT)