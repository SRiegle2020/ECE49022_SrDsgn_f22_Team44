/*****************************************************************************
 * This header file gives a list of the I2C helper functions utilized in the *
 * Sensors-to-MCU subsystem and Watch and Accelerometer Data subsystem.      *
 *****************************************************************************/
#include "stm32f0xx.h"

void init_i2c(void);     //Helper to Initialize I2C and Configure PB6,7
void i2c_waitidle(void); //Wait for I2C to not be busy
void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir);    //Generate Start
void i2c_stop(void);        //Generate Stop
int i2c_checknack(void);    //Check for NACK
void i2c_clearnack(void);   //Clear the NACK
int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size); //Send Data
int i2c_recvdata_P(uint8_t devaddr, void *data, uint8_t size);     //Rx Data (P)
int i2c_recvdata_noP(uint8_t devaddr, void *data, uint8_t size);   //Rx Data
