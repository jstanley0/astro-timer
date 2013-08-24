astro-timer
===========

firmware for an ATMEGA48-powered astrophotography timer

---

This is the code behind a timer I built for astrophotography. The idea is, you
plug this thing into your camera's remote shutter release port, set the camera
to bulb mode, and let this thing take a series of long-exposure photos.

The exposure length, exposure count, time between exposures, and optionally
mirror lockup time can be set. Additionally, screen brightness can be adjusted,
and preferences can be saved in NVRAM.

Parts:
 - Atmel ATMEGA48 microcontroller
 - 4-digit 7-segment LED display (code assumes common anode)
 - Suitable resistors to drive the LEDs
 - 3V power supply (I use 2x AA batteries; they've lasted years)
 - Three momentary-contact switches (Select, Set, and Start keys)
 - One optoisolater (interface to camera)
 - One 32.768kHz watch crystal
 - And some sort of jack compatible with your camera's remote shutter release port

The wiring is as follows:
 - PORTB0    (input)  = Select key
 - PORTB1..5 (output) = Digit anode drivers
 - PORTB6..7          = 32.768kHz watch crystal
 - PORTC0    (input)  = Set key
 - PORTC1    (input)  = Start key
 - PORTC2..4          = unused
 - PORTC5    (output) = Optoisolator to camera
 - PORTD0..7 (output) = Segment cathodes 

Compile with AVRGCC.

