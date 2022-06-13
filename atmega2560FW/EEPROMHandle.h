//#include <avr/io.h>
#include <inttypes.h>
#include <avr/eeprom.h>
#include "uplink.h"
#include "usart.h"

uint8_t writeDataToEEPROM( uint8_t ByteOfData);
uint8_t readDataFromEEPROM();