#ifndef CLOVER_THROTTLEVALVE_H
#define CLOVER_THROTTLEVALVE_H


int throttle_valve_init();

int throttle_valve_start_calibrate();

double throttle_valve_get_pos();

int throttle_valve_move(double degrees, double timems);

int throttle_testing();

void throttle_valve_set_open();

void throttle_valve_set_closed();

#endif //CLOVER_THROTTLEVALVE_H
