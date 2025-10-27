#include "throttle_valve.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define STEPPER0_NODE DT_NODELABEL(stepper0)
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec pul_gpios = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_pul_gpios);
static const struct gpio_dt_spec dir_gpios = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_dir_gpios);

LOG_MODULE_REGISTER(throttle_valve, CONFIG_LOG_DEFAULT_LEVEL);

int throttle_valve_init() {
    // do some initializaiton work here?
    LOG_INF("Initializing throttle valve...");

    if (!device_is_ready(pul_gpios.port) || !device_is_ready(dir_gpios.port)) {
        LOG_ERR("GPIO device(s) not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&pul_gpios, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&dir_gpios, GPIO_OUTPUT_INACTIVE);

    LOG_INF("Throttle valve initialized.");

    return 0;
}

int throttle_valve_start_calibrate() {
    LOG_INF("Imagine I am calibrating?");

    return 0;
}

int throttle_valve_move(double degrees, double timems) {
    LOG_INF("Move %f degrees over %f ms", degrees, timems);

    const double step_angle = 1.8;
    int steps = (int)(degrees / step_angle);
    if (steps < 0) steps = -steps;
    if (steps == 0) {
        LOG_INF("No movement requested");
        return 0;
    }

    /*
    // Set direction
    if (degrees > 0) {
        gpio_pin_set_dt(&dir_gpios, 1);
    } else {
        gpio_pin_set_dt(&dir_gpios, 0);
    }
    */

    // Calculate delay per pulse to complete in timems
    int delay_us = (int)((timems * 1000.0) / steps / 2); // divide by 2 for high+low
    if (delay_us < 5) {
        LOG_WRN("Requested move time too short, limiting delay to 5 us per microstep");
        delay_us = 5;
    }

    LOG_INF("Moving %d steps, %d us per half-step", steps, delay_us);

    for (int i = 0; i < steps; i++) {

        gpio_pin_set_dt(&pul_gpios, 1);
        k_busy_wait(delay_us);
        gpio_pin_set_dt(&pul_gpios, 0);
        k_busy_wait(delay_us);
    }

    LOG_INF("Movement complete");
    return 0;
}

int throttle_testing(){
    LOG_INF("Testing throttle valve movement...");
    double delay_us = 50; // 50 us
    int test_period_us = 5000000; // 5 seconds
    int steps = test_period_us/(2*delay_us); // Number of steps to run for 5 seconds
    gpio_pin_set_dt(&dir_gpios, 0); // Set initial direction
    k_busy_wait(delay_us);

    for (int i = 0; i < steps; i++) {
        gpio_pin_set_dt(&pul_gpios, 1);
        k_busy_wait(delay_us);
        gpio_pin_set_dt(&pul_gpios, 0);
        k_busy_wait(delay_us);

        if (i == steps/2) {
            gpio_pin_set_dt(&dir_gpios, 1); // Change direction after half time
            k_busy_wait(delay_us);
        }

    }

    LOG_INF("Test complete");
    return 0;
}
