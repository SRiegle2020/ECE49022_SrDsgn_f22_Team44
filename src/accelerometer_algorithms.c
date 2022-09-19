#include "stm32f0xx.h"
#include "i2c.h"
#include "uart.h"
#include "rtc.h"
#include "accelerometer_algorithms.h"
#include <math.h>

//=============================================================================
// DETECT_STEP
//  * Processes the accelerometer data into floats from -8g to +8g.
//    NOTE: tried to write a function to do this for me, but returning floats
//          with system workbench kept returning junk values instead. Not sure
//          why this happened.
//  * Returns a 1 if a single step was recorded.
//  * NOTE: this algorithm assumes a 100Hz sampling rate.
//=============================================================================
int detect_step(void) {
    //Process the accelerometer data (convert from 16bit to +-8g).
    float ax = (float)(accelerometer_X())/4096;
    float ay = (float)(accelerometer_Y())/4096;
    float az = (float)(accelerometer_Z())/4096 - 1;

    //Calculate the magnitude of the current acceleration, store the old one
    float a_mag_curr;
    float a_mag_prev = a_mag_curr;
    a_mag_curr = sqrt(ax*ax + ay*ay + az*az);

    //Check for strong rising edge
    //The >0.02 was added to cancel out any effects from turning
    if(a_mag_curr > 1.4 && a_mag_prev < 1.4 && (a_mag_curr-a_mag_prev) > 0.02)
        return 1;
    return 0;
}

//=============================================================================
// BMR
//  * Uses the Harris-Benedict equation to calculate BMR.
//  * Takes weight in pounds (lbs), height in inches (in), and age in years.
//  * Also considers whether user is male or female
//  * NOTE: returning floats with system workbench kept returning junk values
//          instead. Not sure why this happened. Hence, returning as an int
//          by multiplying by 100 to preserve two decimal places.
//=============================================================================
int BMR(int weight, int height, int age, char sex) {
    if(sex == 'M')
        return (66 + (6.2 * (float)weight) + (12.7 * (float)height) - (6.76 * (float)age))*100;
    if(sex == 'F')
        return (655 + (4.35 * (float)weight) + (4.7 * (float)height) - (4.7 * (float)age))*100;
}

//=============================================================================
// AEE_IEEE
//  * Uses the EE algorithm made by Dongwoo Kim and Hee Chan Kim for the IEEE
//    journal. NOTE: we will specifically use the non-seperation, wrist-based
//    regression model (R^2 = 0.86). We cannot use any other mainly because we
//    do not have any wrist or hip mounted accelerometers.
//  * Due to the weird bug with floating types, this file will be void and will
//    keep all data within this file.
//  * Assumes a sampling rate of 100Hz.
//=============================================================================
int f_sample = 100;
float EE = 0;

void AEE_IEEE(void) {
    float IAA = ((float)(accelerometer_X())/4096
               + (float)(accelerometer_Y())/4096
               + (float)(accelerometer_Z())/4096 - 1)/f_sample;
    EE += 13.884 * IAA - 1.3517;
}

//=============================================================================
// OVERALL_EE
//  * Once a minute, the overall EE will be updated. This is done by adding the
//    estimated homeostatic expenditure per minute.
//    This will be called by the RTC clock's once-a-minute update.
//=============================================================================
void overall_EE(int weight, int height, int age, char sex) {
    EE += BMR(weight, height, age, sex)/86400;
}
