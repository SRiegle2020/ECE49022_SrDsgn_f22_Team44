//==============================================================================
// DEFINE LIBRARIES
//	Define all libraries to be used for the project.
//==============================================================================
#include "stm32f0xx.h"
#include "i2c.h"
#include "uart.h"
#include "rtc.h"
#include "accelerometer_algorithms.h"
#include "sensors.h"
#include "lcd.h"

//==============================================================================
// DEFINE TEST CASES
//	Will be useful if needing UART debugging
//==============================================================================
#define TEST_NONE	0x00
#define TEST_SPO2 	0x01
#define TEST_STEP  	0x02
#define TEST_EE		0x04
#define TEST_TIME	0x10
#define TEST_HR		0x20
#define TEST_ALL    TEST_SPO2 | TEST_STEP | TEST_EE | TEST_TIME | TEST_HR

//IMPORTANT ==> Change this to change what tests you are running with the UART
int tests = TEST_ALL;

//==============================================================================
// INITIALIZE GLOBAL VARIABLES
//	Initializes all variables that will be used for the project.
//==============================================================================
int i	  	= 0;
int spo2  	= 0;
int HR		= 70;
int steps 	= 0;
int EE_a  	= 0;
int time	= 0;

void TIM6_DAC_IRQHandler(void) {
    TIM6->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt

    //Get SpO2 Data
    pulseox_check();
    spo2 = get_spo2();
    HR   = get_HR();

    //Get Steps
    accel_sample();
    if(detect_step())
    	steps++;

    //Once a minute, update the EE counter
    if(i == 30*60) {
    	EE_a = EE_IEEE(120);
        i = 0;
    }

    //Get the time
    if(!(i%10)) //Update @ 3Hz (no need to continually update it)
    	time = get_hour() * 100 + get_minutes();

    //PRINT TEST CASES TO UART
    if(tests & TEST_SPO2) {
    	if(spo2 == -1)
    		printf("WRIST NOT DETECTED\n");
		else
			printf("SPO2:  %d%%\n",spo2);
    }
    if(tests & TEST_STEP)
    	printf("STEPS: %d\n",steps);
    if(tests & TEST_EE)
    	printf("EE:    %.2f\n",(float)EE_a/100);
    if(tests & TEST_TIME)
    	printf("TIME: %d%d:%d%d\n",time/1000,(time/100)%10,(time%100)/10,time%10);
    if(tests & TEST_HR)
    	printf("HR:    %d BPM\n",HR);
    i++; //Increment the counter
}

void init_tim6(void) {
	//Set to 30Hz sampling
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 1000 - 1;
    TIM6->ARR = 1600 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

//=============================================================================
// SAMPLE MAIN
//	Initializes all ICs.
//	Then, initialize the timer when everything is configured.
//=============================================================================
int main(void)
{
	init_i2c();
	init_usart5();
	pulseox_setup();
	init_accelerometer();

	init_tim6();
	while(1);
}
