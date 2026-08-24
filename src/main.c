#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/* TIM1 channel 3 / PC3, see boards/ch32v003evt.overlay */
static const struct pwm_dt_spec ttl = PWM_DT_SPEC_GET(DT_NODELABEL(ttl_pwm));

/* PD4, wired up by the board's own .dts, enabled in boards/ch32v003evt.overlay */
static const struct gpio_dt_spec heartbeat = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

#define PERIOD_MIN_NS   PWM_USEC(20)
#define PERIOD_MAX_NS   PWM_USEC(40)
#define PERIOD_STEP_NS  PWM_USEC(2)
#define PULSE_STEP_NS   PWM_USEC(1)

int main(void)
{
    if (!device_is_ready(ttl.dev)) {
        return -ENODEV;
    }

    if (!device_is_ready(heartbeat.port)) {
        return -ENODEV;
    }
    gpio_pin_configure_dt(&heartbeat, GPIO_OUTPUT_INACTIVE);

    printk("PWM + heartbeat running\n");

    uint32_t period_ns = PERIOD_MIN_NS;
    int32_t pulse_ns = 0;
    int32_t pulse_step = PULSE_STEP_NS;

    while (1) {
        pwm_set_dt(&ttl, period_ns, (uint32_t)pulse_ns);
        gpio_pin_toggle_dt(&heartbeat);

        pulse_ns += pulse_step;
        if (pulse_ns >= (int32_t)period_ns) {
            pulse_ns = period_ns;
            pulse_step = -pulse_step;

            period_ns += PERIOD_STEP_NS;
            if (period_ns > PERIOD_MAX_NS) {
                period_ns = PERIOD_MIN_NS;
            }
        } else if (pulse_ns <= 0) {
            pulse_ns = 0;
            pulse_step = -pulse_step;
        }

        k_msleep(20);
    }

    return 0;
}
