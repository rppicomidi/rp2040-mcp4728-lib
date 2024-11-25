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
 * See rp2040-i2c-lib.h for a description of the classes implemented here
 */
#include "rp2040_i2c_lib.h"
#include <cstring> // memset
/* static variables */
rppicomidi::Rp2040_i2c_bus* rppicomidi::Rp2040_i2c_bus::i2c0_irq_context = nullptr;
rppicomidi::Rp2040_i2c_bus* rppicomidi::Rp2040_i2c_bus::i2c1_irq_context = nullptr;

rppicomidi::Rp2040_i2c_bus::Rp2040_i2c_bus(i2c_inst_t* i2c_ , uint baudrate_, uint sda_pin_, uint scl_pin_) : i2c_bus{i2c_}, baudrate{baudrate_}, sda_pin{sda_pin_}, scl_pin{scl_pin_}
{
    critical_section_init(&crit_sec);
#if 0
    i2c_init(i2c_bus, baudrate);
    set_bus_pins(sda_pin, scl_pin);
    i2c_bus->hw->intr_mask = 0; // disable all I2C interrupts
    if (i2c_bus == i2c0) {
        i2c0_irq_context = this;
        irq_add_shared_handler(I2C0_IRQ, i2c0_irq_handler, PICO_DEFAULT_IRQ_PRIORITY);
        irq_set_enabled(I2C0_IRQ, true);
    }
    else {
        assert(i2c_bus == i2c1);
        i2c1_irq_context = this;
        irq_add_shared_handler(I2C1_IRQ, i2c1_irq_handler, PICO_DEFAULT_IRQ_PRIORITY);
        irq_set_enabled(I2C1_IRQ, true);
    }
#endif
    init_bus();
    memset(&current_transfer, 0, sizeof(current_transfer));
}

void rppicomidi::Rp2040_i2c_bus::set_bus_pins(uint sda_pin_, uint scl_pin_)
{
    // if the pin is not the current pin, make the current pin an input
    if (sda_pin != sda_pin_)
    {
        gpio_set_function(sda_pin, GPIO_FUNC_SIO);
        gpio_set_dir(sda_pin, false);
    }
    // if the pin is not the current pin, make the current pin an input
    if (scl_pin != scl_pin_)
    {
        gpio_set_function(scl_pin, GPIO_FUNC_SIO);
        gpio_set_dir(scl_pin, false);
    }
    gpio_set_function(sda_pin_, GPIO_FUNC_I2C);
    sda_pin = sda_pin_;
    gpio_set_function(scl_pin_, GPIO_FUNC_I2C);
    scl_pin = scl_pin_;
}

bool rppicomidi::Rp2040_i2c_bus::set_bus_pins(RP2040_i2c_device* dev, uint sda_pin_, uint scl_pin_)
{
    bool result = false;
    critical_section_enter_blocking(&crit_sec);
    if (is_active_device(dev)) {
        init_bus();
        result = true;
    }
    critical_section_exit(&crit_sec);
    return result;
}

bool rppicomidi::Rp2040_i2c_bus::deinit_i2c_bus(RP2040_i2c_device* dev)
{
    bool result = false;
    critical_section_enter_blocking(&crit_sec);
    if (is_active_device(dev)) {
        result = true;
        gpio_deinit(sda_pin);
        gpio_deinit(scl_pin);
        if (i2c_bus == i2c0) {
            irq_set_enabled(I2C0_IRQ, false);
            irq_remove_handler(I2C0_IRQ, i2c0_irq_handler);
        }
        else {
            assert(i2c_bus == i2c1);
            irq_set_enabled(I2C1_IRQ, false);
            irq_remove_handler(I2C1_IRQ, i2c1_irq_handler);
        }
        i2c_deinit(i2c_bus);
    }
    critical_section_exit(&crit_sec);
    return result;
}

void rppicomidi::Rp2040_i2c_bus::init_bus()
{
    i2c_init(i2c_bus, baudrate);
    set_bus_pins(sda_pin, scl_pin);
    i2c_bus->hw->intr_mask = 0; // disable all I2C interrupts
    if (i2c_bus == i2c0) {
        i2c0_irq_context = this;
        irq_add_shared_handler(I2C0_IRQ, i2c0_irq_handler, PICO_DEFAULT_IRQ_PRIORITY);
        irq_set_enabled(I2C0_IRQ, true);
    }
    else {
        assert(i2c_bus == i2c1);
        i2c1_irq_context = this;
        irq_add_shared_handler(I2C1_IRQ, i2c1_irq_handler, PICO_DEFAULT_IRQ_PRIORITY);
        irq_set_enabled(I2C1_IRQ, true);
    }
}

bool rppicomidi::Rp2040_i2c_bus::reinit_i2c_bus(RP2040_i2c_device* dev)
{
    bool result = false;
    critical_section_enter_blocking(&crit_sec);
    if (is_active_device(dev)) {
        result = true;
        init_bus();
        i2c_bus->hw->enable = 0;
        i2c_bus->hw->tar = dev->get_addr();
        i2c_bus->hw->enable = 1;
    }
    critical_section_exit(&crit_sec);
    return result;
}

void rppicomidi::Rp2040_i2c_bus::i2c_irq_handler()
{
    critical_section_enter_blocking(&crit_sec);
    static io_ro_32 mask = (I2C_IC_INTR_STAT_R_TX_EMPTY_BITS | I2C_IC_INTR_STAT_R_STOP_DET_BITS | I2C_IC_INTR_STAT_R_RX_FULL_BITS);
    if ((i2c_bus->hw->intr_stat & mask) != 0) {
        // Disable the TX_EMPTY IRQ if currently active
        if ((i2c_bus->hw->intr_stat & I2C_IC_INTR_STAT_R_TX_EMPTY_BITS) != 0) {
            i2c_bus->hw->intr_mask &= ~I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
        }
        // Clear the stop detected IRQ if currently active // TODO is this necessary?
        if ((i2c_bus->hw->intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS) == 0) {
            io_ro_32 dummy = i2c_bus->hw->clr_stop_det;
            (void)dummy;
        }

        // Copy the data to the buffer if a read transaction (should clear the RX_FULL interrupt)
        if (current_transfer.is_read) {
            uint8_t* ptr = current_transfer.buffer+current_transfer.bytes_xferred;
            while (current_transfer.bytes_xferred < current_transfer.buffer_size && i2c_bus->hw->rxflr > 0) {
                *ptr++ = i2c_bus->hw->data_cmd & 0xff;
                current_transfer.bytes_xferred++;
            }
            // If not done reading all data, start a new read transfer
            if (current_transfer.bytes_xferred < current_transfer.buffer_size) {
                uint8_t remaining = current_transfer.buffer_size - current_transfer.bytes_xferred;
                i2c_bus->hw->rx_tl = (remaining > 16) ? 15 : remaining - 1;
                for (uint8_t idx = 0; idx < i2c_bus->hw->rx_tl; idx++) {
                    i2c_bus->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS;
                }
                if (current_transfer.buffer_size == (current_transfer.bytes_xferred + i2c_bus->hw->rx_tl + 1)) {
                    i2c_bus->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS;
                }
                else {
                    i2c_bus->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS;
                }
                critical_section_exit(&crit_sec);
                return;
            }
        }
        // Call the callback function if available.
        if (current_transfer.callback != nullptr) {
            current_transfer.callback(current_transfer.dev);
            assert(i2c_bus->hw->txflr == 0); // it had better be; otherwise, the subsequent pending transaction callback just got called. TODO: does current_transfer need to be a queue too?
            current_transfer.callback = nullptr;
        }
    }
    critical_section_exit(&crit_sec);
}

int rppicomidi::Rp2040_i2c_bus::request_bus(RP2040_i2c_device* requesting_device, void (*ready_callback)(RP2040_i2c_device*))
{
    int result = -1;
    if (requesting_device) {
        critical_section_enter_blocking(&crit_sec);
        for (auto it=requesting_devices.begin(); it != requesting_devices.end(); it++) {
            if (it->dev == requesting_device) {
                // It's in the list. Is the active device if at the front of the list.
                result =  is_active_device(it->dev) ? 1:0;
                break;
            }
        }
        if (result == -1) {
            // dev was not found in the list. Add it to the end
            requesting_devices.push_back({requesting_device, ready_callback});
            result =  is_active_device(requesting_device) ? 1:0;
        }
        critical_section_exit(&crit_sec);
    }
    // If the list was previously empty, then the device is now active; otherwise, it's in queue;
    if (result == 1) {
        // Assign the target address
        i2c_bus->hw->enable = 0;
        i2c_bus->hw->tar = requesting_device->get_addr();
        i2c_bus->hw->enable = 1;
    }
    return result;
}

int rppicomidi::Rp2040_i2c_bus::release_bus(RP2040_i2c_device* requesting_device)
{
    int result = -1;
    critical_section_enter_blocking(&crit_sec);
    for (auto it=requesting_devices.begin(); it!=requesting_devices.end();) {
        if (it->dev == requesting_device) {
            if (is_active_device(requesting_device)) {
                // This is the active device. Are there any active transfers?
                if ((i2c_bus->hw->status & I2C_IC_STATUS_ACTIVITY_BITS) != 0) {
                    result = 0; // bus transaction is ongoing. Need to wait until it is done
                }
                else {
                    current_transfer.reset();
                }
            }
            if (result != 0) {
                result = 1;
                requesting_devices.erase(it);
                if (requesting_devices.begin() != requesting_devices.end()) {
                    // The list is not empty; signal to the new head of the list it is now active
                    // but first, assign the device's address as the new target address.
                    i2c_bus->hw->enable = 0;
                    i2c_bus->hw->tar = requesting_devices.begin()->dev->get_addr();
                    i2c_bus->hw->enable = 1;
                    requesting_devices.begin()->callback(requesting_devices.begin()->dev);
                }
            }
            break;
        }
        else {
            ++it;
        }
    }
    critical_section_exit(&crit_sec);
    return result;
}

bool rppicomidi::Rp2040_i2c_bus::write(RP2040_i2c_device* dev, bool send_restart, bool send_stop, uint8_t* data, const uint8_t nbytes, void (*done_callback)(RP2040_i2c_device*))
{
    bool result = false;
    critical_section_enter_blocking(&crit_sec);
    if ((nbytes <= (16-i2c_bus->hw->txflr)) && is_active_device(dev)) {
        current_transfer.callback = done_callback;
        current_transfer.dev = dev;
        current_transfer.is_read = false;
        current_transfer.send_stop = send_stop; // TODO: do I need to copy this?
        io_rw_32 next_data_cmd = data[0];
        if (send_restart) {
            next_data_cmd |= I2C_IC_DATA_CMD_RESTART_BITS;
        }
        if (nbytes == 1 && send_stop) {
            next_data_cmd |= I2C_IC_DATA_CMD_STOP_BITS;
        }
        i2c_bus->hw->data_cmd = next_data_cmd;

        for (uint8_t idx= 1; idx < nbytes; idx++) {
            next_data_cmd = data[idx];
            if (idx == (nbytes - 1) && send_stop) {
                next_data_cmd |= I2C_IC_DATA_CMD_STOP_BITS;
            }
            i2c_bus->hw->data_cmd = next_data_cmd;
        }
        // enable TX FIFO empty IRQ
        i2c_bus->hw->intr_mask |= I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
        result = true;
    }
    critical_section_exit(&crit_sec);
    return result;
}

bool rppicomidi::Rp2040_i2c_bus::read(RP2040_i2c_device* dev, bool send_restart, bool send_stop, uint8_t* data, const uint8_t nbytes, void (*done_callback)(RP2040_i2c_device*))
{
    bool result = false;
    critical_section_enter_blocking(&crit_sec);
    if ((nbytes > 0) && is_active_device(dev) && (current_transfer.callback == nullptr)) {
        current_transfer.callback = done_callback;
        current_transfer.dev = dev;
        current_transfer.is_read = true;
        current_transfer.send_stop = send_stop;
        current_transfer.buffer = data;
        current_transfer.buffer_size = nbytes;
        current_transfer.bytes_xferred = 0;
        i2c_bus->hw->rx_tl = nbytes > 16 ? 15 : nbytes-1;  // will trigger the RX interrupt when all nbytes are read
        io_rw_32 next_data_cmd = I2C_IC_DATA_CMD_CMD_BITS;
        if (send_restart) {
            next_data_cmd |= I2C_IC_DATA_CMD_RESTART_BITS;
        }
        if (nbytes == 1 && send_stop) {
            next_data_cmd |= I2C_IC_DATA_CMD_STOP_BITS;
        }
        i2c_bus->hw->data_cmd = next_data_cmd;
        for (uint8_t idx= 1; idx <= i2c_bus->hw->rx_tl; idx++) {
            next_data_cmd = I2C_IC_DATA_CMD_CMD_BITS;
            if (idx == (nbytes - 1) && send_stop) {
                next_data_cmd |= I2C_IC_DATA_CMD_STOP_BITS;
            }
            i2c_bus->hw->data_cmd = next_data_cmd;
        }
        // enable RX FIFO FULL IRQ
        i2c_bus->hw->intr_mask |= I2C_IC_INTR_MASK_M_RX_FULL_BITS;
        result = true;
    }
    critical_section_exit(&crit_sec);
    return result;
}

bool rppicomidi::Rp2040_i2c_bus::set_general_call_mode(RP2040_i2c_device* dev, bool general_call_mode_active)
{
    if (!is_active_device(dev) || (i2c_bus->hw->status & I2C_IC_STATUS_ACTIVITY_BITS) != 0)
        return false;
    i2c_bus->hw->enable = 0;
    if (general_call_mode_active)
        hw_set_bits(&i2c_bus->hw->tar, I2C_IC_TAR_SPECIAL_BITS);
    else
        hw_clear_bits(&i2c_bus->hw->tar, I2C_IC_TAR_SPECIAL_BITS);
    i2c_bus->hw->enable = 1;
    return true;
}

int rppicomidi::Rp2040_i2c_bus::is_general_call_mode(RP2040_i2c_device* dev)
{
    return (i2c_bus->hw->tar & I2C_IC_TAR_SPECIAL_BITS) != 0;
}
