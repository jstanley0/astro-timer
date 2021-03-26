#include <avr/eeprom.h>
#include "settings.h"

uint8_t stime[2] = { 0, 0 };
uint8_t delay[2] = { 0, 0 };
uint8_t count    = 1;
uint8_t mlu      = 0;
uint8_t bright   = 1;

inline void savebyte(uint16_t addr, uint8_t value)
{
    eeprom_write_byte((uint8_t *)addr, value);
}

uint8_t loadbyte(uint16_t addr, uint8_t default_value, uint8_t max_value)
{
    uint8_t ret = eeprom_read_byte((uint8_t *) addr);
    if (ret > max_value)
        ret = default_value;
    return ret;
}

void Save()
{
    savebyte(0, stime[0]);
    savebyte(1, stime[1]);
    savebyte(2, delay[0]);
    savebyte(3, delay[1]);
    savebyte(4, count);
    savebyte(5, mlu);
    savebyte(6, bright);
}

void Load()
{
    stime[0] = loadbyte(0, 3, 99);
    stime[1] = loadbyte(1, 0, 59);
    delay[0] = loadbyte(2, 0, 99);
    delay[1] = loadbyte(3, 5, 59);
    count    = loadbyte(4, 10, 99);
    mlu      = loadbyte(5, 0, 99);
    bright   = loadbyte(6, 2, 3);
}
