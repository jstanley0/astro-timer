// Astrophotography Exposure Timer Mk IV
// based on AVR ATMEGAx8 running at 1MHz (default fuses okay)
// build with avr-gcc
//
// Source code (c) 2007-2021 by Jeremy Stanley
// https://github.com/jstanley0/astro-timer
// Licensed under GNU GPL v2 or later

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <util/delay.h>

#include "io.h"
#include "input.h"
#include "clock.h"
#include "display.h"
#include "settings.h"
#include "sensors.h"

// 20 minutes (with 1200 I/O polling cycles per minute)
#define IDLE_TIMEOUT_CYCLES 20 * 1200

const uint16_t stop_table[] PROGMEM = {0, 1, 2, 3, 4, 5, 6, 8, 10, 13, 15, 20, 25, 30, 35, 40, 45, 50, 60, 75, 90, 120, 150, 180, 210, 240, 300, 360, 480, 540, 600, 720, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000, 3300, 3600, 4500, 5400};
const size_t STOP_TABLE_SIZE = sizeof(stop_table) / sizeof(stop_table[0]);

void adjust_stop(uint8_t min_sec[2], int8_t *current_stop, int8_t encoder_diff)
{
    uint16_t secs = (uint16_t)min_sec[0] * 60 + min_sec[1];
    if (*current_stop < 0) {
        // the value was manually edited and we're possibly between stops, so find the one to start from
        uint8_t i = 0;
        // find the first stop that's greater than the current time
        while(i < STOP_TABLE_SIZE && pgm_read_word(&stop_table[i]) <= secs)
            ++i;
        // if we're going up, the stop we found is where we want to increment to with our first tick
        if (encoder_diff > 0)
            --i;
        *current_stop = i;
    }
    *current_stop += encoder_diff;
    if (*current_stop < 0)
        *current_stop = 0;
    else if (*current_stop >= STOP_TABLE_SIZE)
        *current_stop = STOP_TABLE_SIZE - 1;
    uint16_t new_time = pgm_read_word(&stop_table[*current_stop]);
    min_sec[0] = new_time / 60;
    min_sec[1] = new_time % 60;
}

void increment_num(uint8_t *num, int8_t encoder_diff, uint8_t max)
{
    // spinning the encoder will stop at the low limit (0) and high limit (max)
    // (where max is 59 or 99)
    *num += encoder_diff;
    if (*num > 200)
        *num = 0;   // underflow, probably
    else if (*num > max)
        *num = max;
}

static unsigned char EditNum(uint8_t *num, uint8_t buttons, int8_t encoder_diff, uint8_t max)
{
    if (buttons == 0 && encoder_diff == 0)
        return 0;

    CLOCK_BLINK_RESET();

    // holding Set resets to 0
    if ((buttons & (BUTTON_SET | BUTTON_HOLD)) == (BUTTON_SET | BUTTON_HOLD)) {
        buttons ^= BUTTON_SET;  // don't save/exit in this case
        *num = 0;
    }

    // tapping Select increments by 1 (and holding Select increments by 10)
    if (buttons & BUTTON_SELECT) {
        if (buttons & BUTTON_HOLD)
            *num += 10;
        else
            *num += 1;
    }

    // the encoder increments / decrements by 1, saturating
    // (button taps wrap back to 0)
    if (encoder_diff != 0) {
        increment_num(num, encoder_diff, max);
    } else {
        if (*num > max)
            *num = 0;
    }

    // save with either the Set button or the Start button
    return (buttons & (BUTTON_SET | BUTTON_START));
}

void adjust_brightness(int8_t encoder_diff)
{
    int8_t diff = encoder_diff ? encoder_diff : 1;
    int8_t new_val = (int8_t)bright - diff;
    while (new_val < 0)
        new_val += 6;
    while (new_val > 5)
        new_val -= 6;
    bright = (uint8_t)new_val;
    display_set_brightness(bright);
}

void display_signature_byte(uint8_t addr)
{
    DisplayHex(addr, HIGH_POS);
    DisplayHex(boot_signature_byte_get(addr), LOW_POS);
    display[EXTRA_POS] = COLON;
}

enum State {
    // main exposure menu
    ST_TIME, ST_DELAY, ST_COUNT, ST_MLU,
    // options
    ST_BRIGHT, ST_SAVED, ST_POWER_METER,
    ST_TEMP_SENSOR, ST_SIGNATURE_ROW,
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

void run()
{
    // init the state machine
    enum State state = ST_TIME;
    enum State prevstate = ST_TIME;
    uint8_t remaining = 0;
    uint8_t cmode = 0;
    int8_t stime_stop = -1;
    int8_t delay_stop = -1;
    uint16_t idle_cycles = 0;
    int8_t useF = 0;
    uint8_t sig = 0;

    for(;;)
    {
        uint8_t buttons;
        int8_t encoder_diff;
        input_poll(&buttons, &encoder_diff);

        if (state >= ST_RUN_PRIME || buttons || encoder_diff) {
            idle_cycles = 0;
        } else if (++idle_cycles == IDLE_TIMEOUT_CYCLES) {
            turn_adc_off();
            break;
        }

        // soft power-off
        if ((buttons & (BUTTON_START | BUTTON_HOLD)) == (BUTTON_START | BUTTON_HOLD)) {
            turn_adc_off();
            break;
        }

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
            } else if (encoder_diff) {
                adjust_stop(stime, &stime_stop, encoder_diff);
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
            } else if (encoder_diff) {
                adjust_stop(delay, &delay_stop, encoder_diff);
            }
            break;
        case ST_COUNT:
            DisplayAlnum(LETTER_C, count, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_MLU;
            } else if (buttons & BUTTON_SET) {
                state = ST_COUNT_SET;
            } else if (encoder_diff) {
                increment_num(&count, encoder_diff, 99);
            }
            break;
        case ST_MLU:
            DisplayAlnum(LETTER_L, mlu, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_BRIGHT;
            } else if (buttons & BUTTON_SET) {
                state = ST_MLU_SET;
            } else if (encoder_diff) {
                increment_num(&mlu, encoder_diff, 99);
            }
            break;
        case ST_BRIGHT:
            DisplayAlnum(LETTER_B, 6 - bright, 0, 0);
            if (buttons & BUTTON_SELECT) {
                state = ST_TIME;
            } else if (encoder_diff) {
                adjust_brightness(encoder_diff);
                display_set_brightness(bright);
            } else if (buttons & BUTTON_START) {
                turn_adc_on();
                init_power_meter();
                state = ST_POWER_METER;
            } else if (buttons & BUTTON_SET) {
                Save();
                prevstate = state;
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
                state = prevstate;
            break;
        case ST_POWER_METER:
            display_power_meter();
            if (buttons & BUTTON_SELECT) {
                turn_adc_off();
                state = ST_TIME;
            } else if (buttons & BUTTON_START) {
                init_temp_sensor();
                state = ST_TEMP_SENSOR;
            }
            break;
        case ST_TEMP_SENSOR:
            display_temp_sensor(useF);
            if (buttons & BUTTON_SELECT) {
                turn_adc_off();
                state = ST_TIME;
            } else if (buttons & BUTTON_START) {
                turn_adc_off();
                state = ST_SIGNATURE_ROW;
            } else if (buttons & BUTTON_SET) {
                useF = !useF;
            }
            break;
        case ST_SIGNATURE_ROW:
            display_signature_byte(sig);
            sig = (sig + encoder_diff) & 0x1f;
            if (buttons & BUTTON_START) {
                state = ST_BRIGHT;
            } else if (buttons & BUTTON_SELECT) {
                state = ST_TIME;
            }
            break;
        case ST_TIME_SET_MINS:
            DisplayNum(stime[0], HIGH_POS, 0x40, 0, 0);
            display[EXTRA_POS] = COLON;
            DisplayNum(stime[1], LOW_POS, 0, 0, 0);
            if (EditNum(&stime[0], buttons, encoder_diff, 99)) {
                state = ST_TIME_SET_SECS;
            }
            break;
        case ST_TIME_SET_SECS:
            DisplayNum(stime[0], HIGH_POS, 0, 0, 0);
            display[EXTRA_POS] = COLON;
            DisplayNum(stime[1], LOW_POS, 0x40, 0, 0);
            if (EditNum(&stime[1], buttons, encoder_diff, 59)) {
                stime_stop = -1;
                state = ST_TIME;
            }
            break;
        case ST_DELAY_SET_MINS:
            DisplayNum(delay[0], HIGH_POS, 0x40, 0, 1);
            display[EXTRA_POS] = EMPTY;
            DisplayNum(delay[1], LOW_POS, 0, 0, 0);
            if (EditNum(&delay[0], buttons, encoder_diff, 99)) {
                state = ST_DELAY_SET_SECS;
            }
            break;
        case ST_DELAY_SET_SECS:
            DisplayNum(delay[0], HIGH_POS, 0, 0, 1);
            display[EXTRA_POS] = EMPTY;
            DisplayNum(delay[1], LOW_POS, 0x40, 0, 0);
            if (EditNum(&delay[1], buttons, encoder_diff, 59)) {
                delay_stop = -1;
                state = ST_DELAY;
            }
            break;
        case ST_COUNT_SET:
            DisplayAlnum(LETTER_C, count, 0x40, 0);
            if (EditNum(&count, buttons, encoder_diff, 99)) {
                state = ST_COUNT;
            }
            break;
        case ST_MLU_SET:
            DisplayAlnum(LETTER_L, mlu, 0x40, 0);
            if (EditNum(&mlu, buttons, encoder_diff, 59)) {
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
            display[EXTRA_POS] = CLOCK_BLINKING() ? EMPTY : COLON;
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
                DisplayNum(gMin, HIGH_POS, 0, 3, CLOCK_BLINKING() ? 0 : 1);
                DisplayNum(gSec, LOW_POS, 0, 0, 0);
            } else {
                // remaining exposures
                DisplayAlnum(LETTER_C, remaining, 0, CLOCK_BLINKING() ? 0 : 4);
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
            } else if ((count > 1) && (buttons & BUTTON_SELECT)) {
                // toggle display, time left vs. count left
                cmode = (cmode + 1) & 1;
            } else if (buttons & BUTTON_SET || encoder_diff) {
                // adjust brightness
                adjust_brightness(encoder_diff);
            }
        }
    }
}

int main(void)
{
    sysclk_init();
    io_init();
    blip();

    // Load saved state, if any
    Load();

    power_init();
    display_init();
    clock_init();
    input_init();

    sei();

    for(;;)
    {
        // doesn't return unless the device has been idle for a long time, ...
        run();

        // .. in which case we shut down, to save battery power.
        // but we leave a pin change interrupt running, so a button press will wake us up
        acknowledge_power_off();
        power_down();
    }
}

