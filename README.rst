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
- ``src/main.c`` - configures TIM1 channel 3 for a **fixed** 25 us
  period / 12.5 us pulse (50% duty): ``ATRLR=1199``, ``CH3CVR=600`` -
  bit-for-bit the same timer counts as the bare-metal reference's
  ``ConfigurePWM(1200-1, 0, 600)`` - so the two firmwares produce a
  directly comparable signal on a scope. Rather than calling
  ``pwm_set_dt()``, ``configure_pwm_vendor_pattern()`` pokes TIM1's
  registers directly, mirroring the vendor SPL's ``TIM_OC3Init()``
  ordering exactly: disable the channel output (``CC3E``) first,
  reconfigure the compare mode and capture/compare-select bits, write
  the compare value, and only then re-enable the channel - as opposed
  to the Zephyr PWM driver's read-modify-write, which doesn't bracket
  the reconfiguration with a disable/enable. This is app-level only
  (raw register access, see TIM1 offsets in
  ``modules/hal/wch/ch32fun/ch32v003hw.h``); it does not touch the
  Zephyr driver. ``main()`` also toggles the PD4 heartbeat GPIO once
  per loop iteration, and every ~1s prints TIM1's live
  ``PSC``/``ATRLR``/``CH3CVR``/``CNT`` registers via ``printk()`` (a
  no-op unless ``serial.conf`` is used). These are read from the
  running CPU rather than via a halted debugger, since the counter
  datapath registers on this chip read back unreliably while the core
  is halted for debug.
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

If the channel index is already correct, note that ``src/main.c``
currently configures TIM1 CH3 via direct register access
(``configure_pwm_vendor_pattern()``), bypassing the Zephyr PWM driver's
``pwm_set_dt()`` entirely, to test whether the vendor SPL's exact
disable/reconfigure/enable register ordering changes anything. Use
``debug.conf`` to attach GDB (see *Debugging* below) and inspect TIM1's
registers directly. With the core halted:

.. code-block:: console

   (gdb) p/x *(uint32_t*)0x40011000   # GPIOC->CFGLR
   (gdb) p/x *(uint32_t*)0x40010004   # AFIO->PCFR1
   (gdb) p/x *(uint32_t*)0x40012C00   # TIM1->CTLR1  (expect 0x81: ARPE|CEN)
   (gdb) p/x *(uint32_t*)0x40012C1C   # TIM1->CHCTLR2 (expect 0x60: OC3M=PWM1)
   (gdb) p/x *(uint32_t*)0x40012C20   # TIM1->CCER    (expect 0x100: CC3E)
   (gdb) p/x *(uint32_t*)0x40012C44   # TIM1->BDTR    (expect 0x8000: MOE)

Note that ``PSC``/``ATRLR``/``CNT``/``CHxCVR`` (the live counter
datapath, at offsets ``0x28``/``0x2C``/``0x24``/``0x3C`` from
``0x40012C00``) are known to read back unreliably through a halted
debug probe on this chip. Trust a live ``printk()`` readout (see
above, enabled via ``serial.conf``) over a GDB read of those specific
registers.

Building
********

Activate the west virtual environment first:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Release build (small, no debug/serial features)
=================================================

.. code-block:: console

   west build -b ch32v003evt -d build .

Uses ~59% of ROM (9.7 KB / 16 KB).

Debug build (GDB-friendly, variables not optimized out)
=========================================================

.. code-block:: console

   west build -b ch32v003evt -d build . -- -DEXTRA_CONF_FILE=debug.conf

Uses ~88% of ROM (14.5 KB / 16 KB).

Serial build (printk output over USART1, 115200 8N1)
========================================================

.. code-block:: console

   west build -b ch32v003evt -d build . -- -DEXTRA_CONF_FILE=serial.conf

Uses ~81% of ROM (13.2 KB / 16 KB).

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
