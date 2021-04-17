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

uint16_t sample_adc()
{
    uint16_t sam = 0;
    if (ADCSRA & (1 << ADIF)) {
        sam = ADCW;
        ADCSRA |= (1 << ADIF);
    }
    ADCSRA |= (1 << ADSC);
    return sam;
}

void display_power_meter()
{
    uint16_t sam = sample_adc();
    if (sam) {
        // VCC = 1.1V * 1024 / adc; we will retrieve hundredths here
        uint16_t cv = (1024L * 110) / sam;
        // since the AVR's voltage range is 1.8 ... 5.5, I'm not going to worry about >= 10 V
        Display3(cv, LETTER_v, 0, 0);
    }
}

void init_temp_sensor()
{
    // 1.1v reference, temperature sensor input
    ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
}

void display_temp_sensor()
{
    // it turns out there *aren't* factory calibration values stored in the signature row
    // so I will just display the raw sample for now, as the datasheet examples are way off
    // I will need two readings at widely separated temperatures to compute the slope and y-intercept
    uint16_t sam = sample_adc();
    if (sam) {
        Display3(sam, EMPTY, 99, 1);
    }
}
