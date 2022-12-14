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

#define NO_INIT_GCC __attribute__ ((section (".noinit")))

//==============================================================================
// DEFINE TEST CASES
//	Will be useful if needing UART debugging
//==============================================================================
#define TEST_NONE	 0x00
#define TEST_SPO2 	 0x01
#define TEST_STEP    0x02
#define TEST_EE		 0x04
#define TEST_TIME	 0x10
#define TEST_HR	     0x20
#define TEST_ENCODER 0x40
#define TEST_TEMP	 0x80
#define TEST_AUDIO   0x100
#define TEST_ALL    TEST_SPO2 | TEST_STEP | TEST_EE | TEST_TIME | TEST_HR | TEST_ENCODER | TEST_AUDIO
#define CS_HIGH do { GPIOB->BSRR = GPIO_BSRR_BS_8; } while(0)

//IMPORTANT ==> Change this to change what tests you are running with the UART
int tests = TEST_NONE;

//==============================================================================
// INITIALIZE GLOBAL VARIABLES
//	Initializes all variables that will be used for the project.
//==============================================================================
int   i	  			= 0;
int   prev_spo2 	= 0;
int   spo2  		= 0;
int   prev_HR   	= 0;
int   HR			= 0;
int   prev_tempF 	= 0;
int   tempF  		= 0;
int   prev_steps    = 0;
int   steps 	    = 0;
int   prev_EE_a     = 0;
int   EE_a  		= 0;
int	  prev_hour     = 0;
int   hour			= 0;
int   prev_minute   = 0;
int   minute  		= 0;

static int  ft   NO_INIT_GCC;
static int  inch NO_INIT_GCC;
static int  wgt  NO_INIT_GCC;
static int  age  NO_INIT_GCC;
static char sex  NO_INIT_GCC;
static char string[20];

int mode = 0;

static inline void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void init_exti(void) {
    RCC->AHBENR  |=  RCC_AHBENR_GPIOAEN;            //Clock GPIOA
    GPIOA->MODER &= ~(GPIO_MODER_MODER0             //Configure PA0-2 as inputs
                    | GPIO_MODER_MODER1
                    | GPIO_MODER_MODER2);
    GPIOA->MODER |= GPIO_MODER_MODER5_0;            //Set PA5 to output
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

//============================================================================
// CHECK_VITALS
//  * Check if user vitals are healthy. If not, play alert tone.
//============================================================================
void check_vitals(void) {
	if((tempF > 991 || (spo2 < 95) || HR < 60 || HR > 100) && spo2 > 0)
		TIM7->CR1 |=  TIM_CR1_CEN;
	else
		TIM7->CR1 &= ~TIM_CR1_CEN;

	if(tests & TEST_AUDIO)
		TIM7->CR1 |=  TIM_CR1_CEN;
	return;
}

//==============================================================================
// EXTI2_3_IRQHandler
//  * Uses an EXTI for PA2 to detect if the encoder button was pressed.
//  * Cycles through the different configuration settings before returning to
//    display.
//==============================================================================
void EXTI2_3_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR2;                            //Acknowledge the interrupt
    TIM6->CR1 &= ~TIM_CR1_CEN;
    TIM2->CR1 &= ~TIM_CR1_CEN;
    mode++;
    nano_wait(100000000); //Add a delay to account for "double presses"
    //Change modes
    if(mode == 1) {
        LCD_Setup();
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST HGT (FEET)--",16,0xff);
        sprintf(string,"%d'",ft);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
    } else if(mode == 2) {
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST HGT (INCH)--",16,0xff);
        sprintf(string,"%2d'",inch);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
    } else if(mode == 3) {
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST WGT--",16,0xff);
        sprintf(string,"%d lbs",wgt);
        LCD_DrawString(260,219,BLACK,BLACK,string,16,0xff);
    } else if(mode == 4) {
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST AGE--",16,0xff);
        sprintf(string,"%d yrs",age);
        LCD_DrawString(260,219,BLACK,BLACK,string,16,0xff);
    } else if(mode == 5) {
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST SEX--",16,0xff);
        sprintf(string,"%c",sex);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
    } else if(mode == 6) {
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST TIME (HRS)--",16,0xff);
        sprintf(string,"%02d:%02d",hour,minute);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
    } else if(mode == 7) {
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST TIME (MIN)--",16,0xff);
        sprintf(string,"%02d:%02d",hour,minute);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);

    //If done editing, redraw entire thing
    } else {
        mode = 0;
        set_hours(hour);
        set_minutes(minute);
        LCD_Setup();
        LCD_Clear(0x18e4);
        sprintf(string,"%02d:%02d",hour,minute);
        LCD_DrawString(104,96,0xf924,BLACK,string,48,0xff);

        LCD_DrawFillRectangle(0, 0, 320, 6, 0xa65b);
        LCD_DrawFillRectangle(0, 0, 6, 240, 0xa65b);
        LCD_DrawFillRectangle(0, 234, 320, 240, 0xa65b);
        LCD_DrawFillRectangle(314, 0, 320, 240, 0xa65b);

        sprintf(string,"SpO2");
        LCD_DrawString(10 + 112,10,WHITE,WHITE,string,16,0xff);
        sprintf(string,"HR");
        LCD_DrawString(10 + 180,10,WHITE,WHITE,string,16,0xff);

        if(spo2 != -1) {
            sprintf(string,"%-3d%%",spo2);
            LCD_DrawString(10 + 112,10 + 16,WHITE,WHITE,string,16,0xff);
            sprintf(string,"%-3d",HR);
            LCD_DrawString(10 + 180,10+16,WHITE,WHITE,string,16,0xff);
        } else {
            LCD_DrawString(10 + 112,10+16,WHITE,WHITE,"N/A",16,0xff);
            LCD_DrawString(10 + 180,10+16,WHITE,WHITE,"N/A",16,0xff);
        }

        sprintf(string,"TEMP");
        LCD_DrawString(10 + 235,10,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%d.%d^F",tempF/10,tempF%10);
        LCD_DrawString(10 + 235,10 + 16,WHITE,WHITE,string,16,0xff);

        sprintf(string,"STEPS");
        LCD_DrawString(10,10,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%-5d",steps);
        LCD_DrawString(10,10+16,WHITE,WHITE,string,16,0xff);

        sprintf(string,"EE");            //Truncate to Int
        LCD_DrawString(10 + 64,10,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%-4d",EE_a/100);
        LCD_DrawString(10 + 64,10 + 16,WHITE,WHITE,string,16,0xff);

        sprintf(string,"HEIGHT");
        LCD_DrawString(100,213,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%d'%2d\"",ft,inch);
        LCD_DrawString(100,197,WHITE,WHITE,string,16,0xff);

        sprintf(string,"WEIGHT",wgt);
        LCD_DrawString(10,213,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%-3dlbs",wgt);
        LCD_DrawString(10,197,WHITE,WHITE,string,16,0xff);

        sprintf(string,"Age");
        LCD_DrawString(200,213,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%-3d",age);
        LCD_DrawString(200,197,WHITE,WHITE,string,16,0xff);

        sprintf(string,"Sex");
        LCD_DrawString(280,213,WHITE,WHITE,string,16,0xff);
        sprintf(string,"%c",sex);
        LCD_DrawString(280,197,WHITE,WHITE,string,16,0xff);

        //Update previous values
        prev_minute = minute;
        prev_spo2   = spo2;
        prev_HR     = HR;
        prev_tempF  = tempF;
        prev_steps  = steps;
        prev_EE_a   = EE_a;

        TIM6->CR1 |= TIM_CR1_CEN;
        TIM2->CR1 |= TIM_CR1_CEN;
    }

    //Used for UART Debugging
    if(tests & TEST_ENCODER)
        printf("Button Released\n");

}

//==============================================================================
// EXTI0_1_IRQHandler
//  * Uses an EXTI for PA0 and PA1 to detect whether encoder is rotated.
//  * Changes settings based off of different modes.
//==============================================================================
void EXTI0_1_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR0; //Acknowledge the interrupt
    int increment;
    if(!((GPIOA->IDR & 0x3)%3))
        increment = 1;
    else
        increment = -1;

    if(mode == 1) {
        ft += increment;
        //Keep values in the range [0,9]
        if(ft < 0)
            ft = 0;
        if(ft > 9)
            ft = 9;
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST HGT (FEET)--",16,0xff);
        sprintf(string,"%d'",ft);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
        RTC->BKP0R = ft;
    } else if(mode == 2) {
        inch += increment;
        //Keep values in the range [0,12)
        if(inch < 0)
            inch = 0;
        if(inch > 11)
            inch = 11;
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST HGT (INCH)--",16,0xff);
        sprintf(string,"%2d'",inch);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
        RTC->BKP1R = inch;
    } else if(mode == 3) {
        wgt += increment;
        if(wgt < 50)  //Keep values in range [50,600] to account for kids and
                      //very big people
            wgt = 50;
        if(wgt > 600)
            wgt = 600;
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST WGT--",16,0xff);
        sprintf(string,"%d lbs",wgt);
        LCD_DrawString(260,219,BLACK,BLACK,string,16,0xff);
        RTC->BKP2R = wgt;
    } else if(mode == 4) {
        age += increment;
        if(age < 0)     //Limit ages to [0,110]
            age = 0;
        if(age > 110)
            age = 110;
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST AGE--",16,0xff);
        sprintf(string,"%d yrs",age);
        LCD_DrawString(260,219,BLACK,BLACK,string,16,0xff);
        RTC->BKP3R = age;
    } else if(mode == 5) {
        if(sex == 'M')
            sex = 'F';
        else
            sex = 'M';
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST SEX--",16,0xff);
        sprintf(string,"%c",sex);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
        RTC->BKP4R = sex;
    } else if(mode == 6) {
        hour += increment;
        if(hour > 23)
            hour = 0;
        if(hour < 0)
            hour = 23;
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST TIME (HRS)--",16,0xff);
        sprintf(string,"%02d:%02d",hour,minute);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
        //TimeHr
    } else if(mode == 7) {
        //TimMin
        minute += increment;
        if(minute > 59)
            minute = 0;
        if(minute < 0)
            minute = 59;
        LCD_DrawFillRectangle(0,214,319,239,WHITE);
        LCD_DrawString(10,219,BLACK,BLACK,"--ADJUST TIME (MIN)--",16,0xff);
        sprintf(string,"%02d:%02d",hour,minute);
        LCD_DrawString(280,219,BLACK,BLACK,string,16,0xff);
    }

    if(tests & TEST_ENCODER) {
        if(!((GPIOA->IDR & 0x3)%3))
            printf("Clockwise\n");
        else
            printf("Counter-clockwise\n");
    }
}

void init_tim7(void) {
	//Set to 660Hz audio (toggle at 330Hz)
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 360 - 1;
    TIM7->ARR = 101 - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= 1 << TIM7_IRQn;
}

//==============================================================================
// Simulates a crummy sine wave
//==============================================================================
void TIM7_IRQHandler(void) {
	TIM7->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
	if(GPIOA->ODR & 0x20)
		GPIOA->ODR &= ~0x20;
	else
		GPIOA->ODR |=  0x20;
}
//==============================================================================
// TIM6_DAC_IRQHandler
//  * Samples all sensors @ 30Hz.
//  * Also has UART debugging.
//==============================================================================
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
    	EE_a = EE_IEEE(wgt);
        i = 0;
    }

    //Get the temperature
    tempF = get_temp();
    if(tests & TEST_TEMP)
    	printf("Temp: %d.%dF\n",tempF/10,tempF%10);

    //Get the time
    if(!(i%10)) {//Update @ 3Hz (no need to continually update it)
    	hour   = get_hour();
    	minute = get_minutes();
    }

    check_vitals();

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

//==============================================================================
// TIM2_IRQHandler
//  * Updates display once every 5 seconds.
//  * Wanted to continually update if changes detected, but sensors take so long
//    that LCD won't work without pausing timers, resetting the display, and
//    redrawing everything.
//==============================================================================
void TIM2_IRQHandler(void) {
	TIM6->CR1 &= ~TIM_CR1_CEN;
    TIM2->SR &= ~TIM_SR_UIF; //Acknowledge Interrupt
    if(minute != prev_minute | prev_spo2 != spo2 | prev_HR != HR | tempF != prev_tempF | prev_steps != steps | prev_EE_a != EE_a) {
    	while(SPI1->SR & SPI_SR_BSY);
    	LCD_Setup();
    	LCD_Clear(0x18e4);
		sprintf(string,"%02d:%02d",hour,minute);
		while(SPI1->SR & SPI_SR_BSY);
		LCD_DrawString(104,96,0xf924,BLACK,string,48,0xff);

		LCD_DrawFillRectangle(0, 0, 320, 6, 0xa65b);
		LCD_DrawFillRectangle(0, 0, 6, 240, 0xa65b);
		LCD_DrawFillRectangle(0, 234, 320, 240, 0xa65b);
		LCD_DrawFillRectangle(314, 0, 320, 240, 0xa65b);

		//Reset values at midnight
		if(midnight()) {
		    steps = 0;
		    EE_a  = 0;
		}
		sprintf(string,"SpO2");
		LCD_DrawString(10 + 112,10,WHITE,WHITE,string,16,0xff);
		sprintf(string,"HR");

		LCD_DrawString(10 + 180,10,WHITE,WHITE,string,16,0xff);
		if(spo2 != -1) {
			sprintf(string,"%-3d%%",spo2);
			LCD_DrawString(10 + 112,10 + 16,WHITE,WHITE,string,16,0xff);
		    sprintf(string,"%-3d",HR);
		    LCD_DrawString(10 + 180,10+16,WHITE,WHITE,string,16,0xff);
		} else {
		    LCD_DrawString(10 + 112,10+16,WHITE,WHITE,"N/A",16,0xff);
		    LCD_DrawString(10 + 180,10+16,WHITE,WHITE,"N/A",16,0xff);
		}

		sprintf(string,"TEMP");
		LCD_DrawString(10 + 235,10,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%d.%d^F",tempF/10,tempF%10);
		LCD_DrawString(10 + 235,10 + 16,WHITE,WHITE,string,16,0xff);

		sprintf(string,"STEPS");
		LCD_DrawString(10,10,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%-5d",steps);
		LCD_DrawString(10,10+16,WHITE,WHITE,string,16,0xff);

		sprintf(string,"EE");            //Truncate to Int
		LCD_DrawString(10 + 64,10,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%-4d",EE_a/100);
		LCD_DrawString(10 + 64,10 + 16,WHITE,WHITE,string,16,0xff);


		sprintf(string,"HEIGHT");
		LCD_DrawString(100,213,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%d'%2d\"",ft,inch);
		LCD_DrawString(100,197,WHITE,WHITE,string,16,0xff);

		sprintf(string,"WEIGHT",wgt);
		LCD_DrawString(10,213,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%-3dlbs",wgt);
		LCD_DrawString(10,197,WHITE,WHITE,string,16,0xff);

		sprintf(string,"Age");
		LCD_DrawString(200,213,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%-3d",age);
		LCD_DrawString(200,197,WHITE,WHITE,string,16,0xff);

		sprintf(string,"Sex");
		LCD_DrawString(280,213,WHITE,WHITE,string,16,0xff);
		sprintf(string,"%c",sex);
		LCD_DrawString(280,197,WHITE,WHITE,string,16,0xff);

		prev_minute = minute;
		prev_spo2   = spo2;
		prev_HR		= HR;
		prev_tempF  = tempF;
		prev_steps  = steps;
		prev_EE_a   = EE_a;

		//LCD_DrawString(0,239-32,WHITE,WHITE,"HELLO THERE",32,0xff);
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

void internal_clock()
{
    /* Disable HSE to allow use of the GPIOs */
    RCC->CR &= ~RCC_CR_HSEON;

    /* Enable Prefetch Buffer and set Flash Latency */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

    /* PCLK = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;

    /* PLL configuration = (HSI/2) * 12 = ~48 MHz */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLMULL12);

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)
    {
    }
}


//=============================================================================
// SAMPLE MAIN
//	Initializes all ICs.
//	Then, initialize the timer when everything is configured.
//=============================================================================
int main(void)
{
	//If a Power Cycle, give initial values
	//Otherwise, the "noinit" attribute will keep values the same with reset
	internal_clock();
	if(RCC->CSR & RCC_CSR_PORRSTF) {
		ft   = 5;
		inch = 11;
		wgt  = 125;
		age  = 20;
		sex  = 'M';
		RCC->CSR |= RCC_CSR_RMVF;
	}
	if(ft < 0 || ft>9)
		ft = 5;
	if(inch < 0 || inch > 11)
		inch = 11;
	if(wgt < 0 || wgt > 600)
		wgt = 125;
	if(age < 0 || age > 110)
		age = 20;
	if(sex != 'M' || sex != 'F')
		sex = 'M';

    LCD_Setup();
    LCD_Clear(BLACK);

	init_i2c();
	init_usart5();
	pulseox_setup();
	init_temp_sensor();
	init_accelerometer();
	init_exti();
	init_watch();

    init_tim6();
    init_tim2();
    init_tim7();
	while(1);
}
