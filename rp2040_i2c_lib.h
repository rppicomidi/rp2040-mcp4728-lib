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
 * This library contains the classes you need to implement I2C device
 * classes on a shared RP2040 I2C bus. The RP2040 is the I2C bus master.
 * The bus may be I2C0 or I2C1.
 *
 * Calls to this library are non-blocking and interrupt driven. Reads
 * and writes make use of the I2C hardware's built-in TX FIFO and RX FIFO
 * so the processor does not need to service every interrupt. Write transactions
 * may be stacked as long as the TX FIFO would not overflow. Read transactions
 * have to be one at a time. Bus sharing uses cooperative bus request and release.
 * If your I2C bus has more than one device on it, each device will need
 * to request the bus for a transaction or set of transactions and release the
 * bus when done. Device functions should be robust to failing to request the bus.
 */
#pragma once
#include <list>
#include <queue>
#include <cstdint>
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/assert.h"
#include "pico/timeout_helper.h"
#include "pico/critical_section.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
namespace rppicomidi
{
class Rp2040_i2c_bus;
/**
 * This class represents a device with a particular address on the I2C bus.
 * Use it as a base class for your real devices
 */
class RP2040_i2c_device
{
public:
    RP2040_i2c_device(uint16_t addr_, Rp2040_i2c_bus* bus_) : addr{addr_}, bus{bus_} {}
    uint16_t get_addr() const {
        return addr;
    }
protected:
    uint16_t addr;      // The I2C address of the device; 10-bit addresses must have upper 5 MSBs 0b11110
    Rp2040_i2c_bus* bus;
    void* context; // the context for the currently pending callback
    void (*callback)(void* context); // the currently pending callback
    static void dev_cb(RP2040_i2c_device* dev) {
        if (dev->callback)
            dev->callback(dev->context);
        dev->context = nullptr;
        dev->callback = nullptr;
    }
private:
    RP2040_i2c_device()=delete;
    RP2040_i2c_device(const RP2040_i2c_device&)=delete;
    RP2040_i2c_device& operator=(const RP2040_i2c_device&)=delete;
};

/**
 * This class represents a physical I2C bus (either I2C 0 or I2C 1). I2C bus transactions
 * are interrupt driven. The functions in this class are not thread safe.
 */
class Rp2040_i2c_bus
{
public:
    Rp2040_i2c_bus(i2c_inst_t* i2c_ , uint baudrate_, uint sda_pin_, uint scl_pin_);

    void enter_critical() {critical_section_enter_blocking(&crit_sec);}
    void exit_critical() {critical_section_exit(&crit_sec); }
    /**
     * @brief request access to the I2C bus
     * 
     * @return 1 if the requesting device is now the active device
     * @return 0 if the requesting device activation is deferred; callback will be called when the device is active.
     * @return -1 if the parameters are invalid.
     * @param requesting_device is the device that wants to become active
     * @param ready_callback is called when the device becomes active after deferral. This function will be called
     * from the IRQ context of the interrupted core in a multi-core critical section. Do not try to start a new
     * transaction from the callback. Signal a non-interrupt context non-critical section routine to do it instead.
     */
    int request_bus(RP2040_i2c_device* requesting_device, void (*ready_callback)(RP2040_i2c_device*));

    /**
     * @brief release the bus if the current device is the requesting device. If the requesting
     * device is not current but in the queue, remove it from the queue.
     *
     * @return 1 if the requesting_device was active or in the queue and was removed
     * @return 0 if the requesting_device is in the middle of an active; need to wait and call it again
     * @return -1 if the requesting_device was not active in the first place
     * @param requesting_device is the device that wants to become inactive
     */
    int release_bus(RP2040_i2c_device* requesting_device);

    /**
     * @brief Write nbytes of data to the last device that successfully request the bus;
     * call done_callback() when done.
     *
     * As long as at least nbytes entries remaining in the TX FIFO,
     * perform the write. The write will start with a restart condition if send_restart is set, and
     * will end with a stop condition if send_stop is set. The write will start with a start condition
     * if the previous bus condition was stoppped. When the TX FIFO is empty or a stop condition
     * is detected, call the done_callback().
     *
     * @return true if successful or false if the TX FIFO does not have enough space to hold nbytes or the bus was not successfully requested
     * @param dev the RP2040_i2c_device that has bus access; must be the same device that successfully requested the bus
     * @param send_restart is true if the first byte of the buffer is preceeded by a restart condition
     * @param send_stop is true if the last byte of the buffer is followed by a stop condition
     * @param data is a pointer to the data buffer.
     * @param done_callback is called if TX FIFO is empty. If the caller does not
     * need to do anything when transmission is complete, you may omit this argument. This function will be called
     * from the IRQ context of the interrupted core in a multi-core critical section. If used, do not start a new
     * write or read until the done_callback is called because the most recent done_callback will overwrite the
     * previous one. Also do not try to start a new transaction from the callback. Signal a non-interrupt context
     * non-critical section routine to do it instead.
     */
    bool write(RP2040_i2c_device* dev, bool send_restart, bool send_stop, uint8_t* data, const uint8_t nbytes, void (*done_callback)(RP2040_i2c_device*)=nullptr);

    /**
     * @brief enter or exit the I2C bus master general call mode
     *
     * @return true if successful or false if the bus was not successfully requested or if a
     * transaction is currently ongoing.
     * @param dev the RP2040_i2c_device that has bus access; must be the same device that successfully requested the bus
     * @param general_call_mode_active is true to activate the general call mode; is false to exit general call mode
     * @note It is true that the purpose of general call mode is to work with all devices,
     * so a single device having access to the bus does not really make sense. However,
     * the framework assumes no transactions can occur without a device having I2C access.
     * If this presents a problem in your system, create an all-call RP2040_i2c_device
     * object, have it obtain bus access, and use that for this function.
     */
    bool set_general_call_mode(RP2040_i2c_device* dev, bool general_call_mode_active);

    /**
     * @brief
     *
     * @return 1 if the I2C bus that dev has access to is in general call mode; 0 if
     * the I2C bus is not in general call mode. -1 if dev does not have access to the
     * I2C bus.
     * @param dev the RP2040_i2c_device that has bus access; must be the same device
     * that successfully requested the bus
     * @note It is true that the purpose of general call mode is to work with all devices,
     * so a single device having access to the bus does not really make sense. However,
     * the framework assumes no transactions can occur without a device having I2C access.
     * If this presents a problem in your system, create an all-call RP2040_i2c_device
     * object, have it obtain bus access, and use that for this function.
     */
    int is_general_call_mode(RP2040_i2c_device* dev);

    /**
     * @brief Read nbytes of data and call done_callback() when done
     *
     * As long as at least nbytes entries remaining in the TX FIFO and the RX FIFO,
     * perform the read. The read will start with a restart condition if send_restart is set, and
     * will end with a stop condition if send_stop is set. The read will start with a start condition
     * if the previous bus condition was stoppped. When the TX FIFO is empty or the RX FIFO
     * contains nbytes of data or a stop condition has been detected, copy the data from the RX FIFO to the data buffer and
     * call the done_callback().
     *
     * @note the TX FIFO is used to drive receive operation. That is why TX FIFO status drives this function.
     * @return true if successful or false if the bus is busy or the bus was not successfully requested or if
     * the I2C bus is in general call mode
     * @param dev the device that has bus access; must be the same device that successfully requested the bus.
     * @param send_restart is true if the first byte of the buffer is preceeded by a restart condition
     * @param send_stop is true if the last byte of the buffer is followed by a stop condition
     * @param data is a pointer to the data buffer that will receive the bytes read from I2C.
     * @param done_callback is called if TX FIFO is empty or a stop condition is detected. It is not recommended
     * to make this function pointer nullptr. This function will be called from the IRQ context of the interrupted core.
     * Do not start a new write or read until the done_callback is called.
     */
    bool read(RP2040_i2c_device* dev, bool send_restart, bool send_stop, uint8_t* data, const uint8_t nbytes, void (*done_callback)(RP2040_i2c_device* dev));

    /**
     * @brief Change the I2C pins to the RP2040 GPIO numbers
     *
     * @return true if successful, false if a transfer is in progress or dev is not the same as the current bus owner.
     * @param dev the device that owns the bus; must be the same device that successfully requested the bus
     * @param sda_pin_ the GPIO number of the SDA pin
     * @param scl_pin_ the GPIO number of the SCL pin
     */
    bool set_bus_pins(RP2040_i2c_device* dev, uint sda_pin_, uint scl_pin_);

    /**
     * @brief get the GPIO pins for the I2C bus. It is not necessary to have successfully requested the bus to do this
     *
     * @param sda_pin_ the GPIO number of the SDA pin
     * @param scl_pin_ the GPIO number of the SCL pin
     */
    void get_bus_pins(uint& sda_pin_, uint& scl_pin_) {sda_pin_ = sda_pin; scl_pin_ = scl_pin; }

    /**
     * @brief
     *
     * @return true if dev has the I2C bus.
     * @param dev is the I2C device that is currently communicating on this bus.
     * @note call this in a critical section.
     */
    bool is_active_device(RP2040_i2c_device* dev) const {return dev == requesting_devices.begin()->dev; }

    /**
     * @brief deactivate the on-chip I2C and associated hardware
     *
     * @return true if dev currently has I2C bus access; false otherwise
     * The main reason to call this function is to substitute a bit-bang
     * or PIO-based I2C support for the on-chip I2C.
     * @param dev is the I2C device that is currently communicating on this bus.
     */
    bool deinit_i2c_bus(RP2040_i2c_device* dev);

    /**
     * @brief reverse the effects of the deinit_i2c_bus() function and
     * restore on-chip I2C hardware support.
     *
     * @return true if dev currently has I2C bus access; false otherwise
     * @param dev is the I2C device that is currently communicating on this bus.
     */
    bool reinit_i2c_bus(RP2040_i2c_device* dev);
protected:
    static Rp2040_i2c_bus* i2c0_irq_context;
    static Rp2040_i2c_bus* i2c1_irq_context;
    static void i2c0_irq_handler(void) {
        i2c0_irq_context->i2c_irq_handler();
    }
    static void i2c1_irq_handler(void) {
        i2c1_irq_context->i2c_irq_handler();
    }
    void i2c_irq_handler();
    void set_bus_pins(uint sda_pin_, uint scl_pin_);
    void init_bus();
    struct I2c_dev_cb
    {
        RP2040_i2c_device* dev;
        void (*callback)(RP2040_i2c_device*);
    };
    struct I2c_dev_in_progress : public I2c_dev_cb
    {
        void reset() {
            buffer = nullptr; buffer_size=0; is_read=false; send_restart=false; send_stop=false; dev = nullptr; callback = nullptr;
        }
        uint8_t* buffer;
        uint8_t buffer_size;
        uint8_t bytes_xferred;
        bool is_read;
        bool send_restart;
        bool send_stop;
    };
    i2c_inst_t* i2c_bus;
    uint baudrate;
    uint sda_pin;
    uint scl_pin;
    critical_section_t crit_sec;
    std::list<I2c_dev_cb> requesting_devices; // The front of the queue is the current device
    I2c_dev_in_progress current_transfer; // if there is no current transfer, then buffer will be NULL, buffer_size will be 0, send_restart will be false and send_stop will be false
private:
    Rp2040_i2c_bus()=delete;
    Rp2040_i2c_bus(const Rp2040_i2c_bus&)=delete;
    Rp2040_i2c_bus& operator=(const Rp2040_i2c_bus)=delete;
};
}