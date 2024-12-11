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
#include "rp2040_mcp4728_lib.h"
#include <cstring> // for memset
rppicomidi::RP2040_MCP4728::RP2040_MCP4728(uint16_t addr_, Rp2040_i2c_bus* bus_, uint ldac_, bool ldac_invert_) : RP2040_i2c_device(addr_, bus_), ldac_gpio{ldac_}, release_bus_pending{false}
{
    memset(&app_callbacks, 0, sizeof(app_callbacks));
    memset(read_data, 0, sizeof(read_data));
    if (ldac_gpio != no_ldac_gpio) {
        gpio_init(ldac_gpio);
        if (ldac_invert_) {
            gpio_set_outover(ldac_gpio, GPIO_OVERRIDE_INVERT);
        }
        gpio_put(ldac_gpio, true); // set LDAC\ high
        gpio_set_dir(ldac_gpio, true); // make it an output
    }
}

void rppicomidi::RP2040_MCP4728::req_bus_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.req_bus.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::fast_write_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.fast_write.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::multi_write_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.multi_write.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::gain_set_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.set_gain.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::vref_set_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.set_vref.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::pd_set_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.set_pd.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::reset_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.reset.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::wakeup_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.wakeup.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::update_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.update.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::seq_write_eeprom_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.seq_write_eeprom.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::read_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.read.call_callback = true;
}

void rppicomidi::RP2040_MCP4728::status_callback(RP2040_i2c_device* context)
{
    auto ptr=reinterpret_cast<RP2040_MCP4728*>(context);
    ptr->app_callbacks.status.call_callback = true;
}

int rppicomidi::RP2040_MCP4728::request_bus(void (*callback)(void* context), void* context)
{
    app_callbacks.req_bus.callback = callback;
    app_callbacks.req_bus.context = context;
    return bus->request_bus(this, req_bus_callback); 
}

int rppicomidi::RP2040_MCP4728::release_bus(void (*callback)(void* context), void* context)
{
    app_callbacks.rel_bus.callback = callback;
    app_callbacks.rel_bus.context = context;
    int status =  bus->release_bus(this);
    if (status == 0)
        release_bus_pending = true;
    return status;
}

void rppicomidi::RP2040_MCP4728::check_callback(app_callback& app_cb)
{
    if (app_cb.call_callback) {
        bus->enter_critical();
        app_cb.call_callback = false;
        bus->exit_critical();
        if (app_cb.callback != nullptr)
            app_cb.callback(app_cb.context);
    }
}

void rppicomidi::RP2040_MCP4728::bytes2channel_read_data(uint8_t* bytes, mcp4728_channel_read_data* data)
{
    data->chan = (bytes[0] >> 4) & 0x3;
    data->rdy = (bytes[0] & 0x80) != 0;
    data->por = (bytes[0] & 0x40) != 0;
    data->vref = (bytes[1] & 0x80) >> 7;
    data->pd = (bytes[1] >> 5) & 0x3;
    data->gain = (bytes[1] >> 4) & 1;
    data->dac_code = (((uint16_t)bytes[1]&0xf)<< 8) | (uint16_t)bytes[2];
}

void rppicomidi::RP2040_MCP4728::task()
{
    check_callback(app_callbacks.req_bus);
    if (release_bus_pending) {
        int status = bus->release_bus(this);
        assert(status != -1);
        if (status == 1) {
            release_bus_pending = false;
            app_callbacks.rel_bus.callback(app_callbacks.rel_bus.context);
        }
    }
    check_callback(app_callbacks.fast_write);
    check_callback(app_callbacks.multi_write);
    check_callback(app_callbacks.seq_write_eeprom);
    check_callback(app_callbacks.set_gain);
    check_callback(app_callbacks.set_vref);
    check_callback(app_callbacks.set_pd);
    if (app_callbacks.read.call_callback) {
        bus->enter_critical();
        app_callbacks.read.call_callback = false;
        bus->exit_critical();
        // copy and format read_data into the API's array
        auto data = app_callbacks.read.data;
        for (uint8_t chan; chan < app_callbacks.read.nchan; chan++) {
            bytes2channel_read_data(read_data + (chan*3), data + chan);
            data[chan].is_eeprom = (chan & 1) != 0;
        }
        if (app_callbacks.read.callback != nullptr)
            app_callbacks.read.callback(app_callbacks.read.context);
    }
    else if (app_callbacks.status.call_callback) {
        bus->enter_critical();
        app_callbacks.status.call_callback = false;
        bus->exit_critical();
        bool is_bsy = (read_data[0] & 0x80) == 0;
        bool is_powered_on = (read_data[0] & 0x40) != 0;
        if (app_callbacks.status.callback != nullptr) {
            app_callbacks.status.callback(app_callbacks.status.context, is_bsy, is_powered_on);
        }
    }
    else if (app_callbacks.reset.call_callback) {
        if (bus->set_general_call_mode(this, false)) {
            bus->enter_critical();
            app_callbacks.reset.call_callback = false;
            bus->exit_critical();
            if (app_callbacks.reset.callback != nullptr) {
                app_callbacks.reset.callback(app_callbacks.reset.context);
            }
        }
    }
    else if (app_callbacks.wakeup.call_callback) {
        if (bus->set_general_call_mode(this, false)) {
            bus->enter_critical();
            app_callbacks.wakeup.call_callback = false;
            bus->exit_critical();
            if (app_callbacks.wakeup.callback != nullptr) {
                app_callbacks.wakeup.callback(app_callbacks.wakeup.context);
            }
        }
    }
    else if (app_callbacks.update.call_callback) {
        if (bus->set_general_call_mode(this, false)) {
            bus->enter_critical();
            app_callbacks.update.call_callback = false;
            bus->exit_critical();
            if (app_callbacks.update.callback != nullptr) {
                app_callbacks.update.callback(app_callbacks.update.context);
            }
        }
    }
}

bool rppicomidi::RP2040_MCP4728::fast_write(const uint16_t* chan_dat, uint8_t nchan, bool stop, void (*callback)(void* context), void* context)
{
    if (nchan > 4)
        return false;
    uint8_t data[nchan*2];
    app_callbacks.fast_write.callback = callback;
    app_callbacks.fast_write.context = context;
    // make sure data is big endian and limited to 2 bits of 
    // powerdown code (bits 13:12) and 12 bits of DAC code (bits 11:0)
    for (int chan = 0; chan < nchan; chan++) {
        data[chan*2] = (chan_dat[chan] >> 8) & 0x3F;
        data[chan*2+1] = chan_dat[chan] & 0xFF;
    }
    return bus->write(this, false, stop, data, nchan*2, fast_write_callback);
}

bool rppicomidi::RP2040_MCP4728::multi_write(const mcp4728_channel_data* chan_dat, uint8_t nchan, void (*callback)(void* context), void* context)
{
    if (nchan > 4)
        return false;
    uint8_t data[nchan*3];
    app_callbacks.multi_write.callback = callback;
    app_callbacks.multi_write.context = context;
    // format the data for the multi-write command
    for (int chan = 0; chan < nchan; chan++) {
        data[chan*3] = 0x40 | (chan_dat[chan].chan << 1) | chan_dat[chan].udac;
        data[chan*3+1] = (chan_dat[chan].vref << 7) | (chan_dat[chan].pd << 5) | (chan_dat[chan].gain << 4) | ((chan_dat[chan].dac_code >> 8) & 0xF);
        data[chan*3+2] = chan_dat[chan].dac_code & 0xFF;
    }
    return bus->write(this, false, true, data, nchan*3, multi_write_callback);
}

bool rppicomidi::RP2040_MCP4728::read_channels(mcp4728_channel_read_data* chan_dat, uint8_t nchan, void (*callback)(void* context), void* context)
{
    if (nchan > 8)
        return false;
    app_callbacks.read.data = chan_dat;
    app_callbacks.read.nchan = nchan;
    app_callbacks.read.callback = callback;
    app_callbacks.read.context = context;
    memset(read_data, 0, sizeof(read_data));
    return bus->read(this, false, true, read_data, nchan * 3, read_callback);
}

bool rppicomidi::RP2040_MCP4728::poll_status(void (*callback)(void* context, bool is_busy, bool is_powered_on), void* context)
{
    app_callbacks.status.callback = callback;
    app_callbacks.status.context = context;
    read_data[0] = 0;
    return bus->read(this, false, true, read_data, 1, status_callback);
}

bool rppicomidi::RP2040_MCP4728::sequential_write_eeprom(const mcp4728_channel_data* chan_dat, uint8_t nchan, void (*callback)(void* context), void* context)
{
    // the number of channels + the first channel number from 0 must be 4 or fewer
    if (nchan > 4)
        return false;
    app_callbacks.seq_write_eeprom.callback = callback;
    app_callbacks.seq_write_eeprom.context = context;
    uint8_t data[(2*nchan)+1];
    if (nchan == 1) {
        data[0] = 0x58 | (chan_dat[0].chan << 1) | chan_dat[0].udac;
    }
    else {
        // channels A-D correspond to channel bitfield values 0x0-0x3.
        data[0] = 0x50 | ((4 - nchan) << 1) | chan_dat[0].udac;
    }
    for (uint8_t chan = 0; chan < nchan; chan++) {
        data[chan*2+1] = (chan_dat[chan].vref << 7) | (chan_dat[chan].pd << 5) | (chan_dat[chan].gain << 4) | ((chan_dat[chan].dac_code >> 8) & 0xF);
        data[chan*2+2] = chan_dat[chan].dac_code & 0xFF;
    }
    return bus->write(this, false, true, data, sizeof(data), seq_write_eeprom_callback);
}

bool rppicomidi::RP2040_MCP4728::set_all_gains(bool gainA, bool gainB, bool gainC, bool gainD, void (*callback)(void* context), void* context)
{
    app_callbacks.set_gain.callback = callback;
    app_callbacks.set_gain.context = context;
    uint8_t data = 0xC0 | (gainA?0x8:0)|(gainB?0x4:0)|(gainC?0x2:0)|(gainD?0x1:0);
    return bus->write(this, false, true, &data, 1, gain_set_callback);
}

bool rppicomidi::RP2040_MCP4728::set_all_vrefs(bool vrefA, bool vrefB, bool vrefC, bool vrefD, void (*callback)(void* context), void* context)
{
    app_callbacks.set_vref.callback = callback;
    app_callbacks.set_vref.context = context;
    uint8_t data = 0x80 | (vrefA?0x8:0)|(vrefB?0x4:0)|(vrefC?0x2:0)|(vrefD?0x1:0);
    return bus->write(this, false, true, &data, 1, vref_set_callback);
}

bool rppicomidi::RP2040_MCP4728::set_all_pds(uint8_t pdA, uint8_t pdB, uint8_t pdC, uint8_t pdD, void (*callback)(void* context), void* context)
{
    if (pdA>3 || pdB>3 || pdC>3 || pdD>3)
        return false;
    app_callbacks.set_pd.callback = callback;
    app_callbacks.set_pd.context = context;
    uint8_t data[2] = {static_cast<uint8_t>(0xA0 | (pdA << 2) | pdB), static_cast<uint8_t>((pdC << 6) | (pdD << 4))};
    return bus->write(this, false, true, data, 2, pd_set_callback);
}

bool rppicomidi::RP2040_MCP4728::reset(void (*callback)(void* context), void* context)
{
    if (!bus->set_general_call_mode(this, true))
        return false;
    app_callbacks.reset.callback = callback;
    app_callbacks.reset.context = context;
    uint8_t data = 0x06;
    return bus->write(this, false, true, &data, 1, reset_callback);
}

bool rppicomidi::RP2040_MCP4728::wakeup(void (*callback)(void* context), void* context)
{
    if (!bus->set_general_call_mode(this, true))
        return false;
    app_callbacks.wakeup.callback = callback;
    app_callbacks.wakeup.context = context;
    uint8_t data = 0x09;
    return bus->write(this, false, true, &data, 1, wakeup_callback);
}

bool rppicomidi::RP2040_MCP4728::update_all_channels(void (*callback)(void* context), void* context)
{
    if (!bus->set_general_call_mode(this, true))
        return false;
    app_callbacks.update.callback = callback;
    app_callbacks.update.context = context;
    uint8_t data = 0x08;
    return bus->write(this, false, true, &data, 1, update_callback);
}

#define BIT_TIME 2

bool rppicomidi::RP2040_MCP4728::bit_bang_8_bits(uint8_t write_byte, uint8_t& read_byte, bool ignore_nak, uint sda_pin, uint scl_pin, bool change_ldac)
{
    if (ldac_gpio == no_ldac_gpio)
        return false;

    uint8_t mask = 0x80;
    read_byte = 0;
    for (int8_t bitnum = 7; bitnum >= 0; bitnum--) {
        gpio_set_dir(sda_pin, (write_byte & mask) == 0); // change data pin
        sleep_us(BIT_TIME);
        gpio_set_dir(scl_pin, false); // clock rising edge

        uint8_t timeout = 100;
        sleep_us(BIT_TIME);
        while(!gpio_get(scl_pin)) {
            // wait for clock stretching
            sleep_us(2);
            --timeout;
        }
        if (timeout == 0)
            return false;
        read_byte |= gpio_get(sda_pin) ? mask:0;
        gpio_set_dir(scl_pin, true);  // clock falling edge
        sleep_us(BIT_TIME/2);
        if (change_ldac && bitnum == 0) {
            gpio_put(ldac_gpio, false);
        }
        sleep_us(BIT_TIME/2);
        mask >>= 1;
    }
    // do the ACK phase
    gpio_set_dir(sda_pin, false);   // make SDA an input
    sleep_us(BIT_TIME);
    gpio_set_dir(scl_pin, false);   // clock rising edge
    sleep_us(BIT_TIME);
    uint8_t timeout = 100;
    while(!ignore_nak && timeout > 0 && gpio_get(sda_pin)) {
        sleep_us(BIT_TIME);
        timeout--;
    }
    gpio_set_dir(scl_pin, true); // Clock falling edge
    sleep_us(BIT_TIME);
    return timeout != 0;
}

bool rppicomidi::RP2040_MCP4728::access_addr_bits(uint8_t new_addr, uint8_t& read_addr)
{
    if (ldac_gpio == no_ldac_gpio || !bus->deinit_i2c_bus(this)) {
        return false;
    }
    uint64_t final_wait_us = 100;
    // Make the I2C pins GPIO pins instead
    uint sda_pin, scl_pin;
    bus->get_bus_pins(sda_pin, scl_pin);
    gpio_set_dir(sda_pin, false);
    gpio_set_dir(scl_pin, false);
    gpio_put(sda_pin, false);
    gpio_put(scl_pin, false);
    gpio_set_function(sda_pin, GPIO_FUNC_SIO);
    gpio_set_function(scl_pin, GPIO_FUNC_SIO);
    sleep_us(BIT_TIME);
    // Make sure the ldac_pin is a GPIO output initialized to high
    gpio_put(ldac_gpio, true);

    // Send a Start Bit
    gpio_set_dir(sda_pin, true);
    sleep_us(BIT_TIME);
    gpio_set_dir(scl_pin, true);
    sleep_us(BIT_TIME);

    // Send the address
    uint8_t byte = new_addr == 0 ? 0:(addr << 1);
    uint8_t read_byte = 0;

    if (!bit_bang_8_bits(byte, read_byte, false, sda_pin, scl_pin, false)) {
        bus->reinit_i2c_bus(this);
        return false;
    }

    // send the command
    if (new_addr == 0) {
        // read old address command
        byte = 0x0C;
        if (!bit_bang_8_bits(byte, read_byte, false, sda_pin, scl_pin, true)) {
            bus->reinit_i2c_bus(this);
            return false;
        } 
        // Repeated Start
        gpio_set_dir(scl_pin, false); // clock high
        sleep_us(BIT_TIME);
        gpio_set_dir(sda_pin, true);
        sleep_us(BIT_TIME);
        gpio_set_dir(scl_pin, true);
        sleep_us(BIT_TIME);
        byte = 0xC1;
        if (!bit_bang_8_bits(byte, read_byte, false, sda_pin, scl_pin, true)) {
            bus->reinit_i2c_bus(this);
            return false;
        }

        byte = 0xFF; // for read
        if (!bit_bang_8_bits(byte, read_addr, true, sda_pin, scl_pin, false)) {
            bus->reinit_i2c_bus(this);
            return false;
        }
        // Make current address the address that was read back.
        addr = ((read_addr >> 1) & 0x7) | 0x60;
    }
    else {
        // writing a new address. Send command with current address bits encoded
        byte = ((addr & 0x7) << 2) | 0x61;
        if (!bit_bang_8_bits(byte, read_byte, false, sda_pin, scl_pin, true)) {
            bus->reinit_i2c_bus(this);
            return false;
        }
        // Send the new address
        byte = ((new_addr & 0x7) << 2) | 0x62;
        if (!bit_bang_8_bits(byte, read_byte, false, sda_pin, scl_pin, false)) {
            bus->reinit_i2c_bus(this);
            return false;
        }
        // Send new address confirmation
        byte = ((new_addr & 0x7) << 2) | 0x63;
        if (!bit_bang_8_bits(byte, read_byte, false, sda_pin, scl_pin, false)) {
            bus->reinit_i2c_bus(this);
            return false;
        }
        // The new address is active. Update the target address
        addr = new_addr;
        final_wait_us = 60000; // write time speicifed max is 50ms. Pad it some
    }
    gpio_put(ldac_gpio, true);   // Done with LDAC\ signaling

    
    // Stop condition
    gpio_set_dir(sda_pin, true);
    sleep_us(BIT_TIME);
    gpio_set_dir(scl_pin, false);
    sleep_us(BIT_TIME);
    gpio_set_dir(sda_pin, false);
    sleep_us(final_wait_us);
    // restore I2C function
    if (!bus->reinit_i2c_bus(this)) {
        return false;
    }
    return true;
}

bool rppicomidi::RP2040_MCP4728::set_ldac_pin(bool is_high)
{
    if (ldac_gpio == no_ldac_gpio)
        return false;
    gpio_put(ldac_gpio, is_high);
    return true;
}