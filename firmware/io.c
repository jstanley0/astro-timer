#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "io.h"
#include "display.h"

void io_init()
{
    DDRB  = 0b00111111;
    PORTB = 0b00000000;
    DDRC  = 0b00000000;
#ifdef EXTERNAL_ENCODER_PULLUPS
    PORTC = 0b00111000;
#else
    PORTC = 0b00111111;
#endif
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
    // shut down all outputs
    DIGITS_OFF();
    SHUTTER_OFF();

    // stop all timers
    uint8_t saved_TCCR2B = TCCR2B;
    TCCR2B = 0;
    uint8_t saved_TCCR1B = TCCR1B;
    TCCR1B = 0;
    uint8_t saved_TCCR0B = TCCR0B;
    TCCR0B = 0;

    // save pin-change interrupt state, and enable the interrupt on all inputs
    uint8_t saved_PCMSK1 = PCMSK1;
    PCMSK1 = 0b000111111;
    uint8_t saved_PCICR = PCICR;
    PCICR = (1 << PCIE1);

    // power down!
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();

    // an interrupt woke us up. restore prior state
    PCMSK1 = saved_PCMSK1;
    PCICR = saved_PCICR;
    TCCR0B = saved_TCCR0B;
    TCCR1B = saved_TCCR1B;
    TCCR2B = saved_TCCR2B;

    // wait for the crystal to start ticking
    while((ASSR & 0x01) | (ASSR & 0x04));
}
