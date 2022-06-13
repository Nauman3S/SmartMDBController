#include "EEPROMHandle.h"

uint8_t writeDataToEEPROM(uint8_t ByteOfData)
{
    eeprom_update_byte((uint8_t *)64, ByteOfData);
}

uint8_t readDataFromEEPROM()
{
    uint8_t value;
    value = eeprom_read_byte((const uint8_t *)64);

    return value;
}