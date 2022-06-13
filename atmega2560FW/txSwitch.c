

#include "txSwitch.h"

void setupTxSwitch(uint8_t pin)

{
  // DDRB = 0b01000000;
  if (pin == PIN12)
  {
    DDRB = 0xFF;
    PORTB = 0x00;
  }
  if (pin == PIN13)
  {
  }

  send_str_p(UPLINK_USART, PSTR("SettingUp Tx Switch\r\n"));
}
void txSwitchState(uint8_t pin, uint8_t val)
{

  if (pin == PIN12 && val == 0)
  {
    //send_str_p(UPLINK_USART,PSTR("Switching OFF TX Port\r\n"));

    PORTB = PIN12_OFF;
  }
  else if (pin == PIN12 && val == 1)
  {
    //  send_str_p(UPLINK_USART,PSTR("Switching ON TX Port\r\n"));
    PORTB = PIN12_ON;
  }

  else if (pin == PIN13 && val == 0)
  {
    //send_str_p(UPLINK_USART,PSTR("Switching OFF TX Port\r\n"));

    PORTB = PIN13_OFF;
  }
  else if (pin == PIN13 && val == 1)
  {
    //  send_str_p(UPLINK_USART,PSTR("Switching ON TX Port\r\n"));
    PORTB = PIN13_ON;
  }
}