/*****************************************************************************
 * This code is just an example for using the uart.h library.                *
 * All you must do is include uart.h, call init_usart5, and then just print  *
 * anything to the serial port assuming all wiring is functional.            *
 *****************************************************************************/
#include "stm32f0xx.h"
#include "uart.h"

int main(void) {
    init_usart5();            //FIRST CALL init_usart5();
    printf("Hello there!\n"); //THEN any printf command sends stuff to the screen.

    int num = 4;                                 //You can also use %d to show integer variables
    printf("May the %dth be with you...\n",num); //Does not work with floats (might fix later)
    while(1);
}
