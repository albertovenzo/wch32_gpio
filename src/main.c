#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

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
