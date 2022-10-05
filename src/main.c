
#include "stm32f0xx.h"
#include "i2c.h"
#include "uart.h"
#include "rtc.h"
#include "accelerometer_algorithms.h"
#include "sensors.h"
#include <math.h>
#include "lcd.h"
//=============================================================================
// SAMPLE MAIN
// Will test in lab after getting header pins soldered and wires added, but
// should allow accelerometer data to be sent to the serial port.
//=============================================================================
int steps = 0;
int i = 0;
void TIM6_DAC_IRQHandler(void) {
    TIM6->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
//    accel_sample();
//    if(detect_step())
//        printf("Steps: %d\n",++steps);
    //printf("%hi %hi %hi\n",accelerometer_X(), accelerometer_Y(), accelerometer_Z());
    //AEE_IEEE();
//    if(i++ == 30*60) {
//    	int EE = EE_IEEE(120);
    	//printf("RETURNED EE: %6.2f\n",(float)EE/100);
//    	i = 0;
//    }
    pulseox_check();
    //nano_wait(1000);
    get_spo2();
    accel_sample();
    if(detect_step())
        printf("Steps: %d\n",++steps);
        //printf("%hi %hi %hi\n",accelerometer_X(), accelerometer_Y(), accelerometer_Z());
        //AEE_IEEE();
    if(i++ == 30*60) {
        int EE = EE_IEEE(120);
        //printf("RETURNED EE: %6.2f\n",(float)EE/100);
        i = 0;
    }
//
//    if(!(i%600))
//        printf("%2d:%2d\n",get_hour(),get_minutes());
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

int main(void)
{
    //init_i2c();
    //init_usart5();
    //init_watch();
    //printf("%d:%d\n",get_hour(),get_minutes());
    //init_accelerometer();
    //init_tim6();
    //while(1) {}
	init_i2c();
	init_usart5();
	pulseox_setup();
	//for(int i = 0; i < 6000000; i++)
	    //nano_wait(1000);
	init_accelerometer();
	init_tim6();
	//get_spo2();
	while(1) {
		//pulseox_readfifo();
	}
}
