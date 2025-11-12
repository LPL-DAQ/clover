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
//#include "servotesting.h"
#include "pts.h"

extern "C" {
#include <app/drivers/blink.h>
}

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);


int main(void) {
    printk("Starting Code %s\n", CONFIG_BOARD);

    // Status LED
    const struct device *blink = DEVICE_DT_GET(DT_NODELABEL(blink_led));
    if (!device_is_ready(blink)) {
        return 0;
    }
    blink_set_period_ms(blink, 100u);


   // const struct device *usb_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
   // uint32_t dtr = 0;
   // while (!dtr) {
   //     uart_line_ctrl_get(usb_dev, UART_LINE_CTRL_DTR, &dtr);
   //     k_sleep(K_MSEC(100));
   // }

    LOG_INF("Initializing throttle valve");
    printk("Initializing throttle valve\n");
    int err = throttle_valve_init();
    if (err) {
        LOG_ERR("Failed to initialize throttle valve");
        return 0;
    }
    LOG_INF("Initializing servos");
    printk("Initializing servos\n");
    //err = servos_init();
    if (err) {
        LOG_ERR("Failed to initialize servos");
        return 0;
    }

    LOG_INF("Initializing PTs");
    printk("Initializing PTs\n");
    err = pts_init();
    if (err) {
        LOG_ERR("Failed to initialize PTs");
        return 0;
    }

    LOG_INF("Starting server");
    printk("Starting server\n");
    serve_connections();

    //servotesting_demo();

    while (1);
}

