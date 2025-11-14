#ifndef CLOVER_THROTTLEVALVE_H
#define CLOVER_THROTTLEVALVE_H

#include <cstdint>

int throttle_valve_init();

int throttle_valve_start_calibrate();

float throttle_valve_get_pos();

float throttle_valve_get_velocity();

float throttle_valve_get_acceleration();

uint64_t throttle_valve_get_nsec_per_pulse();

void throttle_valve_move(float degrees);

void throttle_valve_stop();

int throttle_valve_set_open();

int throttle_valve_set_closed();

#endif //CLOVER_THROTTLEVALVE_H
