#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"
#include "sensors.h"
#include "uart.h"

//============================================================================
// DEFINITIONS FOR DEVICE ADDRESSES
//  * This just includes the definitions for the device addresses.
//============================================================================
#define ACCELEROMETER_ADDR 0x68 //Since MPU6050 has the same address as the PCF8523, we have to set the A0 pin on the MPU6050 HIGH so both can be on the same I2C bus

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
    i2c_recvdata_noP(ACCELEROMETER_ADDR, accelerometer_data, 1);
    return I2C1->RXDR & 0xff;
}

//============================================================================
// INIT_ACCELEROMETER
//  * Configures the accelerometer
//      (1) Reset
//      (2) Set to +-8g sensitivity. This was chosen since the algorithms used
//          by Kim Dongwoo and Hee Chan Kim [Energy Expenditure, IEEE] and by
//          by Malmo University [Step Counter] were using +-6g and +-8g
//          accelerometers, respectively.
//      (3) Set to a 20Hz DLPF. This is because the same algorithms implement
//          a 5Hz [Step Counter, Malmo] and 20Hz [Energy Expenditure, IEEE]
//          LPF for calculations.
//============================================================================
void init_accelerometer(void) {
    accelerometer_write(0x6b,0x00); //Reset
    accelerometer_write(0x1c,0x10); //+-8g sensitivity
    accelerometer_write(0x1a,0x06); //Set to 20Hz DLPF
}

//============================================================================
// ACCELEROMETER_X
//  * Read X-axis data from the accelerometer
//============================================================================
int accelerometer_X(void) {
    return ((accelerometer_read(0x3b) << 8) | accelerometer_read(0x3c));
}

//============================================================================
// ACCELEROMETER_Y
//  * Read Y-axis data from the accelerometer
//  * NOTE: This reads a default of 1g
//============================================================================
int accelerometer_Y(void) {
    return ((accelerometer_read(0x3d) << 8) | accelerometer_read(0x3e));
}

//============================================================================
// ACCELEROMETER_Z
//  * Read Z-axis data from the accelerometer
//============================================================================
int accelerometer_Z(void) {
    return ((accelerometer_read(0x3f) << 8) | accelerometer_read(0x40));
}
