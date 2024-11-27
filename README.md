# rp2040-mcp4728-lib
C++ library for the RP2040 and the MCP4728 4-channel DAC + example code

The `rp2040-mcp4728-lib` implements all of the I2C functions the MCP4728 supports.
There is an API for toggling the LDAC pin if it is used, but none for
or polling the RDY/BSY\ pin.
Use of the LDAC\ pin is optional unless your software needs to
read the MCP4728 I2C address or reprogram the MCP4728 I2C address.
Those functions require timing the LDAC\ pin against the I2C clock. The
`rppicomidi::RP2040_MCP4728::access_addr_bits()` function supports these
two functions. So unless you are reading and writing the I2C address
bits on the MCP4728 chip, you can implement a complete system with
just the I2C SCL and SDA pins. See the MCP4728 data sheet for more
information.

All functions in this library except `rppicomidi::RP2040_MCP4728::access_addr_bits()`
are interrupt driven and non-blocking with callbacks when I2C accesses are complete.
The functions are multi-core safe and application callbacks are called in non-interrupt
context. The application must call the RP2040_MCP4728 class's task() method
peridocially, either as part of a super-loop or in its own thread. The underlying
I2C library is probably useful for other devices, too. It supportes sharing the I2C bus
with other devices.

The `rppicomidi::RP2040_MCP4728::access_addr_bits()` is implemented using
software-controlled bit-banging. It does not use PIO resources because
that function is likely to be called only during board bringup on systems
that require an I2C address other than factory standard.

The `rp2040-mcp4728-cli-lib` implements a command line interface (CLI) for
testing all of the functions in the `rp2040-mcp4728-lib`. It uses the
wonderful embedded-cli library by Sviatoslav Kokurin (funbiscuit), which
must be initialized and included in the build of whatever project uses
it. If your project does not need a CLI, there is no need to use it.

Example code is found in the `examples` directory.
Each example's code is described in the README.md file contained in each
example's directory. At the time of this writing, there is only one
example: a CLI-driven program that exercises all of the features of both
`rp2040-mcp4728-lib` and `rp2040-mcp4728-cli-lib`.
