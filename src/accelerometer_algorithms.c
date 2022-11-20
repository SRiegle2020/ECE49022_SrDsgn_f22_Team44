/*****************************************************************************
 * ACCELEROMETER_ALGORITHMS.C										         *
 * WATCH AND ACCELEROMETER DATA SUBSYSTEM									 *
 * Shannon Riegle															 *
 * ECE49022 Fall 2022, Team 44												 *
 * 																			 *
 * This code contains functions for obtaining, processing, and analyzing     *
 * accelerometer data to calculate information such as steps walked and EE   *
 * using MET. The EE algorithm was obtained from an IEEE article written by  *
 * Carneiro et. al. This algorithm calculates the fRMS of an accelerometer   *
 * magnitude array. This is then multiplied by 1.8, with the product		 *
 * subtracted by 15 to obtain MET. Then, MET is converted to kcal/(lb*min)   *
 * to be added to energy expenditure. The BMR is then also added.			 *
 * NOTE: The algorithms implemented recommend using a DLPF before processing.*
 * 		 This was not done in code, but rather by configuring the MPU6050's  *
 * 		 built-in low-pass-filter to filter at 5Hz.							 *
 *****************************************************************************/
#include "stm32f0xx.h"
#include "i2c.h"
#include "uart.h"
#include "rtc.h"
#include "accelerometer_algorithms.h"
#include <math.h>

#define CS_HIGH do { GPIOB->BSRR = GPIO_BSRR_BS_8; } while(0)

//ASSUMES A FREQUENCY OF 30Hz
float a_mag[1800]; 	//Table of Acceleration Magnitudes
float EE = 0;		//Energy Expenditure
float EE_exercise = 0;
short exercising = 0;

//=============================================================================
// ACCEL_SAMPLE
//	* Takes a sample from accelerometer.
//	* Process the data from signed 16bit to +-8g range.
//	* Calculate the magnitude of acceleration in g.
//  * Stores magnitude to an array.
//=============================================================================
void accel_sample(void) {
	//Update table
	for(int i = sizeof(a_mag)/sizeof(a_mag[0]);i>0;i--)
		a_mag[i] = a_mag[i-1];

	//Signal processing (converting to gs)
	float ax = (float)(accelerometer_X())/4096;
	float ay = (float)(accelerometer_Y())/4096;
	float az = (float)(accelerometer_Z())/4096;

	//Calculate magnitude
	a_mag[0] = sqrt(ax*ax + ay*ay + az*az);
}

//=============================================================================
// DETECT_STEP
//  * Looks at most recent accelerometer samples.
//	* Compares previous steps to determine if strong rising edge detected.
//  * Returns a 1 if a single step was recorded.
//  * NOTE: this algorithm should use a 30Hz sampling rate
//=============================================================================
int detect_step(void) {
	//May be able to remove later
	//accel_sample();

	//Detect if a step is past the threshold at a strong rising edge
    if(a_mag[0] > 1.03 && a_mag[1] < 1.03 && (a_mag[0]-a_mag[1]) > 0.05)
        return 1;
    return 0;
}

//=============================================================================
// BMR
//  * Uses the Harris-Benedict equation to calculate BMR.
//  * Takes weight in pounds (lbs), height in inches (in), and age in years.
//    Also considers whether user is male or female
//  * NOTE: returning floats with system workbench kept returning junk values
//          instead. Not sure why this happened. Hence, returning as an int
//          by multiplying by 100 to preserve two decimal places.
//	* Returns the BMR*100 (to account for the returning float problem).
//=============================================================================
int BMR(int weight, int height, int age, char sex) {
    if(sex == 'M')
        return (66 + (6.2 * (float)weight) + (12.7 * (float)height) - (6.76 * (float)age))*100;
    if(sex == 'F')
        return (655 + (4.35 * (float)weight) + (4.7 * (float)height) - (4.7 * (float)age))*100;
    return -1; //If neither M nor F is passed, return -1
}

//=============================================================================
// START_EXERCISING and END_EXERCISING
//  * Simple void functions that update a short data type.
//	* This will be used to determine if we are exercising or not.
//	* Updates global variable
//=============================================================================
void start_exercising(void) {
	exercising = 1;
	EE_exercise = 0; //Reset the exercising EE counter to 0
}

void end_exercising(void) {
	exercising = 0;
}

//=============================================================================
// EE_IEEE
//  * Uses the EE algorithm by Carneiro et. al.
//    https://ieeexplore.ieee.org/document/7145190
//  * Calculates the fRMS of the accelerometer data over the past minute.
//	  NOTE: This assumes a sampling rate of 30Hz. This already uses about
//		    11.2% of total RAM (3.6kB / 32kB).
//  * Calculates MET using the above equation.
//  * Then converts the MET to imperial units and finds EE for 1 minute.
//  * Returns EE multiplied by 100 (basically keeps the lower two decimals)
//=============================================================================
int EE_IEEE(int weight) {
	//Find fRMS
	float sum = 0;
	for(int i = 0; i < sizeof(a_mag)/sizeof(a_mag[0]);i++)
		sum += a_mag[i]*a_mag[i]*100;	//Multiply by 10 to convert from g to m/s^2
	sum /= (sizeof(a_mag)/sizeof(a_mag[0]));
	float fRMS = sqrt(sum);

	//Calculate MET using regression model
	float MET = (1.1278*fRMS - 9.5074)/2.2;	//Divide by 2.2 for conversion to kcal/(lbs*hrs)
	MET /= 60;							    //Multiply by 1/60 to get kcal/min
	EE += 1.05*MET*weight;					//BE SURE TO ADD BMR TO THIS (homeostasis)!!!

	if(exercising)
		EE_exercise += 1.05*MET*weight;

	//Convert to an Integer and Return
	int EE_int = EE * 100;
//	printf("fRMS: 				%6.2f\n",fRMS);
//	printf("MET in kCal/min*lb: %6.2f\n",MET);
	return(EE_int); //Returns EE times 100
}

//=============================================================================
// MIDNIGHT
//	* Called periodically. Returns a 1 if time is midnight.
//	* Used to reset counts if necessary.
//=============================================================================
int midnight() {
	if(get_minutes() == 0 && get_hour() == 0) {
		EE = 0;
		EE_exercise = 0;
		return 1;
	}
	return 0;
}
