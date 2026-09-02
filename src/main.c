#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/* TIM1 channel 3 / PC3, see boards/ch32v003evt.overlay */
static const struct pwm_dt_spec ttl = PWM_DT_SPEC_GET(DT_NODELABEL(ttl_pwm));

/* PD4, wired up by the board's own .dts, enabled in boards/ch32v003evt.overlay */
static const struct gpio_dt_spec heartbeat = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/*
 * Fixed 25 us / 50% duty, matching the known-working bare-metal SPL
 * reference exactly: ConfigurePWM(1200-1, 0, 600) at 48 MHz (prescaler=0,
 * see boards/ch32v003evt.overlay) is 1200 cycles = 25 us period, 600
 * cycles = 12.5 us pulse.
 */
#define PERIOD_NS PWM_NSEC(25000)
#define PULSE_NS  PWM_NSEC(12500)

#define WCH_GPIOC_CFGLR (*(volatile uint32_t *)(0x40011000))

int main(void)
{
    WCH_GPIOC_CFGLR = 0x3883B443U;
    if (!device_is_ready(ttl.dev)) {
        return -ENODEV;
    }

    if (!device_is_ready(heartbeat.port)) {
        return -ENODEV;
    }
    gpio_pin_configure_dt(&heartbeat, GPIO_OUTPUT_INACTIVE);

    pwm_set_dt(&ttl, PERIOD_NS, PULSE_NS);

    printk("PWM + heartbeat running\n");

    while (1) {
        gpio_pin_toggle_dt(&heartbeat);
        k_msleep(20);
    }

    return 0;
}
