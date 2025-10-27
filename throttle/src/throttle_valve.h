#ifndef CLOVER_THROTTLEVALVE_H
#define CLOVER_THROTTLEVALVE_H


int throttle_valve_init();

// some other functions here?
int throttle_valve_start_calibrate();

int throttle_valve_get_pos();
//throttle_valve_move(???);

int throttle_valve_move(double degrees, double timems);

int throttle_testing();

#endif //CLOVER_THROTTLEVALVE_H
