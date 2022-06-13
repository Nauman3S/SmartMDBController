//pushButtonHandle.c

//THIS FILE USES PIN33 for input with interla pullup enabled
//PC4,PC5 pin 32,33
#include "pushButtonHandle.h"

void setupPB(uint8_t pin)
{
    if (pin == PIN34)
    {
        DDRC = 0b00000000;  //input
        PORTC = 0b11111111; //internal pullup
    }

    if (pin == PIN33)
    {
        DDRC = 0b00000000;  //input
        PORTC = 0b11111111; //internal pullup
    }

    if (pin == PIN32)
    {
        DDRC = 0b00000000;  //input
        PORTC = 0b11111111; //internal pullup
    }
}

uint8_t getPBState(uint8_t pin)
{

    if (pin == PIN33)
    {
        //send_str_p(UPLINK_USART,PSTR("Switching OFF TX Port\r\n"));
        if (!(PINC & (1 << PC5)))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else if (pin == PIN32)
    {
        //send_str_p(UPLINK_USART,PSTR("Switching OFF TX Port\r\n"));
        if (!(PINC & (1 << PC4)))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    else if (pin == PIN34)
    {
        //send_str_p(UPLINK_USART,PSTR("Switching OFF TX Port\r\n"));
        if (!(PINC & (1 << PC3)))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}