//Main File

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
//1 second=15999999
//1/2 second=7999999
//1/400 seconds=3 99 99
#include <avr/io.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
//#include <util/delay.h>
#include <stdint.h>
#include "txSwitch.h"
//#include "pushButtonHandle.h"

#include "usart.h"
#include "mdb.h"
#include "uplink.h"
#include "millisF.h"
//#include "EEPROMHandle.h"
#include "vendSessionStates.h"

#define millisTimer 1
#define waitForSec 0

#define onOffRelay 0
#define simulation 0
#if simulation == 1
uint8_t simStartFlag = 0;
uint8_t vendApproveFlag = 0;
#endif
uint8_t flagger = 0;
int kVal = 0;
uint8_t bootCounter = 0;
uint8_t fl = 0;

// uint8_t writeDataToEEPROM();
// uint8_t readDataFromEEPROM();

int main(void)
{

    // millis_init();

    setup_usart(0, 38400, 8, 'N', 1);
    setup_usart(1, 9600, 9, 'N', 1); //orignal 9600
    setup_usart(2, 9600, 8, 'N', 1);

    millis_init();
    setupTxSwitch(PIN12);
    setupTxSwitch(PIN13);

    txSwitchState(PIN13, 0); //was 1 initially
    txSwitchState(PIN12, 1);

#if waitForSec == 1

#endif
    //_delay_ms(5000);
    sei();

#if simulation == 1

#endif

    send_str_p(0, PSTR("MDB Arduino Mega is Setting Up\r\n"));

    while (1)
    {

        mdb_cmd_handler();
        uplink_cmd_handler();

#if millisTimer == 1
        if (getTimerState())
        {
            static millis_t lastChanged = 0;
            static millis_t lastChanged2 = 0;
            millis_t now = millis_get();

            //if(kVal>=0 && kVal<=50){
            if (now - lastChanged >= 1000)
            {

                send_str_p(0, PSTR("5s Passed\r\n"));
                //incrementTimerPass();
                timeHandler(getTimerPass());

                lastChanged = now;
            }
        }

#endif
#if simulation == 1
        //////////////////////for simulation

        if (getPBState(PIN33) == 1)
        {
            if (simStartFlag == 0)
            {
                char valueM[20];
                sprintf(valueM, "50");
                cmd_get_mdb_state(valueM);
                cmd_start_session(valueM);
                cmd_get_mdb_state(valueM);
                simStartFlag = 1;
            }
        }

        if (getPBState(PIN32) == 1)
        {
            if (vendApproveFlag == 0)
            {

                char valueM0[20];
                sprintf(valueM0, "50");
                cmd_get_mdb_state(valueM0);
                cmd_approve_vend(valueM0);
                cmd_get_mdb_state(valueM0);
                vendApproveFlag = 1;
            }
        }

//////////////////////for simulation
#endif
    }

    return 0;
}
