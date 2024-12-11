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
#pragma once
#include "rp2040_i2c_lib.h"
namespace rppicomidi {
struct mcp4728_channel_data
{
    uint8_t chan;       // the DAC channel 0-3=>A-D
    uint8_t udac;       // 0 to update DAC channel after input register is written; 1 updates the input register only
    uint8_t vref;       // 0 Vref=VDD. 1 Vref=2.048V
    uint8_t pd;         // 0b00 Power On. 0b01 Vout loaded 1K to ground. 0b10 Vout loaded 100K to ground. 0b10 Vout loaded 500k to ground
    uint8_t gain;       // 0 gain=1. 1 gain=2.
    uint16_t dac_code;  // 12-bit DAC output value 0-4095
};

struct mcp4728_channel_read_data : public mcp4728_channel_data
{
    bool is_eeprom;
    bool rdy;           // is true if the DAC is ready to receive more serial commands; is busy otherwise and ignores writes
    bool por;           // is true if the Vdd > Vpor so the DAC is powered on; false if powered off
};

class RP2040_MCP4728 : public RP2040_i2c_device
{
public:
    static const uint no_ldac_gpio=0xFFFF;
    /**
     * @brief constructor
     *
     * @param addr_ the 7-bit device address of the MCP4728 device (usually 0x60)
     * @param bus_ a pointer to the Rp2040_i2c_bus to which this MCP4728 device is attached
     * @param ldac_ is the GPIO number of the LDAC pin or no_ldac_gpio if LDAC is not connected.
     * @param ldac_invert_ is true if the LDAC\ GPIO control has an external inverting buffer.
     */
    RP2040_MCP4728(uint16_t addr_, Rp2040_i2c_bus* bus_, uint ldac_=no_ldac_gpio, bool ldac_invert_=false);

    /**
     * poll the status of pending operations and call callback functions if needed
     */
    void task();

    /**
     * @brief request to make this device the active device on the I2C bus
     *
     * @return 1 if this device is now active on the I2C bus.
     * @return 0 if access is deferred (callback will be called when device is active)
     * @return -1 if parameters are invalid
     * @param callback pointer to function that is called when deferred bus is available
     * @param context is the context parameter for the callback function
     */
    int request_bus(void (*callback)(void* context), void* context);

    /**
     * @brief request to allow another device to be the active device on this bus
     *
     * @param callback pointer to function that is called when deferred bus release completes
     * @param context is the context parameter for the callback function
     * @return 1 if this device was active or in the queue and was removed. Callback will not be called.
     * @return 0 if the this device is in the middle of an active transfer;
     * callback will be called when transfer is done
     * @return -1 if the this device was not active in the first place
     */
    int release_bus(void (*callback)(void* context), void* context);


    /**
     * @brief fast write to MCP4728 channels.
     *
     * This routine writes chan_dat to each DAC channel in order. It always starts will channel A.
     * It does not update the EEPROM. Set up the gain and reference for each of the 4 channels
     * using the multi_write() function, then use this function to update the DAC output and
     * power state for all 4 channels (for example, for waveform generation). The udac bit is not
     * available in this command. Output update is controlled by the state of the LDAC\ pin. If
     * LDAC\ is low when the DAC acknowledges a write to a channel, that channel's output updates.
     * If LDAC\ makes a high to low transition, all 4 outputs update. The update_all_channels()
     * function will also cause the DAC outputs to update. For example, a 4-channel waveform
     * generator might use this command to update all 4 DAC input registers and then use
     * the LDAC\ pin toggle to update the outputs to more precisely control the sample rate.
     * @return true if successful, false if issues accessing the I2C bus or nchan > 4
     * @param chan_dat is an array of 12-bit DAC channel outputs with one of the power-down code values in bits 13:12.
     * If all outputs are powered on, it is just an array of 12-bit DAC channel codes, one per channel.
     * @param nchan the number of channels to write. It cannot be > 4.
     * @param stop is true to send I2C stop after all nchan chan_dat items are written. If this
     * DAC is the only I2C device on the bus, once the DAC channels are set up, you can repeatedly
     * call this fuction to update all 4 channels. Setting stop to false will save one byte of I2C
     * transfer for each function call. (optional)
     * @param callback is the function called when write completes (optional)
     * @param context is the context paramter passed to the callback function (optional)

     */
    bool fast_write(const uint16_t* chan_dat, uint8_t nchan, bool stop=true, void (*callback)(void* context)=nullptr, void* context=nullptr);

    /**
     * @brief write nchan of channel data to the DAC (no EEPROM update); the channels
     * written are specified in the chan_dat array; channel order is arbitrary;
     * nchan no greater than 4.
     *
     * @return true if successful, false if issues accessing the I2C bus or nchan > 4
     * @param chan_dat the array of DAC values to write (MCP4728)
     * @param nchan the number of channels (1-4).
     * @param callback is the function called when write completes (optional)
     * @param context is the context paramter passed to the callback function (optional)
     */
    bool multi_write(const mcp4728_channel_data* chan_dat, uint8_t nchan, void (*callback)(void* context)=nullptr, void* context=nullptr);

    /**
     * @brief Write to DAC outputs and EEPROM. If nchan == 1, write to one of
     * channel A, B, C, or D. If nchan > 1, write to sequential channels C-D, or 
     * channels B-D, or channels A-D.
     * 
     * @return true if successful or false if the DAC does not have I2C bus access or
     * if nchan > 4
     * @param chan_dat is the DAC channel value to write. For nchan == 1, the channel
     * number is specified in chan_dat. For nchan > 1, the chan field is ignored;
     * the last channel in the chan_dat array is always D, the first channel
     * is 'D'-(nchan-1), and the other channels are sequential between the first and D.
     * The udac field of the first channel in the array determines udac for all channels.
     * @param nchan is the number of channels to write.
     * @param callback is the function called when write completes (optional)
     * @param context is the context paramter passed to the callback function
     * @note after transfer completes and EEPROM write starts, further writes will be ignored
     * until the RDY/BSY flag clears (call poll_status() or monitor the RDY/BSY\ pin)
     */
    bool sequential_write_eeprom(const mcp4728_channel_data* chan_dat, uint8_t nchan, void (*callback)(void* context)=nullptr, void* context=nullptr);

    /**
     * @brief read out nchan DAC register values alternating between DAC output values and EEPROM values
     *
     * @return true if successful or false if the DAC does not have I2C bus access or nchan > 8
     * @param chan_dat is an array starting with channel A, DAC output then
     * EEPROM paramters, then repeat up to nchan times, for channels B, C and D. The chan_dat
     * pointer must point to valid data at least until read completes.
     * @param nchan is the number of elements in the chan_dat array to read. If nchan is odd, then
     * the last channel's EEPROM data is not read.
     * @param callback is the function to call when all nchan channels are read
     * @param context is the context parameter to use for the callback function
     * @note although it is possible to set the callback function pointer to nullptr, it is
     * not recommended. It is the only way to know if the chan_dat array contains the read
     * data or not.
     */
    bool read_channels(mcp4728_channel_read_data* chan_dat, uint8_t nchan, void (*callback)(void* context), void* context);

    /**
     * @brief read the DAC A register to discover MCP4728 status and call a callback with the status information
     *
     * @return true if successful or false if the DAC does not have I2C bus access
     * @param callback is the function to call when DAC A register read completes. The context parameter
     * is usually the this pointer of the calling class. The is_busy parameter
     * is true if the MCP4728 is still writing to EEPROM. The is_powered_on parameter is true if the
     * Vdd > Vpor.
     * @param context is the context parameter to use for the callback function
     */
    bool poll_status(void (*callback)(void* context, bool is_busy, bool is_powered_on), void* context);

    /**
     * @brief set the gain values for all channels
     *
     * @return true if successful or false if the DAC does not have I2C bus access
     * @param gainA is true for channel A gain=2, false for gain=1
     * @param gainB is true for channel B gain=2, false for gain=1
     * @param gainC is true for channel C gain=2, false for gain=1
     * @param gainD is true for channel D gain=2, false for gain=1
     */
    bool set_all_gains(bool gainA, bool gainB, bool gainC, bool gainD, void (*callback)(void* context), void* context);

    /**
     * @brief set the Vref values for all channels
     *
     * @return true if successful or false if the DAC does not have I2C bus access
     * @param vrefA is true for channel A Vref=2.048V, false for Vref=Vdd
     * @param vrefB is true for channel B Vref=2.048V, false for Vref=Vdd
     * @param vrefC is true for channel C Vref=2.048V, false for Vref=Vdd
     * @param vrefD is true for channel D Vref=2.048V, false for Vref=Vdd
     */
    bool set_all_vrefs(bool gainA, bool gainB, bool gainC, bool gainD, void (*callback)(void* context), void* context);

    /**
     * @brief set the PD values for all channels
     *
     * @return true if successful or false if the DAC does not have I2C bus access
     * @param pdA is PD value 0-3 for channel A
     * @param pdB is PD value 0-3 for channel A
     * @param pdC is PD value 0-3 for channel A
     * @param pdD is true for channel D Vref=2.048V, false for Vref=Vdd
     */
    bool set_all_pds(uint8_t pdA, uint8_t pdB, uint8_t pdC, uint8_t pdD, void (*callback)(void* context), void* context);

    /**
     * @brief send the reset command to all devices on the same bus as this MCP4728
     *
     * @return true if successful or false if the DAC does not have I2C bus access
     */
    bool reset(void (*callback)(void* context), void* context);

    /**
     * @brief send the wakeup command to all devices on the same bus as this MCP4728
     *
     * @return true if successful or false if the DAC does not have I2C bus access
     */
    bool wakeup(void (*callback)(void* context), void* context);

    /**
     * @brief send the wake-up command to all devices on the same bus as this MCP4728
     */
    bool wake_up(void (*callback)(void* context), void* context);

    /**
     * @brief send the general call software update command to all devices on the same bus as this MCP4728
     */
    bool update_all_channels(void (*callback)(void* context), void* context);

    /**
     * @brief this bit bangs the MCP4728 I2C with LDAC protocol
     * for reading and writing the MCP4728's I2C address bits.
     *
     * This function blocks until it completes. If it returns true,
     * and new_addr is not 0, then 
     * @return true if successful, false if this device does not have access
     * to the I2C bus or new_address values are not valid.
     * @param new_addr Valid new_addr values are 0, or 0x60-0x67.
     * Use new_addr=0 to for general call read device address or
     * set new_addr to 0x60-0x67 to update the address bits to 0-7
     * @param read_addr is the value read back if new_addr is 0. Bits 7:5
     * are the address bits in the EEPROM. Bits 3:1 are the address bits
     * in the input register. Bit 4 is always 1 and bit 0 is always 0.
     * @note if ldac_gpio == no_ldac_gpio, then this function always
     * returns false;
     */
    bool access_addr_bits(uint8_t new_addr, uint8_t& read_addr);

    /**
     * @brief Set the ldac GPIO high or low
     * 
     * @param is_high is true to set the pin high, false to set it low.
     * @return true if successful, false if ldac_pin == no_ldac_gpio
     * @return false 
     */
    bool set_ldac_pin(bool is_high);
protected:
    static void req_bus_callback(RP2040_i2c_device* context);
    static void fast_write_callback(RP2040_i2c_device* context);
    static void multi_write_callback(RP2040_i2c_device* context);
    static void read_callback(RP2040_i2c_device* context);
    static void status_callback(RP2040_i2c_device* context);
    static void seq_write_eeprom_callback(RP2040_i2c_device* context);
    static void gain_set_callback(RP2040_i2c_device* context);
    static void pd_set_callback(RP2040_i2c_device* context);
    static void vref_set_callback(RP2040_i2c_device* context);
    static void reset_callback(RP2040_i2c_device* context);
    static void wakeup_callback(RP2040_i2c_device* context);
    static void update_callback(RP2040_i2c_device* context);
    void bytes2channel_read_data(uint8_t* bytes, mcp4728_channel_read_data* crd);

    /**
     * @brief bit bang 8 bits of data to the device and read back from the device and
     * set LDAC low on the 8th bit if change_ldac is true.
     *
     * @param write_byte is the data to write out. Set to 0xFF if reading back a byte
     * @param read_byte returns the write_byte for a write command and the value
     * read back for a read command if write_byte was set to 0xFF
     * @pram ignore_nak is true during byte reads
     * @param sda_pin is the GPIO number of the SDA pin
     * @param scl_pin is the GPIO number of the SCL pin
     * @param change_ldac is true if LDAC\ toggles low on the 8th bit
     */
    bool bit_bang_8_bits(uint8_t write_byte, uint8_t& read_byte, bool ignore_nak, uint sda_pin, uint scl_pin, bool change_ldac);

    struct app_callback {
        void (*callback)(void* context);
        void *context;
        volatile bool call_callback;
    };
    struct app_read_callback : public app_callback {
        mcp4728_channel_read_data* data;
        uint8_t nchan;
    };
    struct app_status_callback {
        void (*callback)(void* context, bool is_busy, bool is_powered_on);
        void *context;
        volatile bool call_callback;
    };
    struct {
        app_callback req_bus;
        app_callback rel_bus;
        app_callback fast_write;
        app_callback multi_write;
        app_read_callback read;
        app_status_callback status;
        app_callback seq_write_eeprom;
        app_callback set_gain;
        app_callback set_pd;
        app_callback set_vref;
        app_callback reset;
        app_callback wakeup;
        app_callback update;
    } app_callbacks;
    uint ldac_gpio;
    bool release_bus_pending;
    void check_callback(app_callback& app_cb);
    uint8_t read_data[24]; // 8 channels of 3 bytes
private:
    RP2040_MCP4728() = delete;
    RP2040_MCP4728(const RP2040_MCP4728&) = delete;
    RP2040_MCP4728& operator=(const RP2040_MCP4728&) = delete;
};
}
