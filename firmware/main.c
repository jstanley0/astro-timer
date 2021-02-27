// Exposure timer MkII
// based on AVR ATMEGA48 running at 1MHz (default fuses okay)
// build with avr-gcc
//
// Source code (c) 2007 by Jeremy Stanley
// http://www.xmission.com/~jstanley/avrtimer.html
// Licensed under GNU GPL v2 or later
//

// Port assignments:
// PORTB0..3 (output) = Digit anode drivers
// PORTB4    (output) = colon / apostrophe anodes
// PORTB5    (output) = Optoisolator to camera
// PORTB6..7          = 32.768kHz watch crystal
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

#define BUTTON_STATE() ((PINC & 0b111000) >> 3)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

//#define TEST_DISPLAY
#ifdef TEST_DISPLAY
#include <util/delay.h>
#endif

void init_io()
{
    DDRB  = 0b00111111;
    PORTB = 0b00000000;
    DDRC  = 0b00000000;
    PORTC = 0b00111000;
    DDRD  = 0b11111111;
    PORTD = 0b11111111;

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
}

// Put the processor in idle mode for the specified number of "kiloclocks"
// (= periods of 1024 clock cycles)

volatile uint8_t wakeup;
ISR(TIMER1_COMPA_vect)
{
    wakeup = 1;
}

void Sleep(uint16_t kiloclocks)
{
    TCCR1A = 0;
    TCCR1B = 0;                // stop the timer
    TIFR1 = (1 << OCF1A);    // clear output-compare A flag
    OCR1A = kiloclocks;        // set compare match A target
    TCNT1 = 0;                // reset timer counter
    TIMSK1 = (1 << OCIE1A);    // enable compare match A interrupt
    TCCR1B = (1 << CS12) | (1 << CS10);    // start timer with 1/1024 prescaler

    // sleep until it's time to wake up
    // use a loop here because other interrupts will happen
    wakeup = 0;
    set_sleep_mode(SLEEP_MODE_IDLE);
    do {
        sleep_mode();
    } while( !wakeup );

    TIMSK1 = 0;                // stop the interrupt
    TCCR1B = 0;                // stop the timer
}

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

static void clock_start() {
    TCNT2 = 0;
    TIFR2 = (1 << TOV2);
    TIMSK2 |= (1 << TOIE2);
}

static void clock_stop() {
    TIMSK2 &= (uint8_t)~(1 << TOIE2);
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




// Timer interrupt service routine
// Executes once per second, driven by the 32.768khz xtal

volatile int8_t gMin, gSec;
volatile int8_t gDirection = -1;

ISR(TIMER2_OVF_vect)
{
    if (gDirection > 0) {
        // counting up...
        if (++gSec == 60) {
            gSec = 0;
            if (++gMin == 100) {
                gMin = 0;
            }
        }
    }
    else if (gDirection < 0) {
        // counting down...
        if (gSec == 0) {
            if (gMin > 0) {
                gSec = 59;
                gMin--;
            } else {
                // the down-timer started at 0
                // (we just finished a sub-second delay)
                gDirection = 0;
            }
        } else {
            if (--gSec == 0 && gMin == 0) {
                // time has elapsed.
                gDirection = 0;
            }
        }
    }
}

void IntToDigs2(int n, uint8_t digs[4])
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

/*
void IntToDigs4(int n, uint8_t digs[4])
{
    digs[0] = 0;
    while(n >= 1000)
    {
        n -= 1000;
        ++digs[0];
    }

    digs[1] = 0;
    while(n >= 100)
    {
        n -= 100;
        ++digs[1];
    }

    IntToDigs2(n, &digs[2]);
}
*/


// here's how button presses work:
// - a press is registered when a button is released.
// - a hold is registered when the same button has been down
//   for a specified number of cycles.  the button release
//   following the hold does not register.

#define BUTTON_SET    0x01
#define BUTTON_SELECT  0x02
#define BUTTON_START 0x04
#define BUTTON_HOLD  0x10     // button was held

#define REPEAT_THRESHOLD 15

uint8_t GetButtons()
{
    static uint8_t prevState = 0xff;
    static uint8_t repeat = 0;

    uint8_t curState = BUTTON_STATE();

    // if we've already registered a "hold"
    if (repeat >= REPEAT_THRESHOLD) {
        prevState = curState;
        if (curState == 7)
            repeat = 0;    // no buttons are down.
        return 0;
    }

    if (curState != prevState) {
        uint8_t pressed = ~prevState & curState;
        prevState = curState;
        return pressed;
    } else if (curState != 7) {
        // button(s) are being held
        if (++repeat == REPEAT_THRESHOLD) {
            return BUTTON_HOLD | ~(curState & 7);
        }
    }

    return 0;
}

static const uint8_t brighttable[4] PROGMEM = { 255, 85, 28, 9 };


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

static unsigned char EditNum(uint8_t *num, uint8_t buttons, uint8_t max)
{
    if (buttons == 0)
        return 0;

    TCNT2 = 0;    // reset blinkage when a key is pressed

    if (buttons & BUTTON_SELECT) {
        if (buttons & BUTTON_HOLD)
            *num = 0;
        else
            *num += 10;
    }
    if (buttons & BUTTON_START) {
        (*num)++;
    }
    if (*num >= max)
        *num = 0;

    return (buttons & BUTTON_SET);
}

static void DisplayAlnum(char letter, uint8_t num, uint8_t blink_mask, uint8_t dp)
{
    display[0] = letter ^ ((dp & 8) ? DECIMAL_POINT : 0);
    display[1] = EMPTY ^ ((dp & 4) ? DECIMAL_POINT : 0);
    display[4] = EMPTY;
    DisplayNum(num, LOW_POS, blink_mask, (blink_mask == 0) ? 1 : 0, dp);
}

enum State {
    // main exposure menu
    ST_TIME, ST_DELAY, ST_COUNT, ST_MLU,
    // options
    ST_BRIGHT, ST_SAVED,
    // edit states
    ST_TIME_SET_MINS, ST_TIME_SET_SECS,
    ST_DELAY_SET_MINS, ST_DELAY_SET_SECS,
    ST_COUNT_SET, ST_MLU_SET,
    // run states
    ST_RUN_PRIME, ST_RUN_MANUAL,
    ST_MLU_PRIME, ST_MLU_WAIT,
    ST_RUN_AUTO, ST_WAIT
};

// params
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
    stime[0] = loadbyte(0, 0, 99);
    stime[1] = loadbyte(1, 0, 59);
    delay[0] = loadbyte(2, 0, 99);
    delay[1] = loadbyte(3, 0, 59);
    count    = loadbyte(4, 1, 99);
    mlu      = loadbyte(5, 0, 99);
    bright   = loadbyte(6, 1, 3);
}

void InitRun(enum State *state)
{
    gMin = stime[0];
    gSec = stime[1];

    if (gMin > 0 || gSec > 0)
    {
        // count down
        gDirection = -1;
        *state = ST_RUN_AUTO;
    }
    else
    {
        // count up
        gDirection = 1;
        *state = ST_RUN_MANUAL;
    }

    // open the shutter and start the clock
    SHUTTER_ON();
    clock_start();
}


int main(void)
{
    // Initialize I/O
    init_io();

    // Load saved state, if any
    Load();

    // Setup the display timer...

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


    // Setup the RTC...
    gMin = 0;
    gSec = 0;

    // select asynchronous operation of Timer2
    ASSR = (1<<AS2);

    // select prescaler: 32.768 kHz / 128 = 1 sec between each overflow
    TCCR2A = 0;
    TCCR2B = (1<<CS22) | (1<<CS20);

    // wait for TCN2UB and TCR2UB to be cleared
    while((ASSR & 0x01) | (ASSR & 0x04));

    // clear interrupt-flags
    TIFR2 = 0xFF;


    // init the state machine
    enum State state = ST_TIME;
    enum State prevstate = ST_TIME;
    uint8_t remaining = 0;
    uint8_t cmode = 0;

    // Enable interrupts
    sei();

    // Do some stuff
    for(;;)
    {
        uint8_t buttons = GetButtons();

        if ((buttons & BUTTON_START) && (state < ST_BRIGHT)) {
            prevstate = state;
            remaining = count;
            cmode = 0;
            buttons = 0;
            state = ST_RUN_PRIME;
        }

    newstate:
        switch(state)
        {
        case ST_TIME:
            DisplayNum(stime[0], HIGH_POS, 0, 3, 0);
            display[EXTRA_POS] = COLON;
            DisplayNum(stime[1], LOW_POS, 0, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_DELAY;
            } else if (buttons & BUTTON_SET) {
                state = ST_TIME_SET_MINS;
            }
            break;
        case ST_DELAY:
            DisplayNum(delay[0], HIGH_POS, 0, 3, 1);
            display[EXTRA_POS] = EMPTY;
            DisplayNum(delay[1], LOW_POS, 0, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_COUNT;
            } else if (buttons & BUTTON_SET) {
                state = ST_DELAY_SET_MINS;
            }
            break;
        case ST_COUNT:
            DisplayAlnum(LETTER_C, count, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_MLU;
            } else if (buttons & BUTTON_SET) {
                state = ST_COUNT_SET;
            }
            break;
        case ST_MLU:
            DisplayAlnum(LETTER_L, mlu, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_BRIGHT;
            } else if (buttons & BUTTON_SET) {
                state = ST_MLU_SET;
            }
            break;
/* I'll put this in once there's more than one option ;)
        case ST_OPTIONS:
            display[0] = '\xC0';    // 00111111 = 'O'
            display[1] = '\x8C';    // 01110011 = 'P'
            display[2] = EMPTY;
            display[3] = '0x87';    // 01111000 = 't'
            display[4] = '0x92';    // 01101101 = 'S'
            if (buttons & BUTTON_SELECT) {
                state = ST_TIME;
            } else if (buttons & BUTTON_SET) {
                state = ST_BRIGHT;
            }
            break;
*/
        case ST_BRIGHT:
            DisplayAlnum(LETTER_B, 4 - bright, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_TIME;
            } else if (buttons & BUTTON_SET) {
                bright = (bright - 1) & 3;
                OCR0A = pgm_read_byte(&brighttable[bright]);
            } else if (buttons & BUTTON_START) {
                Save();
                state = ST_SAVED;
                remaining = 15;
            }
            break;
        case ST_SAVED:
            display[0] = LETTER_S;
            display[1] = LETTER_A;
            display[2] = LETTER_V;
            display[3] = LETTER_E;
            display[4] = EMPTY;
            if (--remaining == 0)
                state = ST_BRIGHT;
            break;
        case ST_TIME_SET_MINS:
            DisplayNum(stime[0], HIGH_POS, 0x40, 0, 0);
            display[EXTRA_POS] = COLON;
            DisplayNum(stime[1], LOW_POS, 0, 0, 0);
            if (EditNum(&stime[0], buttons, 100)) {
                state = ST_TIME_SET_SECS;
            }
            break;
        case ST_TIME_SET_SECS:
            DisplayNum(stime[0], HIGH_POS, 0, 0, 0);
            display[EXTRA_POS] = COLON;
            DisplayNum(stime[1], LOW_POS, 0x40, 0, 0);
            if (EditNum(&stime[1], buttons, 60)) {
                state = ST_TIME;
            }
            break;
        case ST_DELAY_SET_MINS:
            DisplayNum(delay[0], HIGH_POS, 0x40, 0, 1);
            display[EXTRA_POS] = EMPTY;
            DisplayNum(delay[1], LOW_POS, 0, 0, 0);
            if (EditNum(&delay[0], buttons, 100)) {
                state = ST_DELAY_SET_SECS;
            }
            break;
        case ST_DELAY_SET_SECS:
            DisplayNum(delay[0], HIGH_POS, 0, 0, 1);
            display[EXTRA_POS] = EMPTY;
            DisplayNum(delay[1], LOW_POS, 0x40, 0, 0);
            if (EditNum(&delay[1], buttons, 60)) {
                state = ST_DELAY;
            }
            break;
        case ST_COUNT_SET:
            DisplayAlnum(LETTER_C, count, 0x40, 0);
            if (EditNum(&count, buttons, 100)) {
                state = ST_COUNT;
            }
            break;
        case ST_MLU_SET:
            DisplayAlnum(LETTER_L, mlu, 0x40, 0);
            if (EditNum(&mlu, buttons, 60)) {
                state = ST_MLU;
            }
            break;
        case ST_RUN_PRIME:
            if (mlu > 0) {
                state = ST_MLU_PRIME;
                SHUTTER_ON();
                DisplayAlnum(LETTER_L, mlu, 0, 0);
            } else {
                InitRun(&state);
                goto newstate;
            }
            break;
        case ST_RUN_AUTO:
            if (gDirection == 0) {
                // time has elapsed.  close the shutter and stop the timer.
                SHUTTER_OFF();
                clock_stop();

                if (remaining > 0)
                {
                    if (--remaining == 0)
                    {
                        // we're done.
                        state = prevstate;
                        break;
                    }
                }

                gMin = delay[0];
                gSec = delay[1];
                gDirection = -1;
                state = ST_WAIT;
                clock_start();
                goto newstate;
            }
            // fall through
        case ST_RUN_MANUAL:
            if (cmode == 0) {
                // time left in this exposure
                DisplayNum(gMin, HIGH_POS, 0, 3, 0);
                DisplayNum(gSec, LOW_POS, 0, 0, 1);
            } else {
                // remaining exposures
                DisplayAlnum(LETTER_C, remaining, 0, 1);
            }
            display[EXTRA_POS] = (TCNT2 & 0x80) ? EMPTY : COLON;
            break;
        case ST_MLU_PRIME:
            SHUTTER_OFF();
            gMin = 0;
            gSec = mlu;
            gDirection = -1;
            state = ST_MLU_WAIT;
            clock_start();
            // fall-through
        case ST_MLU_WAIT:
            DisplayAlnum(LETTER_L, gSec, 0, 0);
            if (gDirection == 0)
            {
                // MLU wait period has elapsed
                clock_stop();
                InitRun(&state);
                goto newstate;
            }
            break;
        case ST_WAIT:
            if (cmode == 0) {
                // wait time
                DisplayNum(gMin, HIGH_POS, 0, 3, (TCNT2 & 0x80) ? 0 : 1);
                DisplayNum(gSec, LOW_POS, 0, 0, 0);
            } else {
                // remaining exposures
                DisplayAlnum(LETTER_C, remaining, 0, (TCNT2 & 0x80) ? 0 : 4);
            }
            display[EXTRA_POS] = EMPTY;
            if (gDirection == 0)
            {
                // wait period has timed out;
                // stop the timer and start a new cycle
                clock_stop();
                state = ST_RUN_PRIME;
                goto newstate;
            }
            break;
        }

        if (state >= ST_RUN_PRIME) {
            // check keys
            if (buttons & BUTTON_START) {
                // canceled.
                clock_stop();
                SHUTTER_OFF();

                // if counting up, freeze the count here
                if (state == ST_RUN_MANUAL) {
                    stime[0] = gMin;
                    stime[1] = gSec;
                }

                state = prevstate;
            } else if ((buttons & BUTTON_SELECT) && (count > 1)) {
                // toggle display, time left vs. count left
                cmode = (cmode + 1) & 1;
            } else if (buttons & BUTTON_SET) {
                // adjust brightness
                bright = (bright - 1) & 3;
                OCR0A = pgm_read_byte(&brighttable[bright]);
            }
        }

        Sleep(48);    // approx 50ms at 1MHz
     }
}

