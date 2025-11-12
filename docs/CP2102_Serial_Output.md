# CP2102 Serial Output

This is how to get serial output from a teensy 4.1 board using Zephyr and a CP2102 USB to TTL adapter, using UArt
protocol. Instructions are for Windows, though other OS should be similar.

## Wiring

### Laptop Connection

Plug the red CP2102 adapter into a USB-A port on your laptop. You should see a red light come on. If it does not, and is
a new chip,
there may be a plastic film wrap covering most of the chip. Take a sharp edge, like a razor or knife, and cut it back on
just the usb-a side so that it can fully plug in.

Once plugged in, and the red light having come on, you should see a new device appear in your computer's device manager. It
should be under Ports (COM & LPT), named something like "Silicon Labs CP210x USB to UART Bridge". If the drivers are not
installed, it may be under "Other Devices," in which case you must install and upload the
drivers: https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads.
If on windows, get the CP210x Universal Windows Driver. After installing the drivers, you should be able to see the
device under Ports in device manager.

Note what COM port the device shows up under.

### Teensy Connection

On the CP2102, there are 5 pins: GND, 3V, 5V, RX, TX. Connect the TX pin to RX on the teensy (Pin 0), and
RX pin to the TX on the teensy (Pin 1). Yes, these are meant to be crossed. Connect the CP2102 GND to any teensy GND.
You do not need to connect the other pins,
as the teensy should be powered separately and the CP2102 should be powered by the laptop connection.

## Opening a Serial Connection

You need to install an SSH client on your laptop. On windows, you can use PuTTY. The following will be instructions for
PuTTY, though other options will require similar steps.
PuTTY installation: https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html
Once installed, open PuTTY. Under Session, go to Connection Type and select Serial. Under Serial Line, select the COM
you found in Device Manager (ex. COM11). Under Data, set the baud rate to 115200. Click Open to connect, this should
open
an output window if you are connected correctly.

## Zephyr

### proj.conf

Include the following lines in your proj.conf*:

CONFIG_SERIAL=y

CONFIG_UART_CONSOLE=y

CONFIG_USB_DEVICE_STACK=n

CONFIG_LOG_BACKEND_UART=y

*I am unsure if all of these are fully needed:

### Device Tree (.dts in board subfolder)

In the main section, / {...}, find or make a "chosen" section with the following lines:

chosen {

    zephyr,console = &lpuart6;

    zephyr,serial = &lpuart6;

};

Nothing is needed inside of pinctrl. Go below pinctrl, and make the following two* sections:

&lpuart6 {

status = "okay";

current-speed = <115200>;

};

&cdc_acm_uart0 {

status = "okay";

};

*I am unsure if the cdc_acm_uart0 section is needed.

## Getting Output

In your code, you can now use the printk function to output to the serial port.
After building and flashing, you should see output in the serial window (PuTTY)! This is regardless of whether the
teensy has "disappeared" from TyCommander View

## Troubleshooting
### Unable to Connect
- Check that the COM port is correct. 
- Check that the drivers are installed correctly.
### Gibberish/Non-Character output
- Check that the baud rate is correct (115200).
### No Output
- Check that the device is connected correctly and all wiring is correct.
### CR2102 Test
- Plug the CP2102 into a USB-A port on your computer. You should see a red light come on.
- Short the RX and TX pins on the CP2102 with a putty window open. Then, type on your keyboard: you should see the
output in the putty window. This indicates the chip is working correctly.
