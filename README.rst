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
  ``ttl_pwm`` PWM consumer node, and enables the board's ``leds`` node
  (``led0`` -> PD4).
- ``src/main.c`` - sweeps the PWM duty cycle and period on ``ttl_pwm``,
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

Uses ~90% of ROM (14.7 KB / 16 KB).

Serial build (printk output over USART1, 115200 8N1)
========================================================

.. code-block:: console

   west build -b ch32v003evt -d build . -- -DEXTRA_CONF_FILE=serial.conf

Uses ~91% of ROM (14.8 KB / 16 KB).

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
