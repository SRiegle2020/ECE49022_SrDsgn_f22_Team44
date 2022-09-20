/*****************************************************************************
 * This code contains functions for interfacing with I2C devices. This will  *
 * primarily be used by the Sensors-to-MCU and Watch and Accelerometer Data  *
 * subsystems.																 *
 *****************************************************************************/
#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"

//============================================================================
// INIT_I2C
//  * Configures PB6 to I2C1_SCL
//  * Configures PB7 to I2C1_SDA
//  * NOTE: configurations may need to be slightly tweaked based upon the
//  *       the different requirements of the different I2C sensors. Let us
//  *       hope not.
//============================================================================
void init_i2c(void) {
    //Clock GPIOB
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    //Configure PB6,7 for I2C
    GPIOB->MODER  &= ~0x0000f000; //To AltFn
    GPIOB->MODER  |=  0x0000a000;
    GPIOB->AFR[0] &= ~0xff000000; //AF1
    GPIOB->AFR[0] |=  0x11000000;

    //Enable and Configure I2C Controls
    RCC->APB1ENR  |=  RCC_APB1ENR_I2C1EN;   //Clock I2C
    I2C1->CR1     &= ~I2C_CR1_PE;           //Disable, the configure
    I2C1->CR1     &= ~I2C_CR1_ANFOFF;       //Turn on filter
    I2C1->CR1     &= ~I2C_CR1_ERRIE;        //Disable Error interrupt
    I2C1->CR1     &= ~I2C_CR1_NOSTRETCH;    //Enable clock stretching

    //Configure Timer for 100kHz (Too Fast at 400kHz)
    I2C1->TIMINGR  =  0;                    //Reset TIMINGR
    I2C1->TIMINGR &=  ~I2C_TIMINGR_PRESC;    //Clear PSC
    I2C1->TIMINGR |=  0 << 28;              //Set it to 0
    I2C1->TIMINGR |=  3 << 20;              //Set SCLDEL to 3
    I2C1->TIMINGR |=  1 << 16;              //Set SDADEL to 1
    I2C1->TIMINGR |=  3 << 8;               //Set SCLH to 3
    I2C1->TIMINGR |=  9 << 0;               //Set SCLL to 9

    //Disable both "OWN" addresses
    I2C1->OAR1 &= ~I2C_OAR1_OA1EN;
    I2C1->OAR2 &= ~I2C_OAR2_OA2EN;

    //=========================================================================
    // NOTE! May need further configuration for specific sensors
    //=========================================================================
    //Further configuration
    I2C1->CR2 &= ~I2C_CR2_ADD10;    //Set to 7-bit mode
    I2C1->CR2 |=  I2C_CR2_AUTOEND;  //Enable automatic ending
    I2C1->CR1 |=  I2C_CR1_PE;       //Enable Channel
}

//============================================================================
// I2C_WAITIDLE
//  * Helper for I2C, waits if I2C is busy
//============================================================================
void i2c_waitidle(void) {
    while((I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY);
}

//============================================================================
// I2C_START
//  * Helper for I2C, generates a start bit
//============================================================================
void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir) {
    uint32_t tmpreg = (I2C1->CR2) & ~(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP);

    if(dir == 1)
        tmpreg |=  I2C_CR2_RD_WRN; //Set to read
    else
        tmpreg &= ~I2C_CR2_RD_WRN; //Set to write
    tmpreg |= ((devaddr<<1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES) | I2C_CR2_START; //Write a START bit
    I2C1->CR2 = tmpreg;
}

//============================================================================
// I2C_STOP
//  * Helper for I2C, generates a stop if all data transferred
//============================================================================
void i2c_stop(void) {
    I2C1->CR2 |= I2C_CR2_STOP;               //Allow STOPF
    while((I2C1->ISR & I2C_ISR_STOPF) == 0); //Wait for STOPF
    I2C1->ICR |= I2C_ICR_STOPCF;             //Clear flag
}

//============================================================================
// I2C_CHECKNACK
//  * Helper for I2C, checks if not acknowledged (device is not connected)
//============================================================================
int i2c_checknack(void) {
    return(I2C1->ISR & I2C_ISR_NACKF); //Return 0 if NACK set, something else otherwise
}

//============================================================================
// I2C_CLEARNACK
//  * Clears the NACK flag if detected
//============================================================================
void i2c_clearnack(void) {
    I2C1->ICR |= I2C_ICR_NACKCF; //Clear NACKF
}

//============================================================================
// I2C_SENDDATA
//  * Send # bytes of data to the device
//  * The first byte is typically a 7bit register address followed by a 1bit
//    R/W indicator.
//  * The following bytes are the data to be sent to the chip.
//  * Used both for writing to registers and choosing to read or write
//============================================================================
int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size) {
    if(size <= 0 || data == 0)          //Check conditions
        return -1;
    uint8_t *udata = (uint8_t*)data;    //Make sure data is uint8_t
    i2c_waitidle();                     //Wait until ready
    i2c_start(devaddr, size, 0);        //Start bit

    for(int i = 0; i < size; i++) {     //Transmit the bits
        int count = 0;
        while((I2C1->ISR & I2C_ISR_TXIS) == 0) {
            count++;
            if(count > 1000000)
                return -1;
            if(i2c_checknack()) {       //If NACKF set by slave device
                i2c_clearnack();        //Clear, generate stop, return error
                i2c_stop();
                return -1;
            }
        }
        I2C1->TXDR = udata[i] & I2C_TXDR_TXDATA; //Send transmitted data
    }

    //Wait until all data is sent
    while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
    if((I2C1->ISR & I2C_ISR_NACKF) != 0) //Error checking
        return -1;
    i2c_stop();                          //If no errors, can final end happy
    return 0;
}

//============================================================================
// I2C_RECVDATA_P
//  * Read # bytes of data to the device
//  * NOTE that this version relies on the following format:
//    S DevAddrW Ack RegAddr Ack P S DevAddrR Ack Data Ack Data Nack P
//  * That is, there is a stop between the write/read sections. Some I2C
//  * sensors will use a different format:
//    S DevAddrW Ack RegAddr Ack S DevAddrR Ack Data Ack Data Nack P
//============================================================================
int i2c_recvdata_P(uint8_t devaddr, void *data, uint8_t size) {
    if(size <= 0 || data == 0)          //Check conditions
            return -1;
        uint8_t *udata = (uint8_t*)data;    //Make sure data is uint8_t
        i2c_waitidle();                     //Wait until ready
        i2c_start(devaddr, size, 1);        //Start bit

    for(int i = 0; i < size; i++) {     //Send each byte
        int count = 0;
        while((I2C1->ISR & I2C_ISR_RXNE) == 0) {
            count++;
            if(count > 1000000)         //Too long
                return -1;
            if(i2c_checknack()) {       //Ensure valid transmission/receive
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }
        udata[i] = I2C1->RXDR;          //Store received data
    }

    //Check once more for error, then return received data
    while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
    if(I2C1->ISR & I2C_ISR_NACKF)
        return -1;
    i2c_stop();
    return 0;
}

//============================================================================
// I2C_RECVDATA_NOP
//  * Read # bytes of data to the device
//  * NOTE that this version relies on the following format:
//    S DevAddrW Ack RegAddr Ack S DevAddrR Ack Data Ack Data Nack P
//  * That is, there is no stop between the write/read sections. Some I2C
//  * sensors will use a different format:
//    S DevAddrW Ack RegAddr Ack P S DevAddrR Ack Data Ack Data Nack P
//============================================================================
int i2c_recvdata_noP(uint8_t devaddr, void *data, uint8_t size) {
    if(size <= 0 || data == 0)          //Check conditions
        return -1;
    uint8_t *udata = (uint8_t*)data;    //Make sure data is uint8_t
    i2c_waitidle();                     //Wait until ready
    i2c_start(devaddr, size, 0);        //Start bit

    for(int i = 0; i < size; i++) {     //Transmit the bits
        int count = 0;
        while((I2C1->ISR & I2C_ISR_TXIS) == 0) {
            count++;
            if(count > 1000000)
                return -1;
            if(i2c_checknack()) {       //If NACKF set by slave device
                i2c_clearnack();        //Clear, generate stop, return error
                i2c_stop();
                return -1;
            }
        }
        I2C1->TXDR = udata[i] & I2C_TXDR_TXDATA; //Send transmitted data
    }
    i2c_start(devaddr, size, 1);        //Start bit
    for(int i = 0; i < size; i++) {     //Send each byte
        int count = 0;
        while((I2C1->ISR & I2C_ISR_RXNE) == 0) {
            count++;
            if(count > 1000000)         //Too long
                return -1;
            if(i2c_checknack()) {       //Ensure valid transmission/receive
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }
        udata[i] = I2C1->RXDR;          //Store received data
    }

    //Check once more for error, then return received data
    while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
    if(I2C1->ISR & I2C_ISR_NACKF)
        return -1;
    i2c_stop();
    return 0;
}

