#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/* --- PWM --- */
static const struct pwm_dt_spec ttl =
  PWM_DT_SPEC_GET(DT_NODELABEL(ttl_pwm));

int main(void)
{
    int current_main_thread_count = 0;
    int ret;


    printk("\n--- Testing msleep ---\n");
    k_msleep(5000);
    printk("\n\n--- Zephyr Booting! ---\n");

    printk("--- Peripheral setup successful ---\n");

    while (1) {
        printk("Main thread count: %d\n", current_main_thread_count++);
        k_msleep(500);
    }

    return 0;
}
