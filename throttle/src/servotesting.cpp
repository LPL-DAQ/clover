#include "throttle_valve.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>




static inline void set_us(const pwm_dt_spec& s, uint32_t us) {
    // Period is already in the DT (20 ms), so set just the pulse:
    pwm_set_pulse_dt(&s, PWM_USEC(us));
}

extern "C" void main(void) {
    if (!device_is_ready(tvc_x.dev) || !device_is_ready(tvc_y.dev)) {
        printk("PWM device(s) not ready\n");
        return;
    }
    // Center both, then sweep opposite for a quick check
    set_us(tvc_x, 1500); set_us(tvc_y, 1500);
    k_msleep(500);
    while (1) {
        for (uint32_t u = 700; u <= 2300; u += 50) {
            set_us(tvc_x, u);
            set_us(tvc_y, 2400 - u);
            k_msleep(15);
        }
        for (uint32_t u = 2300; u >= 700; u -= 50) {
            set_us(tvc_x, u);
            set_us(tvc_y, 2400 - u);
            k_msleep(15);
        }
    }
}