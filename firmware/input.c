#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include "input.h"
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
volatile uint8_t prev_clock = 1;
volatile int8_t encoder_ticks = 0;

// triggers every 50ms, used to sample tac buttons and drive the state machine
ISR(TIMER1_COMPA_vect)
{
    input_ready = 1;
}

// the following ISR is taken from
// https://chome.nerpa.tech/mcu/rotary-encoder-interrupt-service-routine-for-avr-micros/

/* encoder routine. Expects encoder with four state changes between detents */
/* and both pins open on detent */
ISR(PCINT1_vect)
{
  static uint8_t old_AB = 3;  //lookup table index
  static int8_t encval = 0;   //encoder value
  static const int8_t enc_states [] PROGMEM =
  {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  //encoder lookup table
  /**/
  old_AB <<=2;  //remember previous state
  old_AB |= ( PINC & 0x03 );
  encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )]));
  /* post "Navigation forward/reverse" event */
  if( encval > 3 ) {  //four steps forward
    encoder_ticks -= ENCODER_MULTIPLIER;
    encval = 0;
  }
  else if( encval < -3 ) {  //four steps backwards
    encoder_ticks += ENCODER_MULTIPLIER;
    encval = 0;
  }
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
        if (curState == 0x7)
            repeat = 0;    // no buttons are down.
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
    *button_mask = GetButtons();
    *encoder_diff = encoder_ticks;
    encoder_ticks = 0;
}

