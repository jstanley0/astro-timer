#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include "input.h"
#include "settings.h"
#include "io.h"

void input_init()
{
    OCR1A = 12500;                       // 50ms cycle at 2MHz
    TCCR1B = (1 << WGM12) | (1 << CS11); // start timer at 1/8 prescaler, in CTC mode
    TIMSK1 = (1 << OCIE1A);              // enable compare match A interrupt

    // enable pin-change interrupt on encoder inputs
    PCMSK1 = (1 << PCINT8) | (1 << PCINT9);
    PCICR = (1 << PCIE1);
}

volatile uint8_t input_ready = 0;
volatile int8_t encoder_ticks = 0;

// triggers every 50ms, used to sample tac buttons and drive the state machine
ISR(TIMER1_COMPA_vect)
{
    input_ready = 1;
}

// the following ISR is adapted from
// https://chome.nerpa.tech/mcu/rotary-encoder-interrupt-service-routine-for-avr-micros/
// expects encoder with four state changes between detents and both pins open on detent
ISR(PCINT1_vect)
{
    // indexed by bits [prevB prevA curB curA], indicates the direction of rotation
    // or 0 for no change/bouncing state
    static const int8_t enc_states[] PROGMEM = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};

    static uint8_t enc_hist = 0b11;
    static int8_t enc_cycle = 0;

    uint8_t enc_bits = (PINC & 0b11);
    enc_hist <<= 2;
    enc_hist |= enc_bits;

    enc_cycle += pgm_read_byte(&enc_states[enc_hist & 0b1111]);

    // see if we've moved from detent to detent, and record the tick
    if (enc_bits == 0b11) {
        if(enc_cycle > 3) {
            encoder_ticks += enc_cw;
            enc_cycle = 0;
        } else if(enc_cycle < -3) {
            encoder_ticks -= enc_cw;
            enc_cycle = 0;
        }
    }
}

// here's how button presses work:
// - a press is registered when a button is released.
// - a hold is registered when the same button has been down
//   for a specified number of cycles.  the button release
//   following the hold does not register.
// - if async button state is queried, any pending event is suppressed
//   (this is used for click+turn navigation)

#define REPEAT_THRESHOLD 20

uint8_t GetButtons(uint8_t async)
{
    static uint8_t prevState = 0xff;
    static uint8_t repeat = 0;

    uint8_t curState = BUTTON_STATE();

    if (async) {
        repeat = REPEAT_THRESHOLD;
        return (~curState & 0x7);
    }

    // if we've already registered a "hold"
    if (repeat >= REPEAT_THRESHOLD) {
        if (curState == 0x7)
            repeat = 0;    // no buttons are down.
        prevState = curState;
        return 0;
    }

    // if the encoder moves while a button is held, suppress the event
    if (encoder_ticks && (curState != 0x7)) {
        repeat = REPEAT_THRESHOLD;
        prevState = curState;
        return 0;
    }

    if (curState != prevState) {
        repeat = 0;
        uint8_t pressed = ~prevState & curState;
        prevState = curState;
        return pressed;
    } else if (curState != 0x7) {
        // button(s) are being held
        if (++repeat == REPEAT_THRESHOLD) {
            return BUTTON_HOLD | ~(curState & 0x7);
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
    uint8_t button_state = GetButtons(encoder_ticks);
    if (encoder_ticks && button_state) {
        *encoder_diff = 0;
        if (button_state & BUTTON_SET) {
            *button_mask = (encoder_ticks > 0) ? BRIGHT_UP : BRIGHT_DOWN;
        } else {
            *button_mask = (encoder_ticks > 0) ? BUTTON_SELECT : BUTTON_BACK;
        }
    } else {
        *encoder_diff = encoder_ticks;
        *button_mask = button_state;
    }
    encoder_ticks = 0;
}

