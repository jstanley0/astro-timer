// Exposure timer MkII
// based on AVR ATMEGA48 running at 1MHz (default fuses okay)
// build with avr-gcc
//
// Source code (c) 2007-2021 by Jeremy Stanley
// https://github.com/jstanley0/astro-timer
// Licensed under GNU GPL v2 or later

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

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

void state_delay() {

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




static unsigned char EditNum(uint8_t *num, uint8_t buttons, uint8_t max)
{
    if (buttons == 0)
        return 0;

    display_blink_reset();

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
    init_io();

    // Load saved state, if any
    Load();

    display_init();
    rtc_init();
    input_init();

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
        // note that this puts the processor to sleep for up to 50ms
        uint8_t buttons = input_poll();

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
                display_set_brightness(bright);
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
                display_set_brightness(bright);
            }
        }
     }
}

