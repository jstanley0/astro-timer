#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "io.h"
#include "display.h"
#include "clock.h"

void io_init()
{
    DDRB  = 0b00111111;
    PORTB = 0b00000000;
    DDRC  = 0b00100000;
    PORTC = 0b00011100;
    DDRD  = 0b11111111;
    PORTD = 0b11111111;
}

// flash the apostrophe. useful for "is this code being reached?"
void blip()
{
    cli();
    DIGITS_OFF();
    DIGIT_VALUE(APOS);
    DIGIT_ON(4);
    _delay_ms(1);
    sei();
}

// go into as deep a sleep as we can manage, waking up on input of any kind
void power_down()
{
    // stop interrupts so things don't change out from underneath us
    cli();

    // shut down all outputs
    DIGITS_OFF();
    SHUTTER_OFF();
    SHUTTER_HALFPRESS_OFF();

    // stop all timers
    uint8_t saved_TCCR2B = TCCR2B;
    TCCR2B = 0;
    uint8_t saved_TCCR1B = TCCR1B;
    TCCR1B = 0;
    uint8_t saved_TCCR0B = TCCR0B;
    TCCR0B = 0;

    // save pin-change interrupt state, and enable the interrupt on all inputs
    uint8_t saved_PCMSK1 = PCMSK1;
    PCMSK1 = 0b00011111;
    uint8_t saved_PCICR = PCICR;
    PCICR = (1 << PCIE1);

    // re-enable interrupts so we can actually wake up
    sei();

    // power down!
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();

    // a pin-change interrupt woke us up.
    // snooze for awhile to swallow the button release
    // (but not _indefinitely_ in case the encoder stopped between detents or something)
    for(uint8_t delay = 0; delay < 10; ++delay) {
        _delay_ms(100);
        if (0b00011111 == (PINC & 0b00011111))
            break;
    }

    // restore prior state
    PCMSK1 = saved_PCMSK1;
    PCICR = saved_PCICR;
    TCCR0B = saved_TCCR0B;
    TCCR1B = saved_TCCR1B;
    TCCR2B = saved_TCCR2B;

    // wait for the crystal to start ticking
    clock_wait_for_xtal();
}
