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

/**
 * @class this class implements a command line interpreter interface
 * to the functions that support the MCP4728 quad 12-bit DAC. It depends
 * on the embedded-cli library, which this class assumes is installed in
 * a directory called embedded-cli with the same parent directory
 * as the parent directory to the directory where this library is stored.
 * All MCP4728 functions are supported. Up to 8 MCP4728 devices on the same
 * I2C bus are supported.
 */
#pragma once
#include <cstdint>
#include "embedded_cli.h"
#include "rp2040_mcp4728_lib.h"
namespace rppicomidi
{
class RP2040_MCP4728_cli
{
public:
    RP2040_MCP4728_cli(EmbeddedCli* cli_, RP2040_MCP4728* dac_list_, const uint8_t ndacs_, uint8_t first_dacnum);
    ~RP2040_MCP4728_cli()=default;
    static const uint16_t get_num_commands() { return 16; }
    RP2040_MCP4728* get_current_dac() {return dac;}
protected:
    // MCP4728 callbacks
    static void write_complete(void*);
    static void allocation_successful(void*);
    static void reset_complete(void*);
    static void seq_polling_callback(void* context, bool is_busy, bool is_powered_on);
    static void seq_write_complete(void*);
    static void read_callback(void* context);
    static void status_callback(void*, bool is_busy, bool is_powered_on);
    static void save_cmd_read_callback(void* context);
    static void release_bus_callback(void*);
    // CLI callbacks
    static void on_multi_write(EmbeddedCli *, char *args, void *context);
    static void on_fast_write(EmbeddedCli *, char *args, void *context);
    static void on_read(EmbeddedCli *, char *args, void *context);
    static void on_status(EmbeddedCli *, char *args, void *context);
    static void on_write_eeprom(EmbeddedCli *, char *args, void *context);
    static void on_set_gains(EmbeddedCli *, char *args, void *context);
    static void on_set_vrefs(EmbeddedCli *, char *args, void *context);
    static void on_set_pds(EmbeddedCli *, char *args, void *context);
    static void on_reset(EmbeddedCli *, char *args, void *context);
    static void on_wakeup(EmbeddedCli *, char *args, void *context);
    static void on_update(EmbeddedCli *, char *args, void *context);
    static void on_set_ldac(EmbeddedCli *, char *args, void *context);
    static void on_read_i2c_addr(EmbeddedCli *, char *args, void *context);
    static void on_write_i2c_addr(EmbeddedCli *, char *args, void *context);
    static void on_save(EmbeddedCli *, char *args, void *context);
    // ndacs must be greater than 1 for this command to be added to the CLI
    static void on_select_dac(EmbeddedCli *, char *args, void *context);

    // CLI helper functions
    static void print_multi_write_usage();
    static void print_fast_write_usage();
    static void print_read_usage();
    static void print_write_eeprom_usage();
    static void print_set_gains_usage();
    static void print_set_vrefs_usage();
    static void print_set_pds_usage();
    static void print_on_select_dac_usage(RP2040_MCP4728_cli* context);
    void assign_next_bus();
    EmbeddedCli* cli;
    RP2040_MCP4728* dac_list;
    uint8_t ndacs;
    RP2040_MCP4728* dac; // The currently selected dac
    struct readback_data_s {
        rppicomidi::mcp4728_channel_read_data data[8];
        uint8_t nchan;
    } readback_data;
    uint8_t current_dacnum;
    uint8_t next_dacnum;
private:
    RP2040_MCP4728_cli()=delete;
    RP2040_MCP4728_cli(const RP2040_MCP4728_cli&)=delete;
    RP2040_MCP4728_cli& operator=(const RP2040_MCP4728_cli&)=delete;
};
}
