#ifndef CLOVER_SERVOTESTING_H
#define CLOVER_SERVOTESTING_H


/* Simple arming/idle step for the ESCs (e.g., hold ~1000 µs). */
int servos_init();

/* Move both servos to target degrees over a time window (ms). */
int servo_write_deg(double servo_x_deg, double servo_y_deg, double timems);

/* Set raw throttle pulses for both ESCs in microseconds. */
int esc_write_us(int esc1_us, int esc2_us);

/* Quick bring-up routine: arm ESCs and sweep servos. */
int servotesting_demo();

/* Convenience presets. */
void servo_neutral();  /* servos ~90°, ESCs neutral if used */
void esc_idle();     /* ESCs ~1000 µs, servos unchanged   */

#endif // CLOVER_SERVOTESTING_H
