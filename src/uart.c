/*****************************************************************************
 * This code contains functions for sending data to the serial port for      *
 * debugging without using the TFT display. Useful for sensors-to-MCU and    *
 * watch and accelerometer data subsystems.                                  *
 *****************************************************************************/
#include "stm32f0xx.h"
#include <stdio.h>
#include "uart.h"

//============================================================================
// INIT_USART5
//  * Configures PC12 to transmit to USART5
//  * Sets buffering for stdin, stdout, and stderr to 0
//============================================================================
void init_usart5(void) {
    //Enable the Clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN; //Enable C,D

    //Set PC12 to AltFn
    GPIOC->MODER &= ~0x3000000;
    GPIOC->MODER |=  0x2000000;

    //Set PC12 to USART5_TX
    GPIOC->AFR[1] &= ~0xf0000;
    GPIOC->AFR[1] |=  0x20000;

    //Clk the USART
    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;

    //Disable and Configure USART5
    USART5->CR1 &= ~USART_CR1_UE;
    USART5->CR1 &= ~(0x10001000);                           //8Bit Word
    USART5->CR2 &= ~(USART_CR2_STOP_1 | USART_CR2_STOP_0);  //1Bit Stop
    USART5->CR1 &= ~USART_CR1_PCE;                          //No Parity
    USART5->CR1 &= ~USART_CR1_OVER8;                        //16 Over
    USART5->BRR &= ~0xffff;                                 //Baud 115.2kBps
    USART5->BRR |=  0x01a1;
    USART5->CR1 |=  USART_CR1_TE;                           //Enable Tx/Rx
    USART5->CR1 |=  USART_CR1_UE;                           //Enable USART

    //Wait until ready
    while((USART5->ISR & 0x200000) == 0);

    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
}

//============================================================================
// __io_putchar
//  * Helper function to allow printf to send characters to the terminal
//============================================================================
int __io_putchar(int c) {
    while(!(USART5->ISR & USART_ISR_TXE)) {}
    if(c == '\n')
        USART5->TDR = '\r';
        while(!(USART5->ISR & USART_ISR_TXE)) {}
    USART5->TDR = c;
    return c;
}
