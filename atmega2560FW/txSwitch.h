
#include <avr/io.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "usart.h"
#include "uplink.h"
#include <stdlib.h>
#include <stdio.h>



// #define PIN12_ON  (1<<PB6)
// #define PIN12_OFF (1>>PB6)

#define PIN12_ON  (1<<PINB6)
#define PIN12_OFF (1>>PINB6)


#define PIN13_ON  (1<<PB7)
#define PIN13_OFF (1>>PB7)
#define PIN12 12
#define PIN13 13


void txSwitchState(uint8_t pin, uint8_t val);
void setupTxSwitch(uint8_t pin);