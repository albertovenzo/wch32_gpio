Simple WCH32 GPIO sample
*************************

A simple project to use GPIO and PWM with the WCH CH32V003 MCU, on the
WCH CH32V003EVT board. Zephyr RTOS is used.

Hardware
********

- Board: ``ch32v003evt`` (WCH CH32V003EVT)
- PWM output: TIM1 channel 3 on pin **PC3**
- Heartbeat GPIO: **PD4** (the board's own ``.dts`` wires this up as
  ``led0``; ``boards/ch32v003evt.overlay`` just enables it)

The chip has 16 KB of flash, so every configuration below is checked
against that budget.

Files
*****

- ``boards/ch32v003evt.overlay`` - enables ``tim1``/``pwm1``, sets the
  ``TIM1_CH3_PC3`` pinctrl (push-pull, max speed), declares the
  ``ttl_pwm`` PWM consumer node (channel index **2**, since the
  ``wch,adtm-pwm`` driver numbers channels 0-based: 2 = physical TIM1
  CH3), overrides ``tim1``'s prescaler to **0** (48 MHz timer clock,
  matching a known-working bare-metal SPL reference that uses
  ``PSC=0``, instead of the SoC dtsi's default of 1 / 24 MHz), and
  enables the board's ``leds`` node (``led0`` -> PD4).
- ``src/main.c`` - pure Zephyr APIs: ``pwm_set_dt()`` drives
  ``ttl_pwm`` with a **fixed** 25 us period / 12.5 us pulse (50%
  duty), computed to land on exactly ``ATRLR=1199``, ``CH3CVR=600`` -
  bit-for-bit the same timer counts as a known-working bare-metal SPL
  reference's ``ConfigurePWM(1200-1, 0, 600)`` - so the two firmwares
  produce a directly comparable signal on a scope. ``main()`` also
  toggles the PD4 heartbeat GPIO once per loop iteration, and prints a
  startup message with ``printk()`` (a no-op unless ``serial.conf`` is
  used).
- ``prj.conf`` - base/release configuration: ``-Os`` size optimization,
  GPIO + PWM enabled, serial/console/printk explicitly disabled (the
  board's defconfig turns them on by default, which doesn't fit this
  project's ROM budget together with debug or serial builds).
- ``debug.conf`` - optional fragment that switches to ``-Og`` so local
  variables are visible in GDB instead of being optimized away.
- ``serial.conf`` - optional fragment that re-enables the USART1
  console (already pinctrl'd as ``zephyr,console`` by the board's
  ``.dts``) so ``printk()`` reaches a serial terminal.

``debug.conf`` and ``serial.conf`` are not combined by default: doing
so overflows the 16 KB ROM by ~2.8 KB on this chip. Build with one or
the other depending on whether you need to single-step/inspect
variables or need UART output.

Verifying the PWM signal on PC3
================================

If the PWM signal isn't visible on a scope, first sanity-check the
overlay: the ``pwms`` cell's channel index is 0-based in the
``wch,adtm-pwm`` driver, so TIM1 physical channel 3 (routed to PC3 by
the ``TIM1_CH3_PC3_0`` pinmux) must be addressed as index ``2``, not
``3``.

If the channel index is already correct, use ``debug.conf`` to attach
GDB (see *Debugging* below) and inspect TIM1's registers directly.
With the core halted:

.. code-block:: console

   (gdb) p/x *(uint32_t*)0x40021018   # RCC->APB2PCENR (expect [0]:AFIOEN, [4]:IOPCEN, [11] TIM1EN)
   (gdb) p/x *(uint32_t*)0x40011000   # GPIOC->CFGLR (expect 0xYYYYBYYY)
   (gdb) p/x *(uint32_t*)0x40012C00   # TIM1->CTLR1  (expect 0x81: ARPE|CEN)
   (gdb) p/x *(uint32_t*)0x40012C2C   # TIM1->ATRLR (expect 0x4AF)
   (gdb) p/x *(uint32_t*)0x40012C1C   # TIM1->CHCTLR2 (expect 0x60: OC3M=PWM1)
   (gdb) p/x *(uint32_t*)0x40012C3C   # TIM1->CH3CVR (expect 0x258)
   (gdb) p/x *(uint32_t*)0x40012C20   # TIM1->CCER    (expect 0x100: CC3E)
   (gdb) p/x *(uint32_t*)0x40012C44   # TIM1->BDTR    (expect 0x8000: MOE)

Note that ``PSC``/``ATRLR``/``CNT``/``CHxCVR`` (the live counter
datapath, at offsets ``0x28``/``0x2C``/``0x24``/``0x3C`` from
``0x40012C00``) are known to read back unreliably through a halted
debug probe on this chip.

Building
********

Activate the west virtual environment first:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Release build (small, no debug/serial features)
=================================================

.. code-block:: console

   west build -b ch32v003evt -d build .

Uses ~69% of ROM (11.3 KB / 16 KB).

Debug build (GDB-friendly, variables not optimized out)
=========================================================

.. code-block:: console

   west build -b ch32v003evt -d build . -- -DEXTRA_CONF_FILE=debug.conf

Uses ~89% of ROM (14.6 KB / 16 KB).

Serial build (printk output over USART1, 115200 8N1)
========================================================

.. code-block:: console

   west build -b ch32v003evt -d build . -- -DEXTRA_CONF_FILE=serial.conf

Uses ~90% of ROM (14.8 KB / 16 KB).

Use ``--pristine`` (or ``-p``) when switching between configurations
in the same build directory, e.g.:

.. code-block:: console

   west build -b ch32v003evt -d build . --pristine -- -DEXTRA_CONF_FILE=debug.conf

Checking ROM/RAM usage
=======================

After a build, the memory report is printed automatically; to re-print
it without rebuilding:

.. code-block:: console

   west build -d build

Flashing
********

.. code-block:: console

   west flash -d build

Debugging
*********

Build with ``debug.conf`` (see above), then attach GDB/OpenOCD as
usual for the CH32V003EVT's WCH-Link debug probe. Local variables
should now be inspectable without "value optimized out".
