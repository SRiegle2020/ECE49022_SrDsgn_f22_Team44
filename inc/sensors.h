/*****************************************************************************
 * This header file gives a list of the sensor functions written for getting *
 * data in the Sensors-to-MCU subsystem.
 *****************************************************************************/
#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"

void pulseox_write(uint8_t reg, uint8_t val);
uint8_t pulseox_simple_read(uint8_t reg);
void pulseox_read_array(uint8_t loc, char data[], uint8_t len);
void pulseox_setup(void);
void pulseox_check(void);
int get_spo2(void);
void accelerometer_write(uint8_t reg, uint8_t val); //Write to the Accelerometer
void init_accelerometer(void);                      //Configure the Accelerometer
uint8_t accelerometer_read(uint8_t reg);            //Read from the Accelerometer
short accelerometer_X(void);                     //Get X Acceleration
short accelerometer_Y(void);                     //Get Y Acceleration
short accelerometer_Z(void);                     //Get Z Acceleration
