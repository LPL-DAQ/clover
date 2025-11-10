#include "throttle_valve.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>

#define STEPPER0_NODE DT_NODELABEL(stepper0)
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)


static const pwm_dt_spec ESC_1 =
    PWM_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const pwm_dt_spec ESC_2 =
    PWM_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);


static const pwm_dt_spec SERVO_X =
    PWM_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 2);
static const pwm_dt_spec SERVO_Y =
    PWM_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 3);



static const struct gpio_dt_spec pul_gpios = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_pul_gpios);
static const struct gpio_dt_spec dir_gpios = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_dir_gpios);

LOG_MODULE_REGISTER(throttle_valve, CONFIG_LOG_DEFAULT_LEVEL);

constexpr int MICROSTEPS = 16;
constexpr double DEG_PER_STEP = 360.0 / 200.0 / 20.0 / static_cast<double>(MICROSTEPS);
volatile int steps = 0;

// Dir pin off = CCW with shaft facing up -> moves more open

int throttle_valve_init() {
    LOG_INF("Initializing throttle valve...");
    (void)SERVO_X;    (void)SERVO_Y;
    (void)ESC_1; (void)ESC_2;

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
    LOG_INF("Beginning calibration.");

    int err = throttle_valve_move(105.0, 30000.0);
    if (err) {
        LOG_ERR("Move failed during calibration: err %d", err);
        return err;
    }
    steps = static_cast<int>(90.0 / DEG_PER_STEP);

    LOG_INF("Done initial movement. Backing off.");
    throttle_valve_move(-5.0, 500.0);

    LOG_INF("Calibrated to fully open.");
    return 0;
}

int throttle_valve_move(double delta_degrees, double timems) {
    int steps_to_move = static_cast<int>(delta_degrees / DEG_PER_STEP);
    if (steps_to_move < 0) steps_to_move = -steps_to_move;
    if (steps_to_move == 0) {
        k_busy_wait(1000*timems);
        return 0;
    }

    // Set direction
    if (delta_degrees > 0) {
        gpio_pin_set_dt(&dir_gpios, 1);
    } else {
        gpio_pin_set_dt(&dir_gpios, 0);
    }

    // Calculate delay per pulse to complete in timems
    int delay_us = (int) ((timems * 1000.0) / steps_to_move / 2); // divide by 2 for high+low
    if (delay_us < 5) {
        delay_us = 5;
    }

    for (int i = 0; i < steps_to_move; i++) {
        k_busy_wait(delay_us);
        gpio_pin_set_dt(&pul_gpios, 1);
        if (delta_degrees > 0) {
            steps += 1;
        } else {
            steps -= 1;
        }
        k_busy_wait(delay_us);
        gpio_pin_set_dt(&pul_gpios, 0);
    }

    return 0;
}

double throttle_valve_get_pos() {
    return static_cast<double>(steps) * DEG_PER_STEP;
}

int throttle_testing() {
    LOG_INF("Testing throttle valve movement...");
    double delay_us = 50; // 50 us
    int test_period_us = 5000000; // 5 seconds
    int steps = test_period_us / (2 * delay_us); // Number of steps to run for 5 seconds
    gpio_pin_set_dt(&dir_gpios, 0); // Set initial direction
    k_busy_wait(delay_us);

    for (int i = 0; i < steps; i++) {
        gpio_pin_set_dt(&pul_gpios, 1);
        k_busy_wait(delay_us);
        gpio_pin_set_dt(&pul_gpios, 0);
        k_busy_wait(delay_us);

        if (i == steps / 2) {
            gpio_pin_set_dt(&dir_gpios, 1); // Change direction after half time
            k_busy_wait(delay_us);
        }

    }

    LOG_INF("Test complete");
    return 0;
}

void throttle_valve_set_open() {
    steps = static_cast<int>(90.0 / DEG_PER_STEP);
}

void throttle_valve_set_closed() {
    steps = 0;
}
