/*****************************************************************************
 * This header file gives a list of the RTC helper functions utilized in the *
 * Watch and Accelerometer Data subsystem. This will be used for both the    *
 * general watch functions and likely for the energy expenditure and step    *
 * counter algorithms. It will certainly be used for reseting EE and step    *
 * counts after midnight.                                                    *
 *****************************************************************************/
#include "stm32f0xx.h"

void watch_write(uint8_t reg, uint8_t val); //Write data to the RTC
uint8_t watch_read(uint8_t reg);            //Read from the RTC
int get_seconds(void);                      //The following get time & date
int get_minutes(void);
int get_hour(void);
int get_day(void);
int get_dayofweek(void);
int get_month(void);
int get_year(void);
void set_seconds(int seconds);              //The following set time & date
void set_minutes(int minutes);
void set_hours(int hours);
void set_day(int day);
void set_dayofweek(int dayofweek);
void set_month(int month);
void set_year(int year);
void init_watch(void);                      //This command sets up the RTC
