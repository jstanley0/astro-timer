// Exposure timer MkII
// based on AVR ATMEGA48 running at 1MHz (default fuses okay)
// build with avr-gcc
//
// Source code (c) 2007 by Jeremy Stanley
// http://www.xmission.com/~jstanley/avrtimer.html
// Licensed under GNU GPL v2 or later
//

// Port assignments:
// PORTB0    (input)  = Select key
// PORTB1..5 (output) = Digit anode drivers
// PORTB6..7          = 32.768kHz watch crystal
// PORTC0    (input)  = Set key
// PORTC1    (input)  = Start key
// PORTC2..4          = unused
// PORTC5    (output) = Optoisolator to camera
// PORTD0..7 (output) = Segment cathodes 

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
	TCCR1B = 0;				// stop the timer
	TIFR1 = (1 << OCF1A);	// clear output-compare A flag
	OCR1A = kiloclocks;		// set compare match A target
	TCNT1 = 0;				// reset timer counter
	TIMSK1 = (1 << OCIE1A);	// enable compare match A interrupt
	TCCR1B = (1 << CS12) | (1 << CS10);	// start timer with 1/1024 prescaler

	// sleep until it's time to wake up
	// use a loop here because other interrupts will happen
	wakeup = 0;
	set_sleep_mode(SLEEP_MODE_IDLE);
	do {
		sleep_mode();
	} while( !wakeup );

	TIMSK1 = 0;				// stop the interrupt
	TCCR1B = 0;				// stop the timer
}


// inverted since we're using a common anode display
uint8_t digits[10] PROGMEM = {
	~0x3f, // 0 = 00111111
	~0x06, // 1 = 00000110
	~0x5b, // 2 = 01011011
	~0x4f, // 3 = 01001111
	~0x66, // 4 = 01100110
	~0x6d, // 5 = 01101101
	~0x7c, // 6 = 01111100
	~0x07, // 7 = 00000111
	~0x7f, // 8 = 01111111
	~0x67, // 9 = 01100111
};

#define DIGITS_OFF()   PORTB &= (uint8_t)0xc1
#define DIGIT_ON(x)    PORTB |= (0x20 >> x)

#define SHUTTER_OFF()  PORTC &= (uint8_t)~(1 << PC5)
#define SHUTTER_ON()   PORTC |= (1 << PC5)

static void clock_start() {
	TCNT2 = 0; 
	TIFR2 = (1 << TOV2); 
	TIMSK2 |= (1 << TOIE2);
}

static void clock_stop() {
	TIMSK2 &= (uint8_t)~(1 << TOIE2);
}

// Refresh interrupt - refreshes the next digit on the display.
// By drawing each in turn quickly enough, we give the illusion of
// a solid display, but without requiring the output ports and wiring
// to drive each digit independently.

volatile uint8_t display[5] = { '\xff', '\xff', '\xff', '\xff', '\xff' };
ISR(TIMER0_OVF_vect)
{
	static uint8_t didx = 0;
	PORTD = display[didx];
	DIGIT_ON(didx);
	if (++didx == 5)
		didx = 0;
}


// Blanking interrupt - clears the display prior to the next refresh.
// We need to turn the digits off before switching segments to
// prevent ghosting caused by the wrong value being briefly displayed.
// By changing the value of OCR0A, we can control the effective 
// brightness of the display.

ISR(TIMER0_COMPA_vect)
{
	DIGITS_OFF();
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


// here's how button presses work:
// - a press is registered when a button is released.
// - a hold is registered when the same button has been down
//   for a specified number of cycles.  the button release
//   following the hold does not register.

#define BUTTON_UP    0x01
#define BUTTON_RIGHT 0x02
#define BUTTON_LEFT  0x04
#define BUTTON_HOLD  0x10 	// button was held

#define REPEAT_THRESHOLD 15

uint8_t GetButtons()
{
	static uint8_t prevState = 0xff;
	static uint8_t repeat = 0;

	uint8_t curState = ((PINB & 1) << 2) | (PINC & 3);
	
	// if we've already registered a "hold"
	if (repeat >= REPEAT_THRESHOLD) {
		prevState = curState;
		if (curState == 7)
			repeat = 0;	// no buttons are down.
		return 0;
	}

	if (curState != prevState) {
		uint8_t pressed = ~prevState & curState;
		prevState = curState;
		return pressed;
	} else if (curState != 7) {
		// button(s) are being held
		if (++repeat == REPEAT_THRESHOLD) {
			return BUTTON_HOLD | ~(curState & 7);
		}
	} 
	
	return 0;
}

static const uint8_t brighttable[4] PROGMEM = { 255, 85, 28, 9 };


#define HIGH_POS 0
#define LOW_POS  3
#define EMPTY '\xFF'
#define COLON_POS 2
#define COLON '\xFC'
#define HIGHDOT '\xFE'
#define LOWDOT '\xFD' 

// strip - bit 0 = don't display tens place if num < 10
//         bit 1 = ...           ones place if num == 0
static void DisplayNum(uint8_t num, uint8_t pos, uint8_t blink_mask, uint8_t strip)
{
	uint8_t digs[2];

	if (TCNT2 & blink_mask) {
		display[pos] = EMPTY;
		display[pos + 1] = EMPTY;
	} else {
		IntToDigs2(num, digs);		
		display[pos] = ((strip & 1) && (num < 10)) ? EMPTY 
			: pgm_read_byte(&digits[digs[0]]);
		display[pos + 1] = ((strip & 2) && (num == 0)) ? EMPTY 
			: pgm_read_byte(&digits[digs[1]]);
	}
}

static unsigned char EditNum(uint8_t *num, uint8_t buttons, uint8_t max)
{
	if (buttons == 0)
		return 0;

    TCNT2 = 0;	// reset blinkage when a key is pressed

	if (buttons & BUTTON_LEFT) {
		if (buttons & BUTTON_HOLD)
			*num = 0;
		else
			*num += 10;
	}
	if (buttons & BUTTON_RIGHT) {
		(*num)++;
	}
	if (*num >= max)
		*num = 0;

	return (buttons & BUTTON_UP);
}

static void DisplayAlnum(char letter, uint8_t num, uint8_t blink_mask)
{
	display[0] = letter;
	display[1] = EMPTY;
	display[2] = EMPTY;
	DisplayNum(num, LOW_POS, blink_mask, (blink_mask == 0) ? 1 : 0);
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

// params
uint8_t stime[2] = { 0, 0 };
uint8_t delay[2] = { 0, 0 };
uint8_t count    = 1;
uint8_t mlu      = 0;
uint8_t bright   = 1;

inline void savebyte(uint16_t addr, uint8_t value)
{
	eeprom_write_byte((uint8_t *)addr, value);
}

uint8_t loadbyte(uint16_t addr, uint8_t default_value)
{
	uint8_t ret = eeprom_read_byte((uint8_t *) addr);
	if (ret == '\xFF')
		ret = default_value;
	return ret;
}

void Save()
{
	savebyte(0, stime[0]);
	savebyte(1, stime[1]);
	savebyte(2, delay[0]);
	savebyte(3, delay[1]);
	savebyte(4, count);
	savebyte(5, mlu);
	savebyte(6, bright);
}

void Load()
{
	stime[0] = loadbyte(0, 0);
	stime[1] = loadbyte(1, 0);
	delay[0] = loadbyte(2, 0);
	delay[1] = loadbyte(3, 0);
	count    = loadbyte(4, 1);
	mlu      = loadbyte(5, 0);
	bright   = loadbyte(6, 1);
}

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
	// Initialize I/O
	DDRB  = 0x3e; // 00111110
	PORTB = 0x01; // 00000001
	DDRC  = 0x20; // 00100000
	PORTC = 0x03; // 00000011
	DDRD  = 0xff; // 11111111
	PORTD = 0xff; // 11111111

	// Load saved state, if any
	Load();

	// Setup the display timer...
	
	// prescaler 1/8; at 1MHz system clock, this gives us an overflow
	// at 488 Hz, providing a per-digit refresh rate of 97.6 Hz.
	TCCR0A = 0;
	TCCR0B = (1<<CS01); 

	// Output compare value B - controls blanking.
	// In full brightness mode, we'll make this happen immediately before the refresh,
	// In lower brightness modes, we'll make it happen sooner.
	OCR0A = pgm_read_byte(&brighttable[bright]);

	// Enable overflow and compare match interrupts
    TIMSK0 = (1<<TOIE0) | (1<<OCIE0A);


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
		uint8_t buttons = GetButtons();

		if ((buttons & BUTTON_RIGHT) && (state < ST_BRIGHT)) {
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
			DisplayNum(stime[0], HIGH_POS, 0, 3);
			display[COLON_POS] = COLON;
			DisplayNum(stime[1], LOW_POS, 0, 0);
			if (buttons & BUTTON_LEFT) {
				state = ST_DELAY;
			} else if (buttons & BUTTON_UP) {
				state = ST_TIME_SET_MINS;
			}
			break;
		case ST_DELAY:
			DisplayNum(delay[0], HIGH_POS, 0, 3);
			display[COLON_POS] = LOWDOT;
			DisplayNum(delay[1], LOW_POS, 0, 0);
			if (buttons & BUTTON_LEFT) {
				state = ST_COUNT;
			} else if (buttons & BUTTON_UP) {
				state = ST_DELAY_SET_MINS;
			}
			break;
		case ST_COUNT:
			DisplayAlnum('\xC6', count, 0);
			if (buttons & BUTTON_LEFT) {
				state = ST_MLU;
			} else if (buttons & BUTTON_UP) {
				state = ST_COUNT_SET;
			}
			break;
		case ST_MLU:
			DisplayAlnum('\xC7', mlu, 0);
			if (buttons & BUTTON_LEFT) {
				state = ST_BRIGHT;
			} else if (buttons & BUTTON_UP) {
				state = ST_MLU_SET;
			}
			break;
/* I'll put this in once there's more than one option ;)
		case ST_OPTIONS:
			display[0] = '\xC0';	// 00111111 = 'O'
			display[1] = '\x8C';	// 01110011 = 'P'
			display[2] = EMPTY;
			display[3] = '0x87';	// 01111000 = 't'
			display[4] = '0x92';	// 01101101 = 'S'
			if (buttons & BUTTON_LEFT) {
				state = ST_TIME;
			} else if (buttons & BUTTON_UP) {
				state = ST_BRIGHT;
			}
			break;
*/
		case ST_BRIGHT:
			DisplayAlnum('\x83', 4 - bright, 0); // 10000011 = 'b'
			if (buttons & BUTTON_LEFT) {
				state = ST_TIME;
			} else if (buttons & BUTTON_UP) {
				bright = (bright - 1) & 3;
				OCR0A = pgm_read_byte(&brighttable[bright]);
			} else if (buttons & BUTTON_RIGHT) {
				Save();
				state = ST_SAVED;
				remaining = 15;
			}
			break;
		case ST_SAVED:
			display[0] = '\x92'; // 01101101 = 'S'
			display[1] = '\x88'; // 01110111 = 'A'
			display[2] = EMPTY;
			display[3] = '\xc1'; // 00111110 = 'V'
			display[4] = '\x86'; // 01111001 = 'E'
			if (--remaining == 0)
				state = ST_BRIGHT;
			break;
		case ST_TIME_SET_MINS:
			DisplayNum(stime[0], HIGH_POS, 0x40, 0);
			display[COLON_POS] = COLON;
			DisplayNum(stime[1], LOW_POS, 0, 0);
			if (EditNum(&stime[0], buttons, 100)) {
				state = ST_TIME_SET_SECS;
			}
			break;
		case ST_TIME_SET_SECS:
			DisplayNum(stime[0], HIGH_POS, 0, 0);
			display[COLON_POS] = COLON;
			DisplayNum(stime[1], LOW_POS, 0x40, 0);
			if (EditNum(&stime[1], buttons, 60)) {
				state = ST_TIME;
			}
			break;
		case ST_DELAY_SET_MINS:
			DisplayNum(delay[0], HIGH_POS, 0x40, 0);
			display[COLON_POS] = LOWDOT;
			DisplayNum(delay[1], LOW_POS, 0, 0);
			if (EditNum(&delay[0], buttons, 100)) {
				state = ST_DELAY_SET_SECS;
			}
			break;
		case ST_DELAY_SET_SECS:
			DisplayNum(delay[0], HIGH_POS, 0, 0);
			display[COLON_POS] = LOWDOT;
			DisplayNum(delay[1], LOW_POS, 0x40, 0);
			if (EditNum(&delay[1], buttons, 60)) {
				state = ST_DELAY;
			}
			break;
		case ST_COUNT_SET:
			DisplayAlnum('\xC6', count, 0x40);
			if (EditNum(&count, buttons, 100)) {
				state = ST_COUNT;
			}
			break;
		case ST_MLU_SET:
			DisplayAlnum('\xC7', mlu, 0x40);
			if (EditNum(&mlu, buttons, 60)) {
				state = ST_MLU;
			}
			break;
		case ST_RUN_PRIME:
			if (mlu > 0) {
				state = ST_MLU_PRIME;
				SHUTTER_ON();
				DisplayAlnum('\xC7', mlu, 0);
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
				DisplayNum(gMin, HIGH_POS, 0, 3);
				DisplayNum(gSec, LOW_POS, 0, 0);
			} else {
				// remaining exposures
				DisplayAlnum('\xC6', remaining, 0);
			}
			display[COLON_POS] = (TCNT2 & 0x80) ? EMPTY : COLON;
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
			DisplayAlnum('\xC7', gSec, 0);
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
				DisplayNum(gMin, HIGH_POS, 0, 3);
				DisplayNum(gSec, LOW_POS, 0, 0);
			} else {
				// remaining exposures
				DisplayAlnum('\xC6', remaining, 0);
			}
			display[COLON_POS] = (TCNT2 & 0x80) ? EMPTY : LOWDOT;
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
			if (buttons & BUTTON_RIGHT) {
				// canceled.
				clock_stop();
				SHUTTER_OFF();

				// if counting up, freeze the count here
				if (state == ST_RUN_MANUAL) {
					stime[0] = gMin;
					stime[1] = gSec;
				}
		
				state = prevstate;
			} else if ((buttons & BUTTON_LEFT) && (count > 1)) {
				// toggle display, time left vs. count left
				cmode = (cmode + 1) & 1;
			} else if (buttons & BUTTON_UP) {
				// adjust brightness
				bright = (bright - 1) & 3;
				OCR0A = pgm_read_byte(&brighttable[bright]);
			}
		}
	
		Sleep(48);	// approx 50ms at 1MHz
 	}
}

