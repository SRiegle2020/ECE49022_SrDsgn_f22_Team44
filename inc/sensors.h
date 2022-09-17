/*****************************************************************************
 * This header file gives a list of the sensor functions written for getting *
 * data in the Sensors-to-MCU subsystem.
 *****************************************************************************/
#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"

void accelerometer_write(uint8_t reg, uint8_t val); //Write to the Accelerometer
void init_accelerometer(void);                      //Configure the Accelerometer
uint8_t accelerometer_read(uint8_t reg);            //Read from the Accelerometer
int accelerometer_X(void);                     //Get X Acceleration
int accelerometer_Y(void);                     //Get Y Acceleration
int accelerometer_Z(void);                     //Get Z Acceleration
