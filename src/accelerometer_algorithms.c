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

//ASSUMES A FREQUENCY OF 30Hz
float a_mag[1800]; 	//Table of Acceleration Magnitudes
float EE = 0;			//Energy Expenditure

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
    if(a_mag[0] > 1.3 && a_mag[1] < 1.3 && (a_mag[0]-a_mag[1]) > 0.02)
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
//=============================================================================
void EE_IEEE(void) {
	//Find fRMS
	float sum = 0;
	for(int i = 0; i < sizeof(a_mag)/sizeof(a_mag[0]);i++)
		sum += a_mag[i]*a_mag[i]*100;	//Multiply by 10 to convert from g to m/s^2
	sum /= (sizeof(a_mag)/sizeof(a_mag[0]));
	float fRMS = sqrt(sum);

	//Calculate MET from table given
	float MET = (1.8*fRMS - 15)/2.2;	//Divide by 2.2 for conversion to kcal/(lbs*hrs)
	MET /= 60;							//Multiply by 1/60 to get kcal/min
	EE += 1.05*MET*120;					//BE SURE TO ADD BMR TO THIS (homeostasis)!!!

	//Print (Used for Debugging, not for actual product)
	printf("EE: %6.2f\n",EE);
}
