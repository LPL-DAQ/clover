//
// Created by lpl on 10/24/25.
//

#include <zephyr/logging/log.h>
#include "ThrottleValve.h"

LOG_MODULE_REGISTER(ThrottleValve, CONFIG_LOG_DEFAULT_LEVEL);

int throttle_valve_init() {
    // do some initializaiton work here?
    LOG_INF("Imagine some stuff happening here yknow?");

    return 0;
}

int throttle_valve_start_calibrate() {
    LOG_INF("Imagine I am calibrating?");

    return 0;
}
