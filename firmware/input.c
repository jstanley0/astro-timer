#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "input.h"
#include "io.h"

void input_init()
{
    OCR1A = 5000;
    TCCR1B = (1 << WGM12) | (1 << CS10);    // start timer w/o prescaler, in CTC mode
    TIMSK1 = (1 << OCIE1A);    // enable overflow interrupt
}

volatile uint8_t input_ready = 0;
volatile int8_t encoder_ticks = 0;

// triggers every 5ms. we will poll the encoder that frequently, but check the buttons
// (and allow the main program's state machine to re-evaluate) every 50ms
ISR(TIMER1_COMPA_vect)
{
    static uint8_t cycles = 0;
    static uint8_t last_encoder_clock = 1;

    uint8_t encoder_clock = ENCODER_CLOCK();
    if (encoder_clock && !last_encoder_clock) {
        encoder_ticks += (ENCODER_DATA() ? -1 : 1);
    }
    last_encoder_clock = encoder_clock;

    if (++cycles == 10) {
        input_ready = 1;
        cycles = 0;
    }
}


// here's how button presses work:zz
// - a press is registered when a button is released.
// - a hold is registered when the same button has been down
//   for a specified number of cycles.  the button release
//   following the hold does not register.

#define REPEAT_THRESHOLD 15

uint8_t GetButtons()
{
    static uint8_t prevState = 0xff;
    static uint8_t repeat = 0;

    uint8_t curState = BUTTON_STATE();

    // if we've already registered a "hold"
    if (repeat >= REPEAT_THRESHOLD) {
        prevState = curState;
        if (curState == 0xF)
            repeat = 0;    // no buttons are down.
        return 0;
    }

    if (curState != prevState) {
        uint8_t pressed = ~prevState & curState;
        prevState = curState;
        return pressed;
    } else if (curState != 0xF) {
        // button(s) are being held
        if (++repeat == REPEAT_THRESHOLD) {
            return BUTTON_HOLD | ~(curState & 0xF);
        }
    }

    return 0;
}

void input_poll(uint8_t *button_mask, int8_t *encoder_diff)
{
    set_sleep_mode(SLEEP_MODE_IDLE);
    while (!input_ready) {
        sleep_mode();
    }
    input_ready = 0;
    *button_mask = GetButtons();
    *encoder_diff = encoder_ticks;
    encoder_ticks = 0;
}

