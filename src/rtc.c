/*****************************************************************************
 * This code contains a list of all the RTC helper functions that allow the  *
 * user to retrieve and adjust time/date information. Useful for conveying   *
 * date information to the user. Also may be used for EE calculations and    *
 * for reseting the calorie-burn and step counters at midnight.              *
 *****************************************************************************/
#include "stm32f0xx.h"
#include "i2c.h"

#define WATCH_ADDR 0x68

//=============================================================================
// WATCH_WRITE
//  * Write data to the RTC.
//  * Used for reseting, configuring, etc.
//=============================================================================
void watch_write(uint8_t reg, uint8_t val) {
    uint8_t watch_writedata[2] = {reg, val};
    i2c_senddata(WATCH_ADDR, watch_writedata,2);
}

//=============================================================================
// WATCH_READ
//  * Read data from the RTC.
//  * This uses the standard I2C interface (that is, the one with a STOP and
//    not a reSTART to switch from write to read).
//=============================================================================
uint8_t watch_read(uint8_t reg) {
    uint8_t watch_data[1] = {reg};
    i2c_senddata(WATCH_ADDR, watch_data, 1);
    i2c_recvdata_P(WATCH_ADDR, watch_data, 1);
    return I2C1->RXDR & 0xff;
}

//=============================================================================
// GET_#########
//  * Get the seconds/minutes/hours/etc. recorded by the RTC.
//  * The get_seconds() algorithm also contains a bit indicating if the
//    OSCILLATOR has stopped (usually only on startup or restart).
//=============================================================================
int get_seconds(void) {
    int bcd_seconds  = watch_read(0x03);    //Get Binary-Coded-Decimal Seconds
    if(bcd_seconds >> 7)                    //Check if Crystal-Off Flag Set
            return -1;
    int tens_seconds = (0x70 & bcd_seconds) >> 4;   //Calculate 10s, 1s
    int unit_seconds = 0xf & bcd_seconds;
    return(tens_seconds*10 + unit_seconds);         //Return seconds
}

int get_minutes(void) {
    int bcd_minutes  = watch_read(0x04);    //Get Binary-Coded-Decimal Seconds
    int tens_minutes = (0x70 & bcd_minutes) >> 4;   //Calculate 10s, 1s
    int unit_minutes =  0x0f & bcd_minutes;
    return(tens_minutes*10 + unit_minutes);         //Return minutes
}

int get_hour(void) {
    int bcd_hour  = watch_read(0x05);        //Get Binary-Coded-Decimal Hours
    int tens_hour = (0x30 & bcd_hour) >> 4; //Calculate 10s,1s
    int unit_hour =  0x0f & bcd_hour;
    return(tens_hour*10 + unit_hour);       //Return hours
}

int get_day(void) {
    int bcd_days  = watch_read(0x06);       //Get Binary-Coded-Decimal Days
    int tens_days = (0x30 & bcd_days) >> 4; //Calculate 10s,1s
    int unit_days =  0x0f & bcd_days;
    return(tens_days*10 + unit_days);       //Return hours
}

int get_dayofweek(void) {
    return(watch_read(0x07) & 0x3); //Return Day of Week (0 = Sun, 1 = Mon, ...)
}

int get_month(void) {
    int bcd_month  = watch_read(0x08);
    int tens_month = (0x10 & bcd_month) >> 4;
    int unit_month =  0x0f & bcd_month;
    return(tens_month*10 + unit_month);
}

int get_year(void) {
    int bcd_year  = watch_read(0x09);
    int tens_year = bcd_year >> 4;
    int unit_year = bcd_year & 0xf;
    return(tens_year*10 + unit_year);
}

//=============================================================================
// SET_########(int ########)
//  * Sets the seconds/minutes/hours/etc.
//  * Must convert from regular values to BCD.
//=============================================================================
void set_seconds(int seconds) {
    watch_write(0x03, ((seconds / 10) << 4) | (seconds % 10));
}

void set_minutes(int minutes) {
    watch_write(0x04, ((minutes / 10) << 4) | (minutes % 10));
}

void set_hours(int hours) {
    watch_write(0x05, ((hours / 10) << 4) | (hours % 10));
}

void set_day(int day) {
    watch_write(0x06, ((day / 10) << 4) | (day % 10));
}

void set_dayofweek(int dayofweek) {
    watch_write(0x07, dayofweek);
}

void set_month(int month) {
    watch_write(0x08, ((month / 10) << 4) | (month % 10));
}

void set_year(int year) {
    watch_write(0x09, ((year / 10) << 4) | (year % 10));
}

//=============================================================================
// INIT_WATCH
//  * Resets the watch when called
//    Probably never going to get called since watch automatically resets on
//    startup, but included in case it was found necessary later.
//  * Sets to a standard switch-over, low-detection disable. This means that if
//    the main power drops below the backup battery AND below the threshold for
//    the switch, the RTC will start using the backup power. This lets the RTC
//    keep tracking time even if the rest of the watch is dead. The low battery
//    detection is disabled since the POWER subsystem has designed a circuit to
//    turn on an indicator LED if low power is detected.
//  * Clears the Oscillator-STOP flag. This allows the user to read from the
//    seconds. NOTE: I should probably add a 2s delay to ensure that the
//    oscillator has started BEFORE disabling the flag. Ultimately, this has
//    had no effect on tracking time so far.
//=============================================================================
void init_watch(void) {
    watch_write(0x00,0x00);                         //Reset
    watch_write(0x02,0x40);                         //Standard Switch-Over, Low Detection Disabled
    int clear_oscflag = watch_read(0x03) &~ 0x80;   //Clear the Oscillator-STOP flag
    watch_write(0x03,clear_oscflag);

    //NOT WORKING! MEANT TO BE A INTERRUPT
    //May just use RTC for tracking time, and use internal timer to check stuff.
    //Then use RTC to recalibrate if necessary
    watch_write(0x10,0x2); 					  //Set to 1Hz Frequency Control
    watch_write(0x11,60);  				      //Set Reload to 60 (p33 of datasheet)
    int timAflag = watch_read(0x01) & ~0x40;  //Clear the Timer A Flag
    timAflag |= 0x2; 						  //Enable TimA interrupt
    watch_write(0x01,timAflag);

    int clkout_ctrl = watch_read(0x0f);       //Set to pulsed interrupt, enable TimA
    clkout_ctrl |= 0x82;
    watch_write(0x0f,clkout_ctrl);
}

//TO-DO; Add a WATCHDOG interrupt that sends a GPIO signal once a minute. This
//will allow the user to not need an internal TIMER peripheral to update time
//displayed.
