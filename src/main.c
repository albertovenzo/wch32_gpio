#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/* TIM1 channel 3 / PC3, see boards/ch32v003evt.overlay. Only used to wait
 * for the underlying pwm1 device (clocks, pinctrl, MOE) to be ready; the
 * actual channel configuration below bypasses pwm_set_dt() and pokes TIM1
 * directly, mirroring the vendor SPL's TIM_OC3Init() disable/reconfigure/
 * enable ordering. App-level only, does not touch the Zephyr PWM driver. */
static const struct pwm_dt_spec ttl = PWM_DT_SPEC_GET(DT_NODELABEL(ttl_pwm));

/* PD4, wired up by the board's own .dts, enabled in boards/ch32v003evt.overlay */
static const struct gpio_dt_spec heartbeat = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/*
 * Fixed 25 us / 50% duty, matching the known-working bare-metal SPL
 * reference exactly: ConfigurePWM(1200-1, 0, 600) at 48 MHz (prescaler=0,
 * see boards/ch32v003evt.overlay) is 1200 cycles = 25 us period, 600
 * cycles = 12.5 us pulse.
 */
#define TIM1_ARR   1199u
#define TIM1_CCR3  600u

/* TIM1 (ADTM) registers, see modules/hal/wch/ch32fun/ch32v003hw.h. */
#define TIM1_BASE    0x40012C00u
#define TIM1_CTLR1   (*(volatile uint32_t *)(TIM1_BASE + 0x00u))
#define TIM1_CHCTLR2 (*(volatile uint32_t *)(TIM1_BASE + 0x1Cu))
#define TIM1_CCER    (*(volatile uint32_t *)(TIM1_BASE + 0x20u))
#define TIM1_CNT     (*(volatile uint32_t *)(TIM1_BASE + 0x24u))
#define TIM1_PSC     (*(volatile uint32_t *)(TIM1_BASE + 0x28u))
#define TIM1_ATRLR   (*(volatile uint32_t *)(TIM1_BASE + 0x2Cu))
#define TIM1_CH3CVR  (*(volatile uint32_t *)(TIM1_BASE + 0x3Cu))
#define TIM1_BDTR    (*(volatile uint32_t *)(TIM1_BASE + 0x44u))

#define TIM_CEN       0x0001u
#define TIM_ARPE      0x0080u
#define TIM_CC3S      0x0003u
#define TIM_OC3PE     0x0008u
#define TIM_OC3M      0x0070u
#define TIM_OC3M_PWM1 0x0060u
#define TIM_CC3E      0x0100u
#define TIM_CC3P      0x0200u
#define TIM_MOE       0x8000u

/*
 * Configure TIM1 channel 3 (PC3) for PWM output, following the vendor SPL's
 * TIM_OC3Init() ordering exactly: disable the channel output first, then
 * reconfigure the compare mode and capture/compare-select bits, write the
 * compare value, and only then re-enable the channel. pwm1's own Zephyr
 * driver init has already enabled ARPE/CEN/MOE and the PC3 pinctrl by the
 * time main() runs, so those are left as-is here (TIM_ARRPreloadConfig and
 * TIM_CtrlPWMOutputs in the vendor code are equivalent no-ops on entry).
 */
static void configure_pwm_vendor_pattern(void)
{
    /* TIM_OC3Init: disable the channel before touching its mode. */
    TIM1_CCER &= ~TIM_CC3E;

    /* TIM_OC3Init: PWM mode 1, capture/compare-select cleared (output mode). */
    TIM1_CHCTLR2 = (TIM1_CHCTLR2 & ~(TIM_OC3M | TIM_CC3S)) | TIM_OC3M_PWM1;

    /* TIM_OC3PreloadConfig(Disable): CCR3 writes take effect immediately. */
    TIM1_CHCTLR2 &= ~TIM_OC3PE;

    /* TIM_OC3Init: compare value, written while the channel is still off. */
    TIM1_CH3CVR = TIM1_CCR3;

    /* TIM_ARRPreloadConfig(Enable) / period. */
    TIM1_CTLR1 |= TIM_ARPE;
    TIM1_ATRLR = TIM1_ARR;

    /* TIM_OC3Init: active-high polarity, channel re-enabled last. */
    TIM1_CCER = (TIM1_CCER & ~TIM_CC3P) | TIM_CC3E;

    /* TIM_CtrlPWMOutputs(Enable) / TIM_Cmd(Enable). */
    TIM1_BDTR |= TIM_MOE;
    TIM1_CTLR1 |= TIM_CEN;
}

int main(void)
{
    if (!device_is_ready(ttl.dev)) {
        return -ENODEV;
    }

    if (!device_is_ready(heartbeat.port)) {
        return -ENODEV;
    }
    gpio_pin_configure_dt(&heartbeat, GPIO_OUTPUT_INACTIVE);

    configure_pwm_vendor_pattern();

    printk("PWM + heartbeat running\n");

    uint32_t count = 0;

    while (1) {
        gpio_pin_toggle_dt(&heartbeat);

        if ((count++ % 50) == 0) {
            printk("TIM1 PSC=%u ATRLR=%u CH3CVR=%u CNT=%u\n",
                   TIM1_PSC, TIM1_ATRLR, TIM1_CH3CVR, TIM1_CNT);
        }

        k_msleep(20);
    }

    return 0;
}
