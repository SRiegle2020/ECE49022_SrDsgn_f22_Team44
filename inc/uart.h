/*****************************************************************************
 * This header file gives a list of the two functions used for the sending   *
 * data to the terminal. The only one that is important is init_usart5().    *
 *****************************************************************************/
#include "stm32f0xx.h"
#include <stdio.h>

void init_usart5(void);  //Initializes the USART to send data to terminal
int __io_putchar(int c); //Helper function for printf; not called by user
