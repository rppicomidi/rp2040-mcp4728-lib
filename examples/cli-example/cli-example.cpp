/**
 * MIT License
 *
 * Copyright (c) 2024 rppicomidi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include "pico/stdlib.h"
#include "rp2040_mcp4728_lib.h"
#include "embedded_cli.h"
#include "rp2040_mcp4728_cli.h"
#ifndef RP2040_MCP4728_EXAMPLES_I2C
#define RP2040_MCP4728_EXAMPLES_I2C i2c1
#endif
#ifndef RP2040_MCP4728_EXAMPLES_BAUD
#define RP2040_MCP4728_EXAMPLES_BAUD 400000
#endif
#ifndef RP2040_MCP4728_EXAMPLES_SDA_GPIO
#define RP2040_MCP4728_EXAMPLES_SDA_GPIO 2
#endif
#ifndef RP2040_MCP4728_EXAMPLES_SCL_GPIO
#define RP2040_MCP4728_EXAMPLES_SCL_GPIO 3
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR0
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR0 0x60
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR1
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR1 0x61
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR2
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR2 0x62
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR3
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR3 0x63
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR4
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR4 0x64
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR5
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR5 0x65
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR6
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR6 0x66
#endif
#ifndef RP2040_MCP4728_EXAMPLES_DAC_ADDR7
#define RP2040_MCP4728_EXAMPLES_DAC_ADDR7 0x67
#endif
#ifndef RP2040_MCP4728_EXAMPLES_NUM_DACS
#define RP2040_MCP4728_EXAMPLES_NUM_DACS 1
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC0
#define RP2040_MCP4728_EXAMPLES_LDAC0 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC1
#define RP2040_MCP4728_EXAMPLES_LDAC1 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC2
#define RP2040_MCP4728_EXAMPLES_LDAC2 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC3
#define RP2040_MCP4728_EXAMPLES_LDAC3 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC4
#define RP2040_MCP4728_EXAMPLES_LDAC4 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC5
#define RP2040_MCP4728_EXAMPLES_LDAC5 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC6
#define RP2040_MCP4728_EXAMPLES_LDAC6 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_LDAC7
#define RP2040_MCP4728_EXAMPLES_LDAC7 rppicomidi::RP2040_MCP4728::no_ldac_gpio
#endif
#ifndef RP2040_MCP4728_EXAMPLES_INVERT_LDAC
#define RP2040_MCP4728_EXAMPLES_INVERT_LDAC false
#endif

// Required functions for the CLI

/*
* The following 3 functions are required by the EmbeddedCli library
*/
static void onCommand(const char *name, char *tokens)
{
    printf("Received command: %s\r\n", name);

    for (int i = 0; i < embeddedCliGetTokenCount(tokens); ++i)
    {
        printf("Arg %d : %s\r\n", i, embeddedCliGetToken(tokens, i + 1));
    }
}

static void onCommandFn(EmbeddedCli *embeddedCli, CliCommand *command)
{
    (void)embeddedCli;
    embeddedCliTokenizeArgs(command->args);
    onCommand(command->name == NULL ? "" : command->name, command->args);
}

static void writeCharFn(EmbeddedCli *embeddedCli, char c)
{
    (void)embeddedCli;
    putchar(c);
}

int main()
{
    EmbeddedCli *cli;
    stdio_init_all();
    rppicomidi::Rp2040_i2c_bus i2c_bus(RP2040_MCP4728_EXAMPLES_I2C, RP2040_MCP4728_EXAMPLES_BAUD,
        RP2040_MCP4728_EXAMPLES_SDA_GPIO, RP2040_MCP4728_EXAMPLES_SCL_GPIO);
#if RP2040_MCP4728_EXAMPLES_NUM_DACS <= 0
#error "RP2040_MCP4728_EXAMPLES_NUM_DACS must be at least 1"
#elif RP2040_MCP4728_EXAMPLES_NUM_DACS > 0
    rppicomidi::RP2040_MCP4728 dac_list[RP2040_MCP4728_EXAMPLES_NUM_DACS] = {
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR0, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC0, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 1
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR1, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC1, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 2
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR2, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC2, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 3
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR3, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC3, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 4
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR4, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC4, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 5
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR5, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC5, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 6
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR6, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC6, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 7
        rppicomidi::RP2040_MCP4728(RP2040_MCP4728_EXAMPLES_DAC_ADDR7, &i2c_bus, RP2040_MCP4728_EXAMPLES_LDAC7, RP2040_MCP4728_EXAMPLES_INVERT_LDAC),
#endif
#if RP2040_MCP4728_EXAMPLES_NUM_DACS > 8
#error "The maximum number of MCP4728 chips on a bus is 8"
#endif

    };
    rppicomidi::mcp4728_channel_data chan_dat[4];
    int result = dac_list[0].request_bus(nullptr, nullptr);
    if (result == 1) {
        printf("MCP4728 # 0 I2C bus allocation successful at address 0x%02x\r\n", dac_list[0].get_addr());
    }
    else {
        printf("something is very wrong. Entering infinite loop\r\n");
        for(;;) {

        }
    }

    // CLI initialization
    EmbeddedCliConfig demo_config;

    demo_config.invitation = ">";
    demo_config.rxBufferSize = 128;
    demo_config.cmdBufferSize = 128;
    demo_config.historyBufferSize = 64;
    demo_config.cliBuffer = NULL;
    demo_config.cliBufferSize = 0;
    demo_config.maxBindingCount = rppicomidi::RP2040_MCP4728_cli::get_num_commands();
    demo_config.enableAutoComplete = true;

    cli = embeddedCliNew(&demo_config);
    cli->onCommand = onCommandFn;
    cli->writeChar = writeCharFn;
    rppicomidi::RP2040_MCP4728_cli dac_cli(cli, dac_list, RP2040_MCP4728_EXAMPLES_NUM_DACS, 0);

    int c;
    do {
        c = getchar_timeout_us(0);
    } while(c != PICO_ERROR_TIMEOUT);

    embeddedCliProcess(cli);

    printf("MCP4728 Demo Command Line Interpreter\r\n");
    printf("Type help for more infomation\r\n");
    while (true) {
        dac_cli.get_current_dac()->task();
        c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            embeddedCliReceiveChar(cli, c);
            embeddedCliProcess(cli);
        }
    }
}
