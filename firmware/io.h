#pragma once

#include <avr/io.h>

void io_init();

// Port assignments:
// PORTB0..3 (output) = Digit anode drivers
// PORTB4    (output) = colon / apostrophe anodes
// PORTB5    (output) = Optoisolator to camera
// PORTB6..7          = 32.768kHz watch crystal
// PORTC0    (input)  = Encoder clock
// PORTC1    (input)  = Encoder data
// PORTC2    (input)  = Encoder key
// PORTC3    (input)  = Select key
// PORTC4    (input)  = Set key
// PORTC5    (input)  = Start key
// PORTD0..7 (output) = Segment cathodes (PD7 = A, PD6 = B, ... PD0 = DP)


// for portability, please put all explicit port references here and init_io()
#define DIGITS_OFF()   PORTB &= 0b11100000;
#define DIGIT_ON(x)    PORTB |= (1 << x)

#define SHUTTER_OFF()  PORTB &= ~(1 << PB5)
#define SHUTTER_ON()   PORTB |= (1 << PB5)

#define DIGIT_VALUE(x) PORTD = x

#define BUTTON_STATE() ((PINC & 0b111100) >> 2)

#define ENCODER_CLOCK() (PINC & 0b00000001)
#define ENCODER_DATA()  (PINC & 0b00000010)

// one-bit printf debugging...
void blip();
