#ifndef CLOVER_THROTTLEVALVE_H
#define CLOVER_THROTTLEVALVE_H


int throttle_valve_init();

int throttle_valve_start_calibrate();

float throttle_valve_get_pos();

int throttle_valve_move(float degrees, float time);

void throttle_valve_stop();

int throttle_valve_set_open();

int throttle_valve_set_closed();

#endif //CLOVER_THROTTLEVALVE_H
