#pragma once

// resources used: timer2

extern volatile int8_t gMin, gSec;
extern volatile int8_t gDirection;

void clock_init();
void clock_start();
void clock_stop();
void clock_wait_for_xtal();

#define CLOCK_BLINKING() (TCNT2 & 0x80)
#define CLOCK_BLINK_RESET() TCNT2 = 0

