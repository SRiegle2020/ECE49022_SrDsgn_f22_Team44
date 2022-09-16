#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"
#include "sensors.h"

//============================================================================
// DEFINITIONS FOR DEVICE ADDRESSES
//  * This just includes the definitions for the device addresses.
//============================================================================
#define ACCELEROMETER_ADDR 0x68 //Default I2C device address

//============================================================================
// ACCELEROMETER_WRITE
//  * Send data to the accelerometer
//  * Used mainly for configuration
//============================================================================
void accelerometer_write(uint8_t reg, uint8_t val) {
    uint8_t accelerometer_writedata[2] = {reg, val};
    i2c_senddata(ACCELEROMETER_ADDR, accelerometer_writedata,2);
}

//============================================================================
// ACCELEROMETER_READ
//  * Read data from the accelerometer
//============================================================================
uint8_t accelerometer_read(uint8_t reg) {
    uint8_t accelerometer_data[1] = {reg};
    //i2c_senddata(ACCELEROMETER_ADDR, accelerometer_data, 1);
    i2c_recvdata(ACCELEROMETER_ADDR, accelerometer_data, 1);
    return I2C1->RXDR & 0xff;
}
