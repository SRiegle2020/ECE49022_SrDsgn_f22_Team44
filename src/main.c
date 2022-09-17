#include "stm32f0xx.h"
#include <stdio.h>
#include "uart.h"
#include "i2c.h"
#include "sensors.h"
#include <math.h>

void init_tim6(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 4800 - 1;
    TIM6->ARR = 3333 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

void TIM6_DAC_IRQHandler(void) {
    TIM6->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
    printf("A_x: %5d   %5d  %5d\n",accelerometer_X(),accelerometer_Y(),accelerometer_Z());
}

//=============================================================================
// SAMPLE MAIN
// Will test in lab after getting header pins soldered and wires added, but
// should allow accelerometer data to be sent to the serial port.
//=============================================================================
int main(void)
{
    init_i2c();
    init_usart5();
    init_accelerometer();
    init_tim6();

	while(1) {}
}
