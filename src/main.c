#include "stm32f0xx.h"
#include "i2c.h"
#include "uart.h"
#include "rtc.h"
#include "accelerometer_algorithms.h"
#include <math.h>
//=============================================================================
// SAMPLE MAIN
// Will test in lab after getting header pins soldered and wires added, but
// should allow accelerometer data to be sent to the serial port.
//=============================================================================
int steps = 0;

void TIM6_DAC_IRQHandler(void) {
    TIM6->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
    if(detect_step())
        printf("Steps: %d\n",++steps);
    //printf("%hi %hi %hi\n",accelerometer_X(), accelerometer_Y(), accelerometer_Z());
}

void init_tim6(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 480  - 1;
    TIM6->ARR = 1000 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

int main(void)
{
    init_i2c();
    init_usart5();
    init_watch();
    init_accelerometer();
    printf("Harris Benedict: %.2f\n",(float)BMR(120,71,20,'M')/100);
    init_tim6();
    while(1) {}
}
