#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#ifdef TEST_DISPLAY
#include <util/delay.h>
#endif

#include "io.h"
#include "display.h"
#include "settings.h"

// 0 = on since we're using a common anode display
const uint8_t digits[16] PROGMEM = {
    0b00000011, // 0
    0b10011111, // 1
    0b00100101, // 2
    0b00001101, // 3
    0b10011001, // 4
    0b01001001, // 5
    0b01000001, // 6
    0b00011111, // 7
    0b00000001, // 8
    0b00001001, // 9
    0b00010001, // A
    0b11000001, // B
    0b01100011, // C
    0b10000101, // D
    0b01100001, // E
    0b01110001  // F
};

void display_init()
{
#ifdef TEST_DISPLAY
    for(uint8_t digit = 1; digit != 0b100000; digit <<= 1) {
        PORTB &= 0b11100000;
        PORTB |= digit;
        for(uint8_t segment = 1; segment != 0; segment <<= 1) {
            PORTD = ~segment;
            _delay_ms(100);
        }
    }
#endif

    TCCR0A = (1<<WGM01);             // CTC mode
    TCCR0B = (1<<CS01) | (1<<CS00);  // 1/64 prescaler

    // Output compare value A - refreshes a digit.
    // At 2MHz clock, 1/64 prescaler, timer ticks happen at 31kHz, so reset the timer after 64 ticks
    // for interrupts at 488Hz and a per-digit refresh rate of 98Hz.
    OCR0A = 64;

    // Note we use a compare-match instead of overflow here because this needs to be
    // a higher priority interrupt than the blanking one; otherwise, if the CPU is busy
    // and both interrupt flags are set before either vector is executed, they'll run
    // in the wrong order, resulting in visual glitches such as very bright sparkling digits
    // in the lowest brightness mode.

    // Output compare value B - controls blanking.
    // In full brightness mode, we'll make this happen immediately before the refresh,
    // In lower brightness modes, we'll make it happen sooner.
    OCR0B = 63 >> bright;

    // Enable compare match A and B interrupts
    TIMSK0 = (1<<OCIE0A) | (1<<OCIE0B);
}

volatile uint8_t display[5] = { '\xff', '\xff', '\xff', '\xff', '\xff' };

// Refresh interrupt - refreshes the next digit on the display.
// By drawing each in turn quickly enough, we give the illusion of
// a solid display, but without requiring the output ports and wiring
// to drive each digit independently.
ISR(TIMER0_COMPA_vect)
{
    static uint8_t didx = 0;
    DIGIT_VALUE(display[didx]);
    DIGIT_ON(didx);
    if (++didx == 5)
        didx = 0;
}

volatile uint8_t OCR0B_buf = 0;

// Blanking interrupt - clears the display prior to the next refresh.
// We need to turn the digits off before switching segments to
// prevent ghosting caused by the wrong value being briefly displayed.
// By changing the value of OCR0B, we can control the effective
// brightness of the display.
ISR(TIMER0_COMPB_vect)
{
    DIGITS_OFF();

    // buffer updates to OCR0B to prevent glitches while changing brightness
    if (OCR0B_buf) {
        OCR0B = OCR0B_buf;
        OCR0B_buf = 0;
    }
}

void IntToDigs2(int n, uint8_t digs[2])
{
    digs[0] = 0;
    while(n >= 10)
    {
        n -= 10;
        ++digs[0];
    }

    digs[1] = 0;
    while(n >= 1)
    {
        n -= 1;
        ++digs[1];
    }
}

void IntToDigs3(uint16_t n, uint8_t digs[3])
{
    digs[0] = 0;
    while(n >= 100)
    {
        n -= 100;
        ++digs[0];
    }

    IntToDigs2(n, &digs[1]);
}

#define DECIMAL_POINT 1

void DisplayAlnum(char letter, uint8_t num, uint8_t blink_mask, uint8_t dp)
{
    display[0] = letter ^ ((dp & 8) ? DECIMAL_POINT : 0);
    display[1] = EMPTY ^ ((dp & 4) ? DECIMAL_POINT : 0);
    display[4] = EMPTY;
    DisplayNum(num, LOW_POS, blink_mask, (blink_mask == 0) ? 1 : 0, dp);
}

void DisplayNum(uint8_t num, uint8_t pos, uint8_t blink_mask, uint8_t strip, uint8_t dp)
{
    uint8_t digs[2];

    if (TCNT2 & blink_mask) {
        display[pos] = EMPTY;
        display[pos + 1] = EMPTY;
    } else {
        IntToDigs2(num, digs);
        display[pos] = ((strip & 1) && (num < 10)) ? EMPTY
            : pgm_read_byte(&digits[digs[0]]);
        if (dp & 2) display[pos] ^= DECIMAL_POINT;
        display[pos + 1] = ((strip & 2) && (num == 0)) ? EMPTY
            : pgm_read_byte(&digits[digs[1]]);
        if (dp & 1) display[pos + 1] ^= DECIMAL_POINT;
    }
}

void Display3(int16_t num, uint8_t letter, uint8_t dp_pos, uint8_t degree)
{
    uint8_t digs[3];
    if (num < 0) {
        IntToDigs2(-num, &digs[0]);
        display[0] = MINUS_SIGN;
        display[1] = pgm_read_byte(&digits[digs[0]]) ^ ((dp_pos == 0) ? DECIMAL_POINT : 0);
        display[2] = pgm_read_byte(&digits[digs[1]]) ^ ((dp_pos == 1) ? DECIMAL_POINT : 0);
    } else {
        IntToDigs3(num, &digs[0]);
        for(uint8_t p = 0; p < 3; ++p) {
            display[p] = pgm_read_byte(&digits[digs[p]]) ^ ((dp_pos == p) ? DECIMAL_POINT : 0);
        }
    }
    display[3] = letter;
    display[4] = degree ? APOS : EMPTY;
}

void DisplayHex(uint8_t num, uint8_t pos)
{
    display[pos] = pgm_read_byte(&digits[num >> 4]);
    display[pos + 1] = pgm_read_byte(&digits[num & 0xF]);
}

void display_set_brightness(uint8_t bright)
{
    OCR0B_buf = 63 >> bright;
}

void display_spin()
{
    static uint8_t bit = 0b10000000;
    display[0] = display[1] = display[2] = display[3] = ~bit;
    bit >>= 1;
    if (bit == 0b10)
        bit = 0b10000000;
}
