#include "io.h"
#include "display.h"

void turn_adc_on()
{
    PRR &= ~(1 << PRADC);
    // enable ADC, 1/32 prescaler, 62.5kHz
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS0);
}

void turn_adc_off()
{
    ADCSRA &= ~(1 << ADEN);
    PRR |= (1 << PRADC);
}

void init_power_meter()
{
    // AVCC reference, 1.1V input
    ADMUX = (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);
}

void display_power_meter()
{
    if (ADCSRA & (1 << ADIF)) {
        // VCC = 1.1V * 1024 / adc; we will retrieve hundredths here
        uint16_t cv = (1024L * 110) / ADCW;
        // since the AVR's voltage range is 1.8 ... 5.5, I'm not going to worry about >= 10 V
        Display3(cv, LETTER_v, 0, 0);

        ADCSRA |= (1 << ADIF);
    }
    ADCSRA |= (1 << ADSC);
}

void init_temp_sensor()
{
    // 1.1v reference, temperature sensor input
    ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
}

void display_temp_sensor(int8_t useF)
{
    // TODO figure out how to read the real values from the factory calibration
    static const uint8_t TS_OFFSET = 21;//boot_signature_byte_get(2);
    static const uint8_t TS_GAIN = 164;//boot_signature_byte_get(3);
    if (ADCSRA & (1 << ADIF)) {
        int16_t deg;
        if (useF) {
            deg = (9 * (((long)ADCW - (273 + 100 - TS_OFFSET)) * 128)) / (5 * TS_GAIN) + 45 + 32;
        } else {
            deg = (((long)ADCW - (273 + 100 - TS_OFFSET)) * 128) / TS_GAIN + 25;
        }
        Display3(deg, useF ? LETTER_F : LETTER_C, 99, 1);

        ADCSRA |= (1 << ADIF);
    }
    ADCSRA |= (1 << ADSC);
}
