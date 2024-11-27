# cli-example

This program implements a serial port terminal command line interface (CLI)
that exercises all of the features of one or more I2C-connected MCP4728 chips.
The CLI uses the wonderful [embedded-cli](https://github.com/funbiscuit/embedded-cli)
library by Sviatoslav Kokurin (funbiscuit). Both the UART serial port and the
USB Device CDC serial port are supported.

To access the CLI, you will need a serial terminal. If you are using VS Code
and have the Serial Monitor extension installed, set the terminal mode
to On by Clicking the Toggle Terminal Mode button. If you prefer an external
terminal program, on Linux or Mac, you can use something like `minicom`. On Windows,
you use something like `PuTTY` or `Teraterm`.

Type help for a list of commands. You can use the Tab key to complete commands.
The Up and Down arrow recall previous commands and you can use the left and right
arrow keys to edit command lines.

# Hardware
I tested this software using a Raspberry Pi Pico W board wired to either 1 or 2
[Adafruit MCP4728 Quad DAC with EEPROM](https://www.adafruit.com/product/4470)
boards. I did not use the STEMMA QT connectors. Rather, I soldered the headers
to the board and wired the Pin 36 3.3V output of the Raspberry Pi Pico W board
to the VCC pin of the MCP4728 board. I wired Pico W Pin 3 ground to the GND
pin of the MCP4728 board. I wired Pico W Pins 4 and 5 (GPIO 2 and 3) to the MCP4728 board
SDA and SCL pins. I wired Pico W pin 9 (GPIO 6) to the MCP4728 board LDAC pin,
but you can choose whatever unused pin you would like. For tests with a
second MCP4728 board, I wired pin 10 (GPIO 7) to the LDAC pin of the 2nd board.

I also tested this powering the MCP4728 boards with 5V VCC. I buffered the I2C SDA
and SCL pins through two BSS138 FET buffers on one [Adafruit 4-channel
I2C-safe Bi-directional Logic Level Converter - BSS138](https://www.adafruit.com/product/757)
board. The LDAC pins I had to buffer through two buffers of a 74HCT244 chip;
The other two buffers on the Adafruit Bi-directional Logic Level Converter were
not happy fighting against the pulldown resistor on each LDAC pin on the
MCP4728 board.

The rough schematic for the 5V VCC version with two MCP4728 boards as described
above is in the schematic directory. Bypass capacitors and the circuit for the 5V analog
supply are not shown.

# Software

## Building

After you clone this repository, please update the submodules
to get the `embedded-cli` library source, too.
```
git clone https://github.com/rppicomidi/rp2040-mcp4728-lib.git
git submodule update --recursive --init
```

The code was tested with Pico-SDK 2.0 and 2.1. I have tested it using the official
VS Code extension for Raspberry Pi Pico developement. You should be able to load this
project directly to VS Code and build it if you have the extension installed. See
the [Getting started guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).

To build on the command line, you will need to create a build directory and
export the path the `pico-sdk`. For example, if the `rp2040-mcp4728-lib`
directory is a sub-directory of `${PROJ_DIR}` and your `pico-sdk` is
stored in `${PROJ_DIR}/pico-sdk`, and your target board is a Raspberry Pi Pico,
then you need to use the following commands
```
export PICO_BOARD=pico
export PICO_SDK_PATH=${PROJ_DIR}/pico-sdk
cd ${PROJ_DIR}/rp2040-mcp4728-lib/examples/cli-example
mkdir build
cd build
```

From the `build` directory, you can get the default build with these commands
```
cmake ..
make
```

By default, this software compiles assuming a single MCP4728 chip at address 0x60
with LDAC pin unwired on port I2C1 through RP2040 GPIO pins 2 and 3 at 400kbps.
You can configure this software to work with up to 8 MCP4728 chips with LDAC
pins wired on either I2C0 or I2C1 with the I2C port wired through whichever
pins on the RP2040 have the appropriate I2C function. You can change the
configuration by editing the preprocessor define statments in the `cli-example.cpp`
code, or you can pass parameters to the cmake command, or you can set your
command line environment variables before doing a build or before launching
VS Code.

The following cmake defines are supported; pass them to
CMake command line with the `-D` prefix; omit the `-D` prefix if you
are setting the configuration using the shell environment.

```
# Define the number of MCP4728 chips wired to the same I2C port. Can be 1-8
NUM_MCP4728=[1-8]
# Define whether you want to use I2C0 or I2C1
MCP4728_I2C=[i2c0|i2c1]
# Define the I2C baud rate 100000-400000
MCP4728_BAUD=[100000|400000]
# Define the GPIO pin for the I2C SDA
MCP4728_I2C_SDA=[RP2040 GPIO number]
# Define the GPIO pin for the I2C SCL
MCP4728_I2C_SCL=[RP2040 GPIO number]
# Define the I2C address for MCP4728 chip 0, 1, 2, ... [0x60-0x67]
# Note that by default, if there is more than one MCP4728 chip, chip 0 is at I2C address 0x60,
# chip 1 is at I2C address 0x61, chip 2 is at I2C address 0x62, etc.
MCP4728_ADDR[0-7]=[0x60-0x67]
# Define the RP2040 GPIO number of the pin wired to the LDAC pin of MCP4728 chip 0, 1, 2, ...
# Note that by default, all MCP4728 are assumed to have no LDAC pin connected, so
# are defined as rppicomidi::RP2040_MCP4728::no_ldac_gpio
MCP4728_LDAC[0-7]=[RP2040 GPIO number or rppicomidi::RP2040_MCP4728::no_ldac_gpio]
```
For example, to build with 2 MCP4728 chips on I2C0 routed to RP2040 GPIO 8 and 9 with
LDAC pins on GPIO 10 and 11, and all other options default, you can use the folliwng
command sequence to build the software (assumes you have created a build directory
one level below this source code directory).

```
cmake -DNUM_MCP4728=2 -DMCP4728_I2C=i2c0 -DMCP4728_I2C_SDA=8 -DMCP4728_I2C_SCL=9 -DMCP4728_LDAC0=10 -DMCP4728_LDAC1=11 ..
make
```

To set up your build environment as above and then launch VS Code, try this sequence of commands from the Linux or Mac
Terminal, or the Windows Git Bash terminal. Then push the `Configure CMake` and `Compile Project` buttons in the Raspberry Pi Pico plugin Quick Access to build your project.
```
export NUM_MCP4728=2
export MCP4728_I2C=i2c0
export MCP4728_I2C_SDA=8
export MCP4728_I2C_SCL=9
export MCP4728_LDAC0=10
export MCP4728_LDAC1=11
code
```

## Using the CLI

At the time this document was created the following serial port commands
are supported and are displayed on your serial port terminal when you type `help`.

```
 * help
        Print list of commands
 * dac-multi-write
        Configure the DAC channel(s); use dac-multi_write cmd with no args for usage
 * dac-fast-write
        Write DAC code and PD value to all 4 channels; use dac-fast_write cmd with no args for usage
 * dac-read
        Read and print DAC channel information; use dac-read cmd with no args for usage
 * dac-status
        Print the Ready/Busy status and the powered-on status
 * dac-write-eeprom
        Configure DAC registers and program EEPROM to match; use dac-write-eeprom with no args for usage
 * dac-save
        Save the current DAC configuration to EEPROM
 * dac-set-gains
        Configure DAC gain registers for all 4 channels at once; use dac-set-gains with no args for usage
 * dac-set-vrefs
        Configure DAC Vref registers for all 4 channels at once; use dac-set-vrefs with no args for usage
 * dac-set-pds
        Configure DAC PD registers for all 4 channels at once; use dac-set-pds with no args for usage
 * dac-reset
        Send General Call Reset to all I2C devices on the bus
 * dac-wakeup
        Send General Call Wakeup to all I2C devices on the bus. Sets PD to 0 all channels.
 * dac-update
        Send General Call Software Update to all I2C devices on the bus. Updates all DAC outputs at the same time
 * dac-select
        Set the DAC chip to access
 * dac-set-ldac
        Set the logic level for the LDAC signal
 * dac-read-addr
        Use LDAC and general call read address bits command to read the I2C address bits from the DAC
 * dac-write-addr
        Use LDAC and write address bits command to write new the I2C address bits to the DAC 
```
## Changing the I2C address

Unless you order special part numbers for different I2C addresses, the MCP4728 normally ships
with I2C address 0x60. The I2C address is field programmable, but it requires
an I2C command paired with special timing of the LDAC signal. If you have multiple
MCP4728 devices on the same I2C bus and you intend to update them in situ, then
each MCP4728 device must have its LDAC signal wired to a unique RP2040 GPIO pin.
If the MCP4728 VCC is 5V, then the circuit must buffer the 3.3V RP2040 GPIO output
to a 5V logic level; the LDAC signal uses CMOS signaling levels, not TTL. Once the
hardware is set up correctly

1. Configure the software build as described above to set the number of
   MCP4728 chips in the system with the `NUM_MCP4728` option
2. Configure the software build as described above to set the RP2040 GPIO pin wired
   to each MCP4728 device using the `MCP4728_LDAC[0-7]` option. If any other
   differences from default are required, use the other options, too.
3. Build the software and load it to the Pico board or other RP2040 board
4. Start a terminal program and verify the CLI is working properly by entering
   the `help` command.
5. Enter the `select-dac [0-7]` command to select the MCP4728 chip number to access.
6. Enter the `read-addr` command to tell the software the currently programmed
   I2C address. It will probably be 0x60.
7. Enter the `write-addr [0x60-0x67]` command to program the new address to the
   device and to tell the driver software the current I2C address.


For example, consider a system wired as follows. It has 2 MCP4728
devices wired to the I2C1 port on RP2040 GPIO pins 2 and 3. The
first MCP4728 device is device # 0. It will have address 0x60. Its
LDAC pin is wired to RP2040 GPIO pin 6. The second MCP4728 device is
device # 1. It will have I2C addres 0x61. Its LDAC pin is wired to
RP2040 GPIO pin 7. The I2C bus will run at 400kbps.

The I2C port, port pins, baud rate, and I2C addresses are the
defaults. From the command line, you can build the software from the `build`
directory.

```
cmake -DNUM_MCP4728=2 -DMCP4728_LDAC0=6 -DMCP4728_LDAC1=7 ..
make
```

To program the device # 1 I2C address, use the following sequence
of CLI commands

```
select-dac 1
read-addr
write-addr 0x61
```
The first command set the MCP4728 device # 1 as the current device
and tells the software which RP2040 pin is LDAC. 

The second command reads the I2C address off the MCP472 device # 1
and updates the software with the address it read back.
It may be the same as the MCP4728 device # 0 and not what the
MCP4728 was configured to be at build time, which is why this command
is necessary. Without this command, the `write-addr` command that follows
will not work.

The third command writes the new I2C address. Now the MCP4728 # 1
device has the desired I2C address.

Note that if you have another device on the I2C bus that is not
a MCP4728, this procedure may not work. The read-addr and write-addr
commands could have completely different behavior with devices
that are not MCP4728; that behavior may interfere with correct I2C
bus operation.

# Bugs, requests for help, feature requests and pull requests

If you find bugs, need help, have suggestions for improvement or want to
contribute code, please file issues and pull requests on GitHub.
I pay attention to them.

Enjoy.


[assets/mcp4728-cli.svg]: asssets/mcp4728-cli-example.svg