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
void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}
void TIM6_DAC_IRQHandler(void) {
    TIM6->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
//    accel_sample();
//    if(detect_step())
//        printf("Steps: %d\n",++steps);
//    //printf("%hi %hi %hi\n",accelerometer_X(), accelerometer_Y(), accelerometer_Z());
//    //AEE_IEEE();
//    if(i++ == 30*60) {
//    	int EE = EE_IEEE(120);
//    	printf("RETURNED EE:		%6.2f\n",(float)EE/100);
//    	i = 0;
//    }

//    pulseox_check();
//    nano_wait(1000);
//    //get_spo2();
//    accel_sample();
//    if(detect_step())
//        printf("Steps: %d\n",++steps);
//        //printf("%hi %hi %hi\n",accelerometer_X(), accelerometer_Y(), accelerometer_Z());
//        //AEE_IEEE();
//    if(i++ == 30*60) {
//        int EE = EE_IEEE(120);
//        printf("RETURNED EE: %6.2f\n",(float)EE/100);
//        i = 0;
//    }
//
//    if(!(i%600))
//        printf("%2d:%2d\n",get_hour(),get_minutes());
}

void init_tim6(void) {
	//Set to 30Hz sampling
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 1000 - 1;
    TIM6->ARR = 1600 - 1;
    //TIM6->PSC   = 4800 - 1;
    //TIM6->ARR   = 1000 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

void init_lcd_spi() {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0x30c30000;
    GPIOB->MODER |=  0x10410000;
    init_spi1_slow();       //No need to redefine these two
    sdcard_io_high_speed(); //Both already exist in wavPlayer.c
}

void init_spi1_slow() {
    RCC->APB2ENR  |=  RCC_APB2ENR_SPI1EN; //ENABLE SPI1 RCC CLOCK
    RCC->AHBENR   |=  RCC_AHBENR_GPIOBEN; //ENABLE GPIOB
    GPIOB->MODER  &= ~0XFC0;              //CLEAR PB3,4,5
    GPIOB->MODER  |=  0XA80;              //SET PB3,4,5 TO ALTFN
    GPIOB->AFR[0] &= ~0XFFF000;           //SET AFR3,4,5 TO AF0

    SPI1->CR1 &= ~SPI_CR1_SPE; //DISABLE SPI
    SPI1->CR1 |= SPI_CR1_BR;   //SET BAUD RATE
    SPI1->CR1 |= SPI_CR1_MSTR; //MASTER MODE
    SPI1->CR2  = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0; //SET TO 8 BIT
    SPI1->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP; //OTHER STUFF
    SPI1->CR2 |= SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_SPE;  //ENABLE
}

void sdcard_io_high_speed() {
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_BR;
    //SPI1->CR1 |=  SPI_CR1_BR;
    SPI1->CR1 |=  SPI_CR1_SPE;
}

int main(void)
{
//    init_i2c();
//    init_usart5();
//    init_watch();
//    printf("%d:%d\n",get_hour(),get_minutes());
//    init_accelerometer();

//    while(1) {}
//	init_i2c();
//	init_usart5();
//	pulseox_setup();
//	for(int i = 0; i < 6000000; i++)
//	    nano_wait(1000);
//	init_accelerometer();
//	init_tim6();
	init_i2c();
	init_watch();
	LCD_Setup();
	LCD_Clear(BLACK);
	set_hours(19);
	set_minutes(57);
	int current_time = 0;
	//LCD_DrawString(10,10,WHITE,WHITE,"Hello there.",16,0xff);
	while(1) {
		int prev_time = current_time;
		current_time = get_hour() * 100 + get_minutes();
		if(current_time != prev_time) {
			LCD_DrawFillRectangle(10,10,45+10,26,BLACK);
			char string[5];
			int hour = get_hour();
			int minute = get_minutes();
			sprintf(string,"%d%d:%d%d",hour/10,hour%10,minute/10,minute%10);
			LCD_DrawString(10,10,WHITE,WHITE,string,16,0xff);
		}
	}

		//pulseox_readfifo();
}
