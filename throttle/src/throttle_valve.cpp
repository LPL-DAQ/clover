#include "throttle_valve.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <cmath>

static const struct gpio_dt_spec pul_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_pul_gpios);
static const struct gpio_dt_spec dir_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), stepper_dir_gpios);

LOG_MODULE_REGISTER(throttle_valve, CONFIG_LOG_DEFAULT_LEVEL);

constexpr int MICROSTEPS = 8;
constexpr float DEG_PER_STEP = 360.0f / 200.0f / 20.0f / static_cast<float>(MICROSTEPS);
constexpr float MAX_SPEED = 450.0f; // In deg/s
constexpr float MAX_ACCELERATION = 2000.0f; // In deg/s^2

constexpr int NSEC_PER_CONTROL_ITER = 100000; // 10 kHz


enum MotorState {
    STOPPED,
    RUNNING,
};
static MotorState state = STOPPED;

volatile static int steps = 0;
volatile static uint64_t cycles_to_run = 0;
volatile static uint64_t start_cycle = 0;
volatile static float acceleration = 0;
volatile static float velocity = 0;
K_MUTEX_DEFINE(motor_lock);

/// Directly controls signal to controller, each rising edge on PUL is one step.
static void pulse(k_timer *timer) {
    // Switch direction, if we must.
    // gpio high -> flipped by converter to low -> more open.
    // pgio low -> flipped by converter to high -> more close.
    if ((gpio_pin_get_dt(&dir_gpio) == 1 && velocity > 0) || (!gpio_pin_get_dt(&dir_gpio) && velocity < 0)) {
        gpio_pin_toggle_dt(&dir_gpio);
        // We need to wait a while after changing dir before for next pulses.
        return;
    }
    if (gpio_pin_get_dt(&pul_gpio)) {
        if (gpio_pin_get_dt(&dir_gpio)) {
            steps -= 1;
        } else {
            steps += 1;
        }
    }
    gpio_pin_toggle_dt(&pul_gpio);
}

K_TIMER_DEFINE(pulse_timer, pulse, nullptr);

/// Schedules interations of motor_control.
static void motor_control(k_timer *) {
    // If state is STOPPED, completely halt motor.
    if (state == MotorState::STOPPED) {
        k_timer_stop(&pulse_timer);
        return;
    }

    // Apply acceleration and enforce velocity limits
    velocity += acceleration * (static_cast<float>(NSEC_PER_CONTROL_ITER) / 1e9f);
    velocity = CLAMP(velocity, -MAX_SPEED, MAX_SPEED);

    float nsec_per_step = 1e9f / velocity * DEG_PER_STEP / 2.0f;
    k_timer_start(&pulse_timer, K_NSEC(static_cast<uint64_t>(nsec_per_step)),
                  K_NSEC(static_cast<uint64_t>(nsec_per_step)));
}

K_TIMER_DEFINE(schedule_motor_control_timer, motor_control, nullptr);

/// Initializes throttle valve driver.
int throttle_valve_init() {
    LOG_INF("Initializing throttle valve...");

    if (!device_is_ready(pul_gpio.port) || !device_is_ready(dir_gpio.port)) {
        LOG_ERR("GPIO device(s) not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&pul_gpio, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&dir_gpio, GPIO_OUTPUT_INACTIVE);

    auto control_interval = K_NSEC(NSEC_PER_CONTROL_ITER);
    k_timer_start(&schedule_motor_control_timer, control_interval, control_interval);

    LOG_INF("Throttle valve initialized.");

    return 0;
}

int throttle_valve_start_calibrate() {
    LOG_INF("Beginning calibration.");

    int err = throttle_valve_move(105.0f, 30.0f);
    if (err) {
        LOG_ERR("Move failed during calibration: err %d", err);
        return err;
    }
    steps = static_cast<int>(90.0f / DEG_PER_STEP);

    LOG_INF("Done initial movement. Backing off.");
    throttle_valve_move(-5.0f, 0.5f);

    LOG_INF("Calibrated to fully open.");
    return 0;
}

/// Moves to a certain degree position within the specified time. Does not necessarily guarantee that this will happen
/// as speed and acceleration limits will be enforced, but it is what we will target.
int throttle_valve_move(float target_deg, float time) {
    if (time <= 0) {
        LOG_ERR("Invalid time: %f", static_cast<double>(time));
        return 1;
    }
    if (time >= 4) {
        LOG_ERR("Time should not be greater than 4 seconds as we cannot represent it as a uin32_t in ns: %f",
                static_cast<double>(time));
        return 1;
    }

    k_mutex_lock(&motor_lock, K_FOREVER);
    // 1D motion with constant acceleration, solving for a.
    float t = time;
    float v_0 = velocity;
    float x = target_deg;
    float x_0 = static_cast<float>(steps) * DEG_PER_STEP;

    acceleration = CLAMP((x - x_0 - v_0 * t) * 2 / (t * t), -MAX_ACCELERATION, MAX_ACCELERATION);

    cycles_to_run = k_ns_to_cyc_near64(static_cast<uint64_t>(time * 1e9f));
    start_cycle = k_cycle_get_64();
    state = RUNNING;
    k_mutex_unlock(&motor_lock);

    return 0;
}

void throttle_valve_stop() {
    k_mutex_lock(&motor_lock, K_FOREVER);
    acceleration = 0;
    state = STOPPED;
    k_mutex_unlock(&motor_lock);
}

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
