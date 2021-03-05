#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "io.h"

void io_init()
{
    DDRB  = 0b00111111;
    PORTB = 0b00000000;
    DDRC  = 0b00000000;
    PORTC = 0b00111000;
    DDRD  = 0b11111111;
    PORTD = 0b11111111;
}

// flash the apostrophe. useful for "is this code being reached?"
void blip()
{
    cli();
    DIGITS_OFF();
    DIGIT_VALUE(~1);
    DIGIT_ON(4);
    _delay_ms(1);
    sei();
}
