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
#include "rp2040_mcp4728_cli.h"
#include "pico/stdlib.h"

rppicomidi::RP2040_MCP4728_cli::RP2040_MCP4728_cli(EmbeddedCli* cli_, RP2040_MCP4728* dac_list_, const uint8_t ndacs_, uint8_t first_dacnum) : cli{cli_}, dac_list{dac_list_},
        ndacs{ndacs_}, dac{dac_list_+first_dacnum}, current_dacnum{first_dacnum}, next_dacnum{first_dacnum}
{
    int result = dac->request_bus(allocation_successful, nullptr);
    assert(result == 1);
    bool add_result = embeddedCliAddBinding(cli, {
            "dac-multi-write",
            "Configure the DAC channel(s); use dac-multi_write cmd with no args for usage",
            true,
            this,
            on_multi_write
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-fast-write",
            "Write DAC code and PD value to all 4 channels; use dac-fast_write cmd with no args for usage",
            true,
            this,
            on_fast_write
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-read",
            "Read and print DAC channel information; use dac-read cmd with no args for usage",
            true,
            this,
            on_read
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-status",
            "Print the Ready/Busy status and the powered-on status",
            true,
            this,
            on_status
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-write-eeprom",
            "Configure DAC registers and program EEPROM to match; use dac-write-eeprom with no args for usage",
            true,
            this,
            on_write_eeprom
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-save",
            "Save the current DAC configuration to EEPROM",
            true,
            this,
            on_save
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-set-gains",
            "Configure DAC gain registers for all 4 channels at once; use dac-set-gains with no args for usage",
            true,
            this,
            on_set_gains
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-set-vrefs",
            "Configure DAC Vref registers for all 4 channels at once; use dac-set-vrefs with no args for usage",
            true,
            this,
            on_set_vrefs
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-set-pds",
            "Configure DAC PD registers for all 4 channels at once; use dac-set-pds with no args for usage",
            true,
            this,
            on_set_pds
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-reset",
            "Send General Call Reset to all I2C devices on the bus",
            true,
            this,
            on_reset
    });
    assert(add_result);
    (void)add_result; // in case not in debug mode
    // flush out junk from the keyboard buffer
    add_result = embeddedCliAddBinding(cli, {
            "dac-wakeup",
            "Send General Call Wakeup to all I2C devices on the bus. Sets PD to 0 all channels.",
            true,
            this,
            on_wakeup
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-update",
            "Send General Call Software Update to all I2C devices on the bus. Updates all DAC outputs at the same time",
            true,
            this,
            on_reset
    });
    assert(add_result);
    if (ndacs > 1) {
        add_result = embeddedCliAddBinding(cli, {
            "dac-select",
            "Set the DAC chip to access",
            true,
            this,
            on_select_dac
        });
    }
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-set-ldac",
            "Set the logic level for the LDAC signal",
            true,
            nullptr,
            on_set_ldac
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-read-addr",
            "Use LDAC and general call read address bits command to read the I2C address bits from the DAC",
            true,
            this,
            on_read_i2c_addr
    });
    assert(add_result);
    add_result = embeddedCliAddBinding(cli, {
            "dac-write-addr",
            "Use LDAC and write address bits command to write new the I2C address bits to the DAC",
            true,
            this,
            on_write_i2c_addr
    });
    assert(add_result);
    (void)add_result;
}

void rppicomidi::RP2040_MCP4728_cli::write_complete(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    printf("MCP4728 # %u write complete\r\n", me->current_dacnum);
}

void rppicomidi::RP2040_MCP4728_cli::allocation_successful(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    printf("MCP4728 # %u deferred allocation successful\r\n", me->current_dacnum);
}

void rppicomidi::RP2040_MCP4728_cli::reset_complete(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    printf("reset complete\r\n", me->current_dacnum);
}

void rppicomidi::RP2040_MCP4728_cli::seq_polling_callback(void* context, bool is_busy, bool is_powered_on)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    if (is_busy) {
        // poll again
        me->dac->poll_status(seq_polling_callback, me);
    }
    else {
        printf("MCP4728 # %u programming complete\r\n", me->current_dacnum);
    }
}

void rppicomidi::RP2040_MCP4728_cli::seq_write_complete(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    me->dac->poll_status(seq_polling_callback, me);
    printf("MCP4728 # %u registers programmed.\r\nPolling DAC EEPROM...", me->current_dacnum);
}

void rppicomidi::RP2040_MCP4728_cli::print_multi_write_usage()
{
    printf("usage:\r\n\tdac-multi_write channel[A-D] G[0|1] Vref[0|1] PD[0-3] UDAC[0|1] Code[0-4095]...\r\n\r\n");
    printf("Channel can be A, B, C or D\r\n");
    printf("G=0 means gain=1; G=1 means gain=2 if Vref=1 and Vdd>4.096V\r\n");
    printf("Vref=0 means Vref=Vdd; Vref=1 means Vref=2.048V\r\n");
    printf("PD=0 means out on; PD=1 means out off and 1kOhm to gnd;\r\nPD=2 means out off and 100kOhm to gnd; PD=3 means out off and 500kOhm to gnd\r\n");
    printf("UDAC=0 means update output after channel data write; UDAC=1 writes registers but does not update the output\r\n");
    printf("Code is the 12-bit DAC code 0-4095\r\n");
    printf("example: Set up channels B and C for Gain=1, Vref=Vdd, Outputs On, update immediately, output Vdd/2 and Vdd/4\r\n");
    printf("\tdac-multi_write B 0 0 0 0 2048 C 0 0 0 0 1024\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::on_multi_write(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);

    // must be 6 arguments per DAC channel and no more than 4 channels
    if (argc < 6 || ((argc % 6) != 0) || (argc/6) > 4) {
        print_multi_write_usage();
    }
    else {
        const uint8_t nchan = argc/6;
        rppicomidi::mcp4728_channel_data chan_dat[nchan];
        for (uint8_t chan = 0; chan < nchan; chan++) {
            // Get channel letter
            const char* ch = embeddedCliGetToken(args, 1 + (chan*6));
            if (strlen(ch) != 1) {
                print_multi_write_usage();
                return;
            }
            chan_dat[chan].chan = toupper(ch[0]) - 'A';
            if (chan_dat[chan].chan > 3) {
                print_multi_write_usage();
                return;
            }
            chan_dat[chan].gain = atoi(embeddedCliGetToken(args, 2 + (chan*6)));
            if (chan_dat[chan].gain > 1) {
                print_multi_write_usage();
                return;
            }
            chan_dat[chan].vref = atoi(embeddedCliGetToken(args, 3 + (chan*6)));
            if (chan_dat[chan].vref > 1) {
                print_multi_write_usage();
                return;
            }
            chan_dat[chan].pd = atoi(embeddedCliGetToken(args, 4 + (chan*6)));
            if (chan_dat[chan].pd > 3) {
                print_multi_write_usage();
                return;
            }
            chan_dat[chan].udac = atoi(embeddedCliGetToken(args, 5 + (chan*6)));
            if (chan_dat[chan].udac > 1) {
                print_multi_write_usage();
                return;
            }
            chan_dat[chan].dac_code = atoi(embeddedCliGetToken(args, 6 + (chan*6)));
            if (chan_dat[chan].dac_code > 4095) {
                print_multi_write_usage();
                return;
            }
        }
        if (!me->dac->multi_write(chan_dat, nchan, write_complete, me)) {
            printf("error writing to MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_fast_write_usage()
{
    printf("usage:\r\n\tdac-fast_write PDA[0-3] CodeA[0-4095] PDB[0-3] CodeB[0-4095] PDC[0-3] CodeC[0-4095] PDD[0-3] CodeD[0-4095] stop[0|1]\r\n\r\n");
    printf("Set the PD code and DAC output code for all 4 channels A-D\r\n");
    printf("In the following, substitute channel A, B, C, or D for ?\r\n");
    printf("PD?=0 means out on; PD=1 means out off and 1kOhm to gnd;\r\nPD=2 means out off and 100kOhm to gnd; PD=3 means out off and 500kOhm to gnd\r\n");
    printf("Code? is the 12-bit DAC code 0-4095\r\n");
    printf("stop=0 omits I2C Stop so subsquent fast_write commands save one byte.\r\nUse stop=1 to send commands other than fast_write after this one\r\n");
    printf("example: all outputs powered on, A=1024, B=2048, C=3072, D=4095 then stop:\r\n");
    printf("\tdac-fast_write 0 1024 0 2048 0 3072 0 4095 1\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::on_fast_write(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    // must be 9 arguments (PD and DAC code for all 4 channels plus stop 0 or 1)
    if (argc != 9) {
        print_fast_write_usage();
    }
    else {
        uint16_t data[4];
        bool stop = false;
        for (uint8_t chan = 0; chan < 4; chan++) {
            uint16_t pd = atoi(embeddedCliGetToken(args, 1+(chan*2)));
            if (pd > 3) {
                print_fast_write_usage();
                return;
            }
            uint16_t code = atoi(embeddedCliGetToken(args, 2+(chan*2)));
            if (code > 4095) {
                print_fast_write_usage();
                return;
            }
            data[chan] = (pd << 12) | code;
        }
        uint8_t stopval = atoi(embeddedCliGetToken(args, 9));
        if (stopval > 1) {
            print_fast_write_usage();
            return;
        }
        stop = stopval == 1;
        if (!me->dac->fast_write(data, 4, stop, write_complete, me)) {
            printf("error writing to MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_read_usage()
{
    printf("usage:\r\n\tdac-read nchan\r\n");
    printf("Read back alternate DAC output values and EEPROM values\r\n");
    printf("nchan is the number of channel data items to read. Each of DAC output register\r\n");
    printf("and the EEPROM register count as one chanel data item.\r\n");
    printf("For example, If nchan is 2, read both the DAC output register and the EEPROM\r\n");
    printf("register for DAC output A. If nchan is 4, then read each register for DAC A & B.\r\n");
    printf("If nchan is odd, then the last channel's EEPROM register will not be read\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::read_callback(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    auto rb = &me->readback_data;
    printf("MCP4728 # %u readback data:\r\n", me->current_dacnum);
    for (uint8_t chan = 0; chan < rb->nchan; chan++) {
        printf("channel %c %s registers:\r\n", 'A'+rb->data[chan].chan, rb->data[chan].is_eeprom ? "EEPROM":"DAC Output");
        printf("POR=%u RDY=%u \r\n", rb->data[chan].por, rb->data[chan].rdy);
        printf("code=%u G=%u PD=%u Vref=%u\r\n", rb->data[chan].dac_code, rb->data[chan].gain, rb->data[chan].pd, rb->data[chan].vref);
    }
}

void rppicomidi::RP2040_MCP4728_cli::status_callback(void* context, bool is_busy, bool is_powered_on)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    printf("MCP4728 # %u is %sbusy. MCP4728 # %u is %spowered on.\r\n", me->current_dacnum, is_busy?"":"not ", me->current_dacnum, is_powered_on?"":"not ");
}

void rppicomidi::RP2040_MCP4728_cli::on_read(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    // must be 1 argument (nchan, and 0<nchan<=8)
    if (argc != 1) {
        print_read_usage();
    }
    else {
        uint8_t nchan = atoi(embeddedCliGetToken(args, 1));
        if (nchan == 0 || nchan > 8) {
            print_read_usage();
            return;
        }
        me->readback_data.nchan = nchan;
        if (!me->dac->read_channels(me->readback_data.data,me->readback_data.nchan, read_callback, me)) {
            printf("error starting MCP4728 # %u read\r\n", me->current_dacnum);
        }
        else {
            printf("read MCP4728 # %u started\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_status(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 0) {
        printf("Print the Ready/Busy status and the powered-on status. usage: status");
    }
    else if (!me->dac->poll_status(status_callback, &me->readback_data)) {
        printf("error starting MCP4728 # %u read\r\n", me->current_dacnum);
    }
    else {
        printf("read MCP4728 # %u started\r\n", me->current_dacnum);
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_write_eeprom_usage()
{
    printf("usage:\r\n\tdac-write-eeprom channel[A-D]|nchan UDAC[0|1] G[0|1] Vref[0|1] PD[0-3] Code[0-4095]...\r\n");
    printf("channel can be A, B, C or D for single channel write, or specify a nchan=2, 3, or 4\r\n");
    printf("to write to channels C-D, B-D, or A-D, respectively.\r\n");
    printf("UDAC=0 means update each output after channel data write; UDAC=1 writes registers but does not update the outputs\r\n");
    printf("G=0 means gain=1; G=1 means gain=2 if Vref=1 and Vdd>4.096V\r\n");
    printf("Vref=0 means Vref=Vdd; Vref=1 means Vref=2.048V\r\n");
    printf("PD=0 means out on; PD=1 means out off and 1kOhm to gnd;\r\nPD=2 means out off and 100kOhm to gnd; PD=3 means out off and 500kOhm to gnd\r\n");
    printf("Code is the 12-bit DAC code 0-4095\r\n");
    printf("example: Set up channels C and D to update immediately with Gain=1, Vref=Vdd, Outputs On,\n\r");
    printf("output Vdd/2 and Vdd/ and then program EEPROM for channels C and D\r\n");
    printf("\twrite-eeprom 2 0 0 0 0 2048 0 0 0 1024\r\n");
    printf("example: Set up channel B, update by LDAC\\, for Gain=1, Vref=2.048V, Outputs On, \n\r");
    printf("output 100mV\r\n");
    printf("\tdac-write-eeprom B 1 0 1 0 200\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::on_write_eeprom(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc < 6 || ((argc-2) %4) != 0) {
        print_write_eeprom_usage();
    }
    else {
        const uint8_t nchan = (argc-2)/4;
        rppicomidi::mcp4728_channel_data chan_dat[nchan];

        // Get channel letter
        const char* ch = embeddedCliGetToken(args, 1);
        if (strlen(ch) != 1) {
            print_write_eeprom_usage();
            return;
        }
        if (ch[0] > '1' && ch[0] <= '4') {
            // then it is a number of channels.
            if (nchan != (ch[0] - '0')) {
                print_write_eeprom_usage();
                return;
            }
        }
        else if (nchan == 1) {
            char chan = toupper(ch[0]);
            if (chan >= 'A' && chan <= 'D') {
                chan_dat[0].chan = chan - 'A';
            }
            else {
                print_write_eeprom_usage();
                return;
            }
        }
        else {
            print_write_eeprom_usage();
            return;
        }

        chan_dat[0].udac = atoi(embeddedCliGetToken(args, 2));
        if (chan_dat[0].udac > 1) {
            print_write_eeprom_usage();
            return;
        }
        for (uint8_t chan = 0; chan < nchan; chan++) {
            chan_dat[chan].gain = atoi(embeddedCliGetToken(args, 3 + (chan*4)));
            if (chan_dat[chan].gain > 1) {
                print_write_eeprom_usage();
                return;
            }
            chan_dat[chan].vref = atoi(embeddedCliGetToken(args, 4 + (chan*4)));
            if (chan_dat[chan].vref > 1) {
                print_write_eeprom_usage();
                return;
            }
            chan_dat[chan].pd = atoi(embeddedCliGetToken(args, 5 + (chan*4)));
            if (chan_dat[chan].pd > 3) {
                print_write_eeprom_usage();
                return;
            }
            chan_dat[chan].dac_code = atoi(embeddedCliGetToken(args, 6 + (chan*4)));
            if (chan_dat[chan].dac_code > 4095) {
                print_write_eeprom_usage();
                return;
            }
        }
        if (!me->dac->sequential_write_eeprom(chan_dat, nchan, seq_write_complete, me)) {
            printf("error writing EEPROM on MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_set_gains_usage()
{
    printf("usage:\r\n\tdac-set-gains gainA gainB gainC gainD\r\n");
    printf("gainx=0 means gain of 1; gainx=1 means gain of 2\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::on_set_gains(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 4) {
        print_set_gains_usage();
    }
    else {
        bool gains[4];
        for (uint8_t idx = 1; idx <= 4; idx++) {
            const char* tok = embeddedCliGetToken(args, idx);
            if (strlen(tok) != 1 || (tok[0] != '0' && tok[0] !='1')) {
                print_set_gains_usage();
                return;
            }
            gains[idx-1] = tok[0] == '1';
        }
        if (!me->dac->set_all_gains(gains[0], gains[1], gains[2], gains[3], write_complete, me)) {
            printf("error writing to MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_set_vrefs_usage()
{
    printf("usage:\r\n\tdac-set-vrefs vrefA vrefB vrefC vrefD\r\n");
    printf("vrefx=0 means Vref is Vdd; vrefx=1 means Vref=2.048V\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::on_set_vrefs(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 4) {
        print_set_vrefs_usage();
    }
    else {
        bool vrefs[4];
        for (uint8_t idx = 1; idx <= 4; idx++) {
            const char* tok = embeddedCliGetToken(args, idx);
            if (strlen(tok) != 1 || (tok[0] != '0' && tok[0] !='1')) {
                print_set_vrefs_usage();
                return;
            }
            vrefs[idx-1] = tok[0] == '1';
        }
        if (!me->dac->set_all_vrefs(vrefs[0], vrefs[1], vrefs[2], vrefs[3], write_complete, nullptr)) {
            printf("error writing to MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_set_pds_usage()
{
    printf("usage:\r\n\tdac-set-pds pdA pdB pdC pdD\r\n");
    printf("pdx=0 means channel x is on;\r\n");
    printf("pdx=1 means channel x is off with output shorted to ground via a 1kohm resistor\r\n");
    printf("pdx=2 means channel x is off with output shorted to ground via a 100kohm resistor\r\n");
    printf("pdx=3 means channel x is off with output shorted to ground via a 500kohm resistor\r\n");
}

void rppicomidi::RP2040_MCP4728_cli::on_set_pds(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 4) {
        print_set_pds_usage();
    }
    else {
        uint8_t pds[4];
        for (uint8_t idx = 1; idx <= 4; idx++) {
            const char* tok = embeddedCliGetToken(args, idx);
            if (strlen(tok) != 1 || (tok[0] != '0' && tok[0] !='1' && tok[0]!='2' && tok[0]!='3')) {
                print_set_pds_usage();
                return;
            }
            pds[idx-1] = tok[0] - '0';
        }
        if (!me->dac->set_all_pds(pds[0], pds[1], pds[2], pds[3], write_complete, me)) {
            printf("Error writing to MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_reset(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 0) {
        printf("usage: dac-reset\r\n");
    }
    else {
        if (!me->dac->reset(reset_complete, me)) {
            printf("error sending reset\r\n");
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_wakeup(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 0) {
        printf("usage: dac-wakeup\r\n");
    }
    else {
        if (!me->dac->wakeup(reset_complete, me)) {
            printf("error sending wakeup\r\n");
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_update(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 0) {
        printf("usage: dac-update\r\n");
    }
    else {
        if (!me->dac->update_all_channels(reset_complete, me)) {
            printf("error sending update\r\n");
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::print_on_select_dac_usage(RP2040_MCP4728_cli* context)
{
    printf("usage: dac-select 0-%u\r\n", context->ndacs-1);
}

void rppicomidi::RP2040_MCP4728_cli::assign_next_bus()
{
    current_dacnum = next_dacnum;
    dac = dac_list + next_dacnum;
    int result = dac->request_bus(allocation_successful, this);
    if (result == -1) {
        printf("bus allocation problem\r\n");
    }
    else if (result == 0) {
        printf("bus allocation deferred until other device releases the bus\r\n");
    }
    else {
        printf("MCP4728 # %u is currently accessible on the I2C bus at address 0x%02x\r\n", current_dacnum, dac->get_addr());
    }
}

void rppicomidi::RP2040_MCP4728_cli::release_bus_callback(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    printf("bus released\r\n");
    me->assign_next_bus();
}

void rppicomidi::RP2040_MCP4728_cli::on_select_dac(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    if (me->ndacs == 1) {
        printf("There is only one MCP4728 chip on this I2C bus. Nothing to select.\r\n");
        return;
    }
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 1) {
        print_on_select_dac_usage(me);
    }
    else {
        uint8_t dacnum = atoi(embeddedCliGetToken(args, 1));
        if (dacnum >= me->ndacs) {
            print_on_select_dac_usage(me);
        }
        else {
            if (me->dac == me->dac_list+dacnum) {
                printf("DAC chip %u already selected\r\n", dacnum);
            }
            else {
                me->next_dacnum = dacnum;
                int result = me->dac->release_bus(release_bus_callback, me);
                if (result == 1) {
                    me->assign_next_bus();
                }
                else if (result == 0) {
                    printf("bus release deferred\r\n");
                }
                else {
                    printf("MCP4728 # %u never had control of the I2C bus\r\n", me->current_dacnum);
                }
            }
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_set_ldac(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 1) {
        printf("usage: set-ldac 0|1\r\n");
    }
    else {
        int val = atoi(embeddedCliGetToken(args, 1));
        if (val > 1 || val < 0)
            printf("usage: dac-set-ldac 0|1\r\n");
        else if (!me->dac->set_ldac_pin(val==1)) {
            printf("error: no LDAC pin is defined for MCP4728 # %u\r\n", me->current_dacnum);
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_read_i2c_addr(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 0) {
        printf("usage: dac-read-addr\r\n");
    }
    else if (!me->dac->set_ldac_pin(true)) {
        printf("error: no LDAC pin is defined for MCP4728 # %u\r\n", me->current_dacnum);
    }
    else {
        uint8_t read_addr;
        if (!me->dac->access_addr_bits(0, read_addr)) {
            printf("problem reading address bits from DAC\r\n");
        }
        else {
            printf("MCP4728 # %u raw value read =0x%02x\r\n", me->current_dacnum, read_addr);
            printf("I2C address in EEPROM=%02x in input register=%02x\r\n",
                0x60|(((read_addr)>>5) & 0x7), 0x60|(((read_addr)>>1) & 0x7));
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_write_i2c_addr(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 1) {
        printf("usage: dac-write-addr [0x60-0x67]\r\n");
    }
    else if (!me->dac->set_ldac_pin(true)) {
        printf("error: no LDAC pin is defined for MCP4728 # %u\r\n", me->current_dacnum);
    }
    else {
        uint8_t new_addr = strtol(embeddedCliGetToken(args, 1), nullptr, 16);
        if (new_addr >= 0x60 && new_addr <= 0x67) {
            uint8_t dummy;
            if (!me->dac->access_addr_bits(new_addr, dummy)) {
                printf("problem writing address bits to MCP4728 # %u\r\n", me->current_dacnum);
            }
            else {
                printf("new EEPROM address 0x%02x written to MCP4728 # %u\r\n", new_addr, me->current_dacnum);
            }
        }
        else {
            printf("usage: dac-write-addr [0x60-0x67]\r\n");
        }
    }
}

void rppicomidi::RP2040_MCP4728_cli::save_cmd_read_callback(void* context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    rppicomidi::mcp4728_channel_data data[4];
    printf("Read of MCP4728 # %u complete. Starting EEPROM write\r\n", me->current_dacnum);
    for (uint8_t chan = 0; chan < 4; chan++) {
        data[chan].chan = me->readback_data.data[chan*2].chan;
        data[chan].dac_code = me->readback_data.data[chan*2].dac_code;
        data[chan].gain = me->readback_data.data[chan*2].gain;
        data[chan].pd = me->readback_data.data[chan*2].pd;
        data[chan].vref = me->readback_data.data[chan*2].vref;
        data[chan].udac = 1; // don't update the outputs as long as LDAC is high
    }

    if (!me->dac->sequential_write_eeprom(data, 4, write_complete, context)) {
        printf("error writing to MCP4728 # %u\r\n", me->current_dacnum);
    }
}

void rppicomidi::RP2040_MCP4728_cli::on_save(EmbeddedCli *, char *args, void *context)
{
    auto me = reinterpret_cast<RP2040_MCP4728_cli*>(context);
    uint16_t argc = embeddedCliGetTokenCount(args);
    if (argc != 0) {
        printf("usage: dac-save\r\n");
    }
    else {
        if (!me->dac->read_channels(me->readback_data.data, me->readback_data.nchan, save_cmd_read_callback, me)) {
            printf("error starting MCP4728 # %u read\r\n", me->current_dacnum);
        }
        else {
            printf("reading MCP4728 # %u input registers\r\n", me->current_dacnum);
        }
    }
}
