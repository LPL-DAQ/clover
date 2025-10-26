/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_pkt.h>

#include "server.h"
#include "throttle_valve.h"

extern "C" {
#include <app/drivers/blink.h>
}

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
             "Console device is not ACM CDC UART device");

int main(void) {
    // Serial over USB setup
    if (usb_enable(NULL)) {
        return 0;
    }
    const struct device *usb_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;
    while (!dtr) {
        uart_line_ctrl_get(usb_dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    // Status LED
    const struct device *blink = DEVICE_DT_GET(DT_NODELABEL(blink_led));
    if (!device_is_ready(blink)) {
        LOG_ERR("Blink LED not ready");
        return 0;
    }
    blink_set_period_ms(blink, 1000u);
    LOG_INF("hello there!");

    throttle_valve_init();

    serve_connections();
}

