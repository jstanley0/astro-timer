#pragma once

#include <avr/io.h>

void sysclk_init();

void io_init();

// Port assignments:
// PORTB0..3 (output) = Digit anode drivers
// PORTB4    (output) = colon / apostrophe anodes
// PORTB5    (output) = Camera shutter output (full-press)
// PORTB6..7          = 32.768kHz watch crystal
// PORTC0    (input)  = Encoder clock
// PORTC1    (input)  = Encoder data
// PORTC2    (input)  = Start (encoder) key
// PORTC3    (input)  = Select key
// PORTC4    (input)  = Set key
// PORTC5    (output) = Camera shutter output (half-press)
// PORTD0..7 (output) = Segment cathodes (PD7 = A, PD6 = B, ... PD0 = DP)

// undefine when using a discrete encoder (vs breakout board with integrated pullup resistors)
//#define EXTERNAL_ENCODER_PULLUPS

// for portability, please put all explicit port references here and init_io()
#define DIGITS_OFF()   PORTB &= 0b11100000;
#define DIGIT_ON(x)    PORTB |= (1 << x)

#define SHUTTER_OFF()  PORTB &= ~(1 << PB5)
#define SHUTTER_ON()   PORTB |= (1 << PB5)

#define SHUTTER_HALFPRESS_OFF()  PORTC &= ~(1 << PC5)
#define SHUTTER_HALFPRESS_ON()   PORTC |= (1 << PC5)

#define DIGIT_VALUE(x) PORTD = x

#define BUTTON_STATE() ((PINC & 0b11100) >> 2)

// note: in contravention of my comment above, input.c uses an explicit pin-change interrupt vector :P

// one-bit printf debugging...
void blip();

// shut down unnecessary peripherals to save power
void power_init();

// let the user know we're turning off
void acknowledge_power_off();

// to save power if the device is left idle too long
void power_down();
