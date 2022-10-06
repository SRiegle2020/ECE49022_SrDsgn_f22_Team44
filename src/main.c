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
#define TEST_HR	    0x20
#define TEST_ENCODER 0x40
#define TEST_ALL    TEST_SPO2 | TEST_STEP | TEST_EE | TEST_TIME | TEST_HR | TEST_ENCODER

//IMPORTANT ==> Change this to change what tests you are running with the UART
int tests = TEST_ALL;

//==============================================================================
// INITIALIZE GLOBAL VARIABLES
//	Initializes all variables that will be used for the project.
//==============================================================================
int   i	  			= 0;
int   prev_spo2 	= 0;
int   spo2  		= 0;
int   prev_HR   	= 0;
int   HR			= 0;
float prev_tempF 	= 0;
float tempF  		= 0;
int   prev_steps    = 0;
int   steps 	    = 0;
int   prev_EE_a     = 0;
int   EE_a  		= 0;
int	  prev_hour     = 0;
int   hour			= 0;
int   prev_minute   = 0;
int   minute  		= 0;

int  ft   = 5;
int  inch = 10;
int  wgt  = 150;
int  age  = 20;
char sex = 'M';
char string[38];

void init_exti(void) {
    RCC->AHBENR  |=  RCC_AHBENR_GPIOAEN;            //Clock GPIOA
    GPIOA->MODER &= ~(GPIO_MODER_MODER0             //Configure PA0-2 as inputs
                    | GPIO_MODER_MODER1
                    | GPIO_MODER_MODER2);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0             //Set PUPDR
                    | GPIO_PUPDR_PUPDR1);
    GPIOA->PUPDR |=  (GPIO_PUPDR_PUPDR0_0
                    | GPIO_PUPDR_PUPDR1_0);

    RCC->APB2ENR      |=  RCC_APB2ENR_SYSCFGCOMPEN;         //Clock the interrupt
    SYSCFG->EXTICR[1] &= ~(SYSCFG_EXTICR1_EXTI0_PA
                         | SYSCFG_EXTICR1_EXTI2_PA);        //Set to PA0, PA2
    EXTI->IMR         |=  EXTI_IMR_MR0 | EXTI_IMR_MR2;      //Unmask the interrupts for PA0,PA2
    EXTI->RTSR        |=  EXTI_RTSR_TR0;                    //Set to both rising and falling edge for PA0
    EXTI->FTSR        |=  EXTI_FTSR_TR0;
    EXTI->RTSR        |=  EXTI_RTSR_TR2;                    //Set to rising edge for PA2 (changes when button is RELEASED)

    NVIC->ISER[0] = 1 << EXTI0_1_IRQn;
    NVIC->ISER[0] = 1 << EXTI2_3_IRQn;  //Enable the interrupts
}

void EXTI2_3_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR2;                            //Acknowledge the interrupt
    if(tests & TEST_ENCODER)
        printf("Button Released\n");
}

void EXTI0_1_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR0;                            //Acknowledge the interrupt
    LCD_DrawFillRectangle(0,214,319,239,WHITE);
    LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST HEIGHT?--",12,0xff);
    if(tests & TEST_ENCODER) {
        if(!((GPIOA->IDR & 0x3)%3))
            printf("Clockwise\n");
        else
            printf("Counter-clockwise\n");
    }
}

void TIM6_DAC_IRQHandler(void) {
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
    if(!(i%10)) {//Update @ 3Hz (no need to continually update it)
    	hour   = get_hour();
    	minute = get_minutes();
    }

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
    	printf("TIME:  %02d:%02d\n",hour,minute);
    if(tests & TEST_HR)
    	printf("HR:    %d BPM\n",HR);
    i++; //Increment the counter

    TIM6->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
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

void TIM2_IRQHandler(void) {
	TIM6->CR1 &= ~TIM_CR1_CEN;
    TIM2->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
    if(minute != prev_minute | prev_spo2 != spo2 | prev_HR != HR | tempF != prev_tempF | prev_steps != steps | prev_EE_a != EE_a) {
    	LCD_Setup();
    	LCD_Clear(BLACK);
		sprintf(string,"%02d:%02d",hour,minute);
		LCD_DrawString(10,10,WHITE,WHITE,string,16,0xff);

		if(spo2 != -1) {
			sprintf(string,"SpO2:  %-3d   %%",spo2);
			LCD_DrawString(26,10+16,WHITE,WHITE,string,16,0xff);
		    sprintf(string,"HR:    %-3d   BPM",HR);
		    LCD_DrawString(26,10+32,WHITE,WHITE,string,16,0xff);
		} else {
		    LCD_DrawString(26,10+16,WHITE,WHITE,"SpO2:  N/A",16,0xff);
		    LCD_DrawString(26,10+32,WHITE,WHITE,"HR:    N/A",16,0xff);
		}

		sprintf(string,"Temp:  %-5.1f degF",tempF);
		LCD_DrawString(26,10+48,WHITE,WHITE,string,16,0xff);

		sprintf(string,"Steps: %-5d Steps",steps);
		LCD_DrawString(26,10+64,WHITE,WHITE,string,16,0xff);
		sprintf(string,"EE:    %-4d  Cal",EE_a/100);            //Truncate to Int
		LCD_DrawString(26,10+80,WHITE,WHITE,string,16,0xff);

		sprintf(string,"Hgt: %d'%-2d\"",ft,inch);
		LCD_DrawString(26,10+112,WHITE,WHITE,string,16,0xff);
		sprintf(string,"Wgt: %-3d lbs",wgt);
		LCD_DrawString(26,10+126,WHITE,WHITE,string,16,0xff);
		sprintf(string,"Age: %-3d yrs",age);
		LCD_DrawString(26,10+142,WHITE,WHITE,string,16,0xff);
		sprintf(string,"Sex: %c",sex);
		LCD_DrawString(26,10+158,WHITE,WHITE,string,16,0xff);

		prev_minute = minute;
		prev_spo2   = spo2;
		prev_HR		= HR;
		prev_tempF  = tempF;
		prev_steps  = steps;
		prev_EE_a   = EE_a;
    }
    TIM6->CR1 |=  TIM_CR1_CEN;
}

void init_tim2(void) {
	//Set to update display every 5 seconds
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 4800  - 1;
    TIM2->ARR = 50000 - 1;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1  |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM2_IRQn;
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
    //SPI1->CR1 |=  SPI_CR1_BR_0;
    SPI1->CR1 |=  SPI_CR1_SPE;
}

//=============================================================================
// SAMPLE MAIN
//	Initializes all ICs.
//	Then, initialize the timer when everything is configured.
//=============================================================================
int main(void)
{
    LCD_Setup();
    LCD_Clear(BLACK);

	init_i2c();
	init_usart5();
	pulseox_setup();
	init_accelerometer();
	init_exti();

    init_tim6();
    init_tim2();
	while(1);
}
