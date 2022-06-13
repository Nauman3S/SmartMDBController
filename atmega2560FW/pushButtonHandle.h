//pushButtonHandle.h
#include <avr/io.h>
#include <inttypes.h>

#define PIN32 32
#define PIN33 33
#define PIN34 34

uint8_t getPBState(uint8_t pin);
void setupPB(uint8_t pin);
