//#define TEST_DISPLAY
#ifdef TEST_DISPLAY
#include <util/delay.h>
#endif

// 0 = on since we're using a common anode display
const uint8_t digits[10] PROGMEM = {
    0b00000011, // 0
    0b10011111, // 1
    0b00100101, // 2
    0b00001101, // 3
    0b10011001, // 4
    0b01001001, // 5
    0b11000001, // 6
    0b00011111, // 7
    0b00000001, // 8
    0b00011001, // 9
};
#define DECIMAL_POINT 1
#define LETTER_C 0b01100011
#define LETTER_L 0b11100011
#define LETTER_B 0b11000001
#define LETTER_S 0b01001001
#define LETTER_A 0b00010001
#define LETTER_V 0b10000011
#define LETTER_E 0b01100001

static const uint8_t brighttable[4] PROGMEM = { 255, 85, 28, 9 };

void init_display()
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

    // prescaler 1/8; at 1MHz system clock, this gives us an overflow
    // at 488 Hz, providing a per-digit refresh rate of 97.6 Hz.
    TCCR0A = 0;
    TCCR0B = (1<<CS01);

    // Output compare value B - controls blanking.
    // In full brightness mode, we'll make this happen immediately before the refresh,
    // In lower brightness modes, we'll make it happen sooner.
    OCR0A = pgm_read_byte(&brighttable[bright]);

    // Enable overflow and compare match interrupts
    TIMSK0 = (1<<TOIE0) | (1<<OCIE0A);
}

// Refresh interrupt - refreshes the next digit on the display.
// By drawing each in turn quickly enough, we give the illusion of
// a solid display, but without requiring the output ports and wiring
// to drive each digit independently.

volatile uint8_t display[5] = { '\xff', '\xff', '\xff', '\xff', '\xff' };
ISR(TIMER0_OVF_vect)
{
    static uint8_t didx = 0;
    DIGIT_VALUE(display[didx]);
    DIGIT_ON(didx);
    if (++didx == 5)
        didx = 0;
}

// Blanking interrupt - clears the display prior to the next refresh.
// We need to turn the digits off before switching segments to
// prevent ghosting caused by the wrong value being briefly displayed.
// By changing the value of OCR0A, we can control the effective
// brightness of the display.

ISR(TIMER0_COMPA_vect)
{
    DIGITS_OFF();
}



static void DisplayAlnum(char letter, uint8_t num, uint8_t blink_mask, uint8_t dp)
{
    display[0] = letter ^ ((dp & 8) ? DECIMAL_POINT : 0);
    display[1] = EMPTY ^ ((dp & 4) ? DECIMAL_POINT : 0);
    display[4] = EMPTY;
    DisplayNum(num, LOW_POS, blink_mask, (blink_mask == 0) ? 1 : 0, dp);
}



#define HIGH_POS 0
#define LOW_POS  2
#define EMPTY '\xFF'
#define EXTRA_POS 4
#define COLON 0b01111111
#define APOS 0b11111110

// strip - bit 0 = don't display tens place if num < 10
//         bit 1 = ...           ones place if num == 0
// dp = bit 0 = low digit decimal point on; bit 1 = high digit decimal point on
static void DisplayNum(uint8_t num, uint8_t pos, uint8_t blink_mask, uint8_t strip, uint8_t dp)
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

void display_set_brightness(uint8_t bright)
{
    OCR0A = pgm_read_byte(&brighttable[bright]);
}

void display_blink_reset()
{
    TCNT2 = 0;
}
