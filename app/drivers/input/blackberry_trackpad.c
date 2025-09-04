/*
 * Copyright (c) 2023 My Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT blackberry_trackpad

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#ifndef CONFIG_INPUT_LOG_LEVEL
#define CONFIG_INPUT_LOG_LEVEL 3
#endif

LOG_MODULE_REGISTER(blackberry_trackpad, CONFIG_INPUT_LOG_LEVEL);

/* BlackBerry trackpad SPI commands */
#define BB_TP_CMD_READ_MOTION 0x02
#define BB_TP_CMD_READ_DELTA_X 0x03
#define BB_TP_CMD_READ_DELTA_Y 0x04
#define BB_TP_CMD_CONFIG_1 0x0A
#define BB_TP_CMD_CONFIG_2 0x0B
#define BB_TP_CMD_POWER_DOWN 0x0F
#define BB_TP_CMD_POWER_UP 0x10

/* Configuration values (placeholders) */
#define BB_TP_CONFIG_1_VAL 0x8D /* Default configuration */
#define BB_TP_CONFIG_2_VAL 0x40 /* Enable motion detection */

struct blackberry_trackpad_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec irq_gpio;
    struct gpio_dt_spec shutdown_gpio;
    bool swap_xy;
    bool invert_x;
    bool invert_y;
    int scale_x;
    int scale_y;
};

struct blackberry_trackpad_data {
    const struct device *dev;
    struct gpio_callback irq_cb;
    struct k_work motion_work;
    int16_t last_x;
    int16_t last_y;
};

static int bb_tp_spi_write_read(const struct device *dev, uint8_t cmd, uint8_t *data) {
    const struct blackberry_trackpad_config *config = dev->config;

    const struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1,
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };

    struct spi_buf rx_buf = {
        .buf = data,
        .len = 1,
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1,
    };

    return spi_transceive_dt(&config->spi, &tx, &rx);
}

static void bb_tp_motion_work_handler(struct k_work *work) {
    struct blackberry_trackpad_data *data =
        CONTAINER_OF(work, struct blackberry_trackpad_data, motion_work);
    const struct blackberry_trackpad_config *config = data->dev->config;

    uint8_t motion_status;
    int16_t delta_x = 0, delta_y = 0;
    uint8_t raw_delta;
    int ret;

    /* Read motion status */
    ret = bb_tp_spi_write_read(data->dev, BB_TP_CMD_READ_MOTION, &motion_status);
    if (ret < 0) {
        LOG_ERR("Failed to read motion status: %d", ret);
        return;
    }

    /* Check if motion detected */
    if (!(motion_status & 0x80)) {
        return; /* No motion */
    }

    /* Read X delta */
    ret = bb_tp_spi_write_read(data->dev, BB_TP_CMD_READ_DELTA_X, &raw_delta);
    if (ret < 0) {
        LOG_ERR("Failed to read delta X: %d", ret);
        return;
    }
    delta_x = (int8_t)raw_delta;

    /* Read Y delta */
    ret = bb_tp_spi_write_read(data->dev, BB_TP_CMD_READ_DELTA_Y, &raw_delta);
    if (ret < 0) {
        LOG_ERR("Failed to read delta Y: %d", ret);
        return;
    }
    delta_y = (int8_t)raw_delta;

    if (config->swap_xy) {
        int16_t tmp = delta_x;
        delta_x = delta_y;
        delta_y = tmp;
    }
    if (config->invert_x) {
        delta_x = -delta_x;
    }
    if (config->invert_y) {
        delta_y = -delta_y;
    }
    if (config->scale_x > 1) {
        delta_x *= config->scale_x;
    }
    if (config->scale_y > 1) {
        delta_y *= config->scale_y;
    }

    data->last_x += delta_x;
    data->last_y += delta_y;

    /* Emit input events (as relative motion) */
    input_report_rel(data->dev, INPUT_REL_X, delta_x, K_NO_WAIT);
    input_report_rel(data->dev, INPUT_REL_Y, delta_y, K_NO_WAIT);
    input_sync(data->dev);
}

static void bb_tp_irq_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
    ARG_UNUSED(port);
    ARG_UNUSED(pins);
    struct blackberry_trackpad_data *data =
        CONTAINER_OF(cb, struct blackberry_trackpad_data, irq_cb);
    k_work_submit(&data->motion_work);
}

static int blackberry_trackpad_init(const struct device *dev) {
    const struct blackberry_trackpad_config *config = dev->config;
    struct blackberry_trackpad_data *data = dev->data;
    int ret;

    data->dev = dev;
    k_work_init(&data->motion_work, bb_tp_motion_work_handler);

    if (!spi_is_ready_dt(&config->spi)) {
        LOG_ERR("SPI bus not ready");
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&config->irq_gpio)) {
        LOG_ERR("IRQ GPIO not ready");
        return -ENODEV;
    }

    if (config->shutdown_gpio.port) {
        if (!gpio_is_ready_dt(&config->shutdown_gpio)) {
            LOG_ERR("Shutdown GPIO not ready");
            return -ENODEV;
        }
        ret = gpio_pin_configure_dt(&config->shutdown_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure shutdown GPIO: %d", ret);
            return ret;
        }
    }

    ret = gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure IRQ GPIO: %d", ret);
        return ret;
    }

    gpio_init_callback(&data->irq_cb, bb_tp_irq_handler, BIT(config->irq_gpio.pin));
    ret = gpio_add_callback(config->irq_gpio.port, &data->irq_cb);
    if (ret < 0) {
        LOG_ERR("Failed to add GPIO callback: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure GPIO interrupt: %d", ret);
        return ret;
    }

    /* Basic init sequence placeholders */
    k_msleep(10);
    uint8_t dummy = 0;
    ret = bb_tp_spi_write_read(dev, BB_TP_CMD_CONFIG_1, &dummy);
    if (ret < 0) {
        LOG_ERR("Failed to write config 1: %d", ret);
        return ret;
    }
    ret = bb_tp_spi_write_read(dev, BB_TP_CMD_CONFIG_2, &dummy);
    if (ret < 0) {
        LOG_ERR("Failed to write config 2: %d", ret);
        return ret;
    }
    ret = bb_tp_spi_write_read(dev, BB_TP_CMD_POWER_UP, &dummy);
    if (ret < 0) {
        LOG_ERR("Failed to power up trackpad: %d", ret);
        return ret;
    }

    LOG_INF("BlackBerry trackpad initialized");
    return 0;
}

#define BLACKBERRY_TRACKPAD_INIT(inst)                                                             \
    static const struct blackberry_trackpad_config blackberry_trackpad_config_##inst = {           \
        .spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_OP_MODE_MASTER, 0),                \
        .irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                        \
        .shutdown_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, shutdown_gpios, {0}),                      \
        .swap_xy = DT_INST_PROP(inst, swap_xy),                                                    \
        .invert_x = DT_INST_PROP(inst, invert_x),                                                  \
        .invert_y = DT_INST_PROP(inst, invert_y),                                                  \
        .scale_x = DT_INST_PROP_OR(inst, scale_x, 1),                                              \
        .scale_y = DT_INST_PROP_OR(inst, scale_y, 1),                                              \
    };                                                                                             \
    static struct blackberry_trackpad_data blackberry_trackpad_data_##inst;                        \
    DEVICE_DT_INST_DEFINE(inst, blackberry_trackpad_init, NULL, &blackberry_trackpad_data_##inst,  \
                          &blackberry_trackpad_config_##inst, POST_KERNEL,                         \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(BLACKBERRY_TRACKPAD_INIT)

