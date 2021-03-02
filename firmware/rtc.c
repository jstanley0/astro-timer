
void init_rtc()
{
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
}

static void clock_start() {
    TCNT2 = 0;
    TIFR2 = (1 << TOV2);
    TIMSK2 |= (1 << TOIE2);
}

static void clock_stop() {
    TIMSK2 &= (uint8_t)~(1 << TOIE2);
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
