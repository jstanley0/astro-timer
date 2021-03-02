#include <avr/io.h>

void init_io()
{
    DDRB  = 0b00111111;
    PORTB = 0b00000000;
    DDRC  = 0b00000000;
    PORTC = 0b00111000;
    DDRD  = 0b11111111;
    PORTD = 0b11111111;
}
