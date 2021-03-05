#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "input.h"
#include "io.h"

void input_init()
{
    OCR1A = 50000; // 50ms at 1MHz
    TCCR1B = (1 << WGM12) | (1 << CS10);    // start timer w/o prescaler, in CTC mode
    TIMSK1 = (1 << OCIE1A);    // enable compare match A interrupt

    // enable pin-change interrupt on encoder clock
    PCMSK1 = (1 << PCINT8);
    PCICR = (1 << PCIE1);

}

volatile uint8_t input_ready = 0;
volatile int8_t encoder_ticks = 0;

// triggers every 50ms
ISR(TIMER1_COMPA_vect)
{
    input_ready = 1;
}

// triggers when the encoder is moved
// TODO debounce, somehow
ISR(PCINT1_vect)
{
    static uint8_t prev_clock = 1;
    uint8_t clock = (PINC & 1);
    // detect rising edge
    if (clock && !prev_clock) {
        encoder_ticks += (PINC & 2) ? -1 : 1;
    }
    prev_clock = clock;
}


// here's how button presses work:
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

