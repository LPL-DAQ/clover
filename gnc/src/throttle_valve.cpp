#include "throttle_valve.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <cmath>
#include <algorithm>

static const struct gpio_dt_spec pul_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_pul_gpios);
static const struct gpio_dt_spec dir_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_dir_gpios);

LOG_MODULE_REGISTER(throttle_valve, CONFIG_LOG_DEFAULT_LEVEL);

constexpr float MICROSTEPS = 8.0f;
constexpr float GEARBOX_RATIO = 20.0f;
constexpr float STEPS_PER_REVOLUTION = 200.0f;
constexpr float DEG_PER_STEP = 360.0f / STEPS_PER_REVOLUTION / GEARBOX_RATIO / MICROSTEPS;

constexpr float MAX_VELOCITY = 450.0f; // In deg/s
constexpr float MAX_ACCELERATION = 2000.0f; // In deg/s^2


enum MotorState {
    STOPPED,
    RUNNING,
};
static MotorState state = STOPPED;

static float velocity = 0;
static float acceleration = 0;
K_MUTEX_DEFINE(motor_lock);

volatile static int steps = 0;

/// Directly controls signal to controller, each rising edge on PUL is one step.
static void pulse(k_timer *) {
    // Switch direction, if we must.
    // gpio high -> flipped by converter to low -> more open.
    // pgio low -> flipped by converter to high -> more close.
    if ((gpio_pin_get_dt(&dir_gpio) == 1 && velocity > 0) || (!gpio_pin_get_dt(&dir_gpio) && velocity < 0)) {
        gpio_pin_toggle_dt(&dir_gpio);
        // We need to wait a while after changing dir before for next pulses.
        return;
    }
    // high to low -> flipped by converter to low to high -> rising edge, count a step.
    if (gpio_pin_get_dt(&pul_gpio)) {
        // As before,
        if (gpio_pin_get_dt(&dir_gpio)) {
            steps -= 1;
        } else {
            steps += 1;
        }
    }
    gpio_pin_toggle_dt(&pul_gpio);
}

K_TIMER_DEFINE(pulse_timer, pulse, nullptr);

/// Initializes throttle valve driver.
int throttle_valve_init() {
    LOG_INF("Initializing throttle valve...");

    if (!device_is_ready(pul_gpio.port) || !device_is_ready(dir_gpio.port)) {
        LOG_ERR("GPIO device(s) not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&pul_gpio, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&dir_gpio, GPIO_OUTPUT_INACTIVE);

    LOG_INF("Throttle valve initialized.");

    return 0;
}

int throttle_valve_start_calibrate() {
    LOG_INF("Beginning calibration.");
//
//    int err = throttle_valve_move(105.0f, 30.0f);
//    if (err) {
//        LOG_ERR("Move failed during calibration: err %d", err);
//        return err;
//    }
//    steps = static_cast<int>(90.0f / DEG_PER_STEP);
//
//    LOG_INF("Done initial movement. Backing off.");
//    throttle_valve_move(-5.0f, 0.5f);
//
//    LOG_INF("Calibrated to fully open.");
    return 0;
}

/// Moves to a certain degree position within the specified time. Does not necessarily guarantee that this will happen
/// as speed and acceleration limits will be enforced, but it is what we will target.
void throttle_valve_move(float target_deg) {
    constexpr float CONTROL_TIME = 0.001;

    float target_velocity = (throttle_valve_get_pos() - target_deg) / CONTROL_TIME;

    // Apply acceleration limit
    float target_acceleration = (target_velocity - velocity) / CONTROL_TIME;
    if (acceleration > MAX_ACCELERATION) {
        target_velocity = velocity + CONTROL_TIME * MAX_ACCELERATION;
    } else if (acceleration < -MAX_ACCELERATION) {
        target_velocity = velocity - CONTROL_TIME * MAX_ACCELERATION;
    }

    // Apply velocity acceleration
    target_velocity = std::clamp(target_velocity, -MAX_VELOCITY, MAX_VELOCITY);

    // Based on velocity, calculate pulse interval
    float nsec_per_step = 1e9f / target_velocity * DEG_PER_STEP / 2.0f;

    // Set true values for acceleraiton and velocity.
    acceleration = (target_velocity - velocity) / CONTROL_TIME;
    velocity = target_velocity;

    k_timer_start(&pulse_timer, K_NSEC(static_cast<uint64_t>(nsec_per_step)),
                  K_NSEC(static_cast<uint64_t>(nsec_per_step)));

    state = RUNNING;
}

void throttle_valve_stop() {
    k_mutex_lock(&motor_lock, K_FOREVER);
    acceleration = 0;
    state = STOPPED;
    k_mutex_unlock(&motor_lock);
}

/// Get current degree position of motor in degrees.
float throttle_valve_get_pos() {
    return static_cast<float>(steps) * DEG_PER_STEP;
}

int throttle_valve_set_open() {
    k_mutex_lock(&motor_lock, K_FOREVER);
    if (state != MotorState::STOPPED) {
        k_mutex_unlock(&motor_lock);
        LOG_ERR("Cannot reset to position open when motor is stopped.");
        return 1;
    }
    k_timer_stop(&pulse_timer);
    steps = static_cast<int>(90.0f / DEG_PER_STEP);
    k_mutex_unlock(&motor_lock);
    return 0;
}

int throttle_valve_set_closed() {
    k_mutex_lock(&motor_lock, K_FOREVER);
    if (state != MotorState::STOPPED) {
        k_mutex_unlock(&motor_lock);
        LOG_ERR("Cannot reset to position closed when motor is stopped.");
        return 1;
    }
    k_timer_stop(&pulse_timer);
    steps = 0;
    k_mutex_unlock(&motor_lock);
    return 0;
}
