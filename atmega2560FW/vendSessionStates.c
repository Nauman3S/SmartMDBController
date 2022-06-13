//vendSessionStates
#include "vendSessionStates.h"

volatile uint8_t vendStateT = 0;

void saveVendState(uint8_t state)
{

    if (state == VEND_SUCCESS_FLAG)
    {
        writeDataToEEPROM(VEND_SUCCESS_FLAG);
        vendStateT = 1;
    }
    else if (state == VEND_FAILURE_FLAG)
    {
        writeDataToEEPROM(VEND_FAILURE_FLAG);
        vendStateT = 2;
    }
    else if (state == VEND_DEFAULT)
    {
        writeDataToEEPROM(VEND_DEFAULT);
        vendStateT = 0;
    }
}
uint8_t getLastVendState()
{
    //return vendStateT;
    return readDataFromEEPROM();
}
void clearLastVendState()
{
    writeDataToEEPROM(VEND_DEFAULT);
    vendStateT = 0;
}

void statesCheckStartup()
{
    //writeDataToEEPROM();

    vendStateT = 0; //getLastVendState();

    if (vendStateT == 0)
    {
        send_str_p(0, PSTR("LAST state was 0\r\n"));
    }
    else if (vendStateT == 1)
    {

        send_str_p(UPLINK_USART, PSTR("VEND SESSION COMPLETE\r\n"));
    }
    else if (vendStateT == 2)
    {

        send_str_p(0, PSTR("LAST state was Failed\r\n"));
    }
}