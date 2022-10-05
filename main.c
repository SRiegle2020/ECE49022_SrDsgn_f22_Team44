/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "uart.h"
			
void init_exti(void) {
	RCC->AHBENR  |=  RCC_AHBENR_GPIOAEN;			//Clock GPIOA
	GPIOA->MODER &= ~(GPIO_MODER_MODER0				//Configure PA0-2 as inputs
			 	    | GPIO_MODER_MODER1
					| GPIO_MODER_MODER2);
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0				//Set PUPDR
					| GPIO_PUPDR_PUPDR1);
	GPIOA->PUPDR |=  (GPIO_PUPDR_PUPDR0_0
					| GPIO_PUPDR_PUPDR1_0);

	RCC->APB2ENR 	  |=  RCC_APB2ENR_SYSCFGCOMPEN; 		//Clock the interrupt
	SYSCFG->EXTICR[1] &= ~(SYSCFG_EXTICR1_EXTI0_PA
						 | SYSCFG_EXTICR1_EXTI2_PA);		//Set to PA0, PA2
	EXTI->IMR  		  |=  EXTI_IMR_MR0 | EXTI_IMR_MR2;		//Unmask the interrupts for PA0,PA2
	EXTI->RTSR		  |=  EXTI_RTSR_TR0;					//Set to both rising and falling edge for PA0
	EXTI->FTSR		  |=  EXTI_FTSR_TR0;
	EXTI->RTSR		  |=  EXTI_RTSR_TR2;					//Set to rising edge for PA2 (changes when button is RELEASED)

	NVIC->ISER[0] = 1 << EXTI0_1_IRQn;
	NVIC->ISER[0] = 1 << EXTI2_3_IRQn;	//Enable the interrupts
}

void EXTI2_3_IRQHandler(void) {
	EXTI->PR |= EXTI_PR_PR2; 							//Acknowledge the interrupt
	printf("Button Released\n");
}

void EXTI0_1_IRQHandler(void) {
	EXTI->PR |= EXTI_PR_PR0;							//Acknowledge the interrupt
	if(!((GPIOA->IDR & 0x3)%3))
		printf("Clockwise\n");
	else
		printf("Counter-clockwise\n");
}


int main(void)
{
	init_usart5();
	init_exti();
	for(;;);
}
