astro-timer
===========

firmware for an ATMEGAx8-powered astrophotography timer

---

This is the code behind a timer I built for astrophotography. The idea is, you
plug this thing into your camera's remote shutter release port, set the camera
to bulb mode, and let this thing take a series of long-exposure photos.

The exposure length, exposure count, time between exposures, and optionally
mirror lockup time can be set. Additionally, screen brightness can be adjusted,
and preferences can be saved in NVRAM.

Parts:
 - Atmel ATMEGA168 microcontroller (a prior version used ATMEGA48 and the code 
   still fits, but my original hardware was lost and I had an ATMEGA168 on hand)
 - 4-digit 7-segment LED display (code assumes common anode; PCB uses
   COM-09483 from SparkFun, model YSD-439AR6B-35)
 - Suitable resistors to drive the LEDs (I use 470 ohms)
 - 3V power supply (I use 2x AA batteries; they've lasted years)
 - Three momentary-contact switches (Select, Set, and Start keys)
 - One optoisolater (interface to camera) and a resistor (680 ohms)
 - One 32.768kHz watch crystal (TODO: see if capacitors help timing accuracy)
 - And some sort of jack compatible with your camera's remote shutter release port

You can optionally use a rotary encoder to make adjusting exposure
time and count super quick, just like using your camera. The encoder makes
the Set button unnecessary unless you want to set an arbitrary MM:SS exposure
time rather than use a fixed stop (and the encoder still helps you set these
values quickly).

The wiring is as follows (see KiCad schematic in hardware/)
 - PORTB0..3 (output) = Digit anode drivers
 - PORTB4    (output) = colon / apostrophe anodes
 - PORTB5    (output) = Optoisolator to camera
 - PORTB6..7          = 32.768kHz watch crystal
 - PORTC0    (input)  = Rotary encoder CLK
 - PORTC1    (input)  = Rotary encoder DT
 - PORTC2    (input)  = Rotary encoder SW
 - PORTC3    (input)  = Set key
 - PORTC4    (input)  = Select key
 - PORTC5    (input)  = Start key
 - PORTD0..7 (output) = Segment cathodes (PD7 = A, PD6 = B, ... PD0 = DP)

Compile with AVRGCC.

