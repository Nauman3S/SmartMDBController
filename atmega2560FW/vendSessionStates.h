//vendSessionStates
#include <avr/io.h>
#include "EEPROMHandle.h"

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "usart.h"

#define VEND_DEFAULT 0
#define VEND_SUCCESS_FLAG 1
#define VEND_FAILURE_FLAG 2

extern volatile uint8_t vendStateT;

void saveVendState(uint8_t state);
uint8_t getLastVendState();
void clearLastVendState();
void statesCheckStartup();