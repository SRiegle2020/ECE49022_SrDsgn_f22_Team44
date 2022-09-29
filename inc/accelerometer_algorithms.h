/*****************************************************************************
 * This header file gives a list of the accelerometer algorithms used for    *
 * detecting a step and for calculating energy expenditure. It also will     *
 * calculate BMR from user height/weight/age/sex to calculate TOTAL energy   *
 * expenditure.																 *
 * TO-DO:																     *
 * 	(1) Add functions that reset counters at midnight					     *
 * 	(2) Add functions to return 2-decimal approximations to main for display *
 *****************************************************************************/
#include "stm32f0xx.h"
#include "i2c.h"
#include "uart.h"
#include "rtc.h"
#include <math.h>

void accel_sample(void);							//Updates the accelerometer array (1min @ 30Hz)
int detect_step(void);								//Returns a 1 if a step (strong rising edge)
int BMR(int weight, int height, int age, char sex);	//Calculate BMR
int EE_IEEE(int weight);							//Find Energy Expended, return EE as int (saves last two decimals)
void start_exercising(void);						//Sets an exercising "boolean" to true
void end_exercising(void);							//Sets an exercising "boolean" to false
int midnight();				//Resets EE counters and returns a 1 if midnight
