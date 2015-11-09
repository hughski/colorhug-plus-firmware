		ColorHug Firmware README

Right, you're probably reading this file as you want to make a driver
for the ColorHug hardware.

If you came here for modifying, building and flashing the firmware
please have a look at the Makefiles.

== General Device Information ==

The device is a small USB peripheral that connects to the host computer
and takes XYZ measurements of a specified accuracy. Longer measurements
lead to a more accurate result, but take more time.

The device has two LEDs that can either be switched on or off, or be
programmed to do a repeated flashing with specified on and off durations.

== Firmware Design ==

The device will enter a bootloader if programming the device failed or was
interrupted, or if the device is detached and then reset at runtime.

When the device is in bootloader mode the LEDs will flash in an
alternate pattern to advise the user that it is not fully functional.

== Building and flashing the firmware ==

Before compiling you need to have:

* A checkout of https://github.com/hughsie/m-stack/tree/usb_dfu
* The xc8 compiler installed from Microchip

Then just issue 'make' and then you flash the `bootloader.hex` file to the
device using a hardware programmer, which will also remove the firmware image.

If you just want to flash the firmware do `make install` if you have fwupd
or `dfu-util -D firmware.dfu` will do the same thing.
