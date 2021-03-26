astro-timer
===========

firmware for an ATMEGAx8-powered astrophotography timer

![astro-timer](https://user-images.githubusercontent.com/713453/112564756-73facf00-8da1-11eb-8924-9507fb7b1897.jpg)

---

This is the code behind a timer I built for astrophotography. The idea is, you
plug this thing into your camera's remote shutter release port, set the camera
to bulb mode, and let this thing take a series of long-exposure photos.

The exposure length, exposure count, time between exposures, and optionally
mirror lockup time can be set. Additionally, screen brightness can be adjusted,
and preferences can be saved in NVRAM.

Parts:
 - Atmel ATMEGA328P microcontroller (a prior version used ATMEGA48 and the code 
   still fits, but my original hardware was lost and my PCB house could surface-
   mount a '32 inexpensively)
 - 4-digit 7-segment LED display (code assumes common anode; PCB uses
   COM-09483 from SparkFun, model YSD-439AR6B-35. The Lite-ON LTC-4627 and
   Para Light A-394 should also be compatible).
 - Suitable resistors to drive the LEDs (I use 470 ohms)
 - 3V power supply (e.g. 2x AAA batteries)
 - Two momentary-contact switches (Set and Select keys)
 - One rotary encoder (for adjusting parameter values up and down) with switch (Start key)
 - Two NPN transistors (interface to camera) and base resistors (22K)
 - One 32.768kHz watch crystal, 6pf load capacitance
 - And some sort of jack compatible with your camera's remote shutter release port

The wiring is as follows (see KiCad schematic in hardware/)
 - PORTB0..3 (output) = Digit anode drivers
 - PORTB4    (output) = colon / apostrophe anodes
 - PORTB5    (output) = Transistor to camera (full press; tip in Canon connector)
 - PORTB6..7          = 32.768kHz watch crystal
 - PORTC0    (input)  = Rotary encoder CLK
 - PORTC1    (input)  = Rotary encoder DT
 - PORTC2    (input)  = Start key (on rotary encoder)
 - PORTC3    (input)  = Select key
 - PORTC4    (input)  = Set key
 - PORTC5    (output) = Transistor to camera (optional, half press; ring in Canon connector)
 - PORTD0..7 (output) = Segment cathodes (PD7 = A, PD6 = B, ... PD0 = DP)

Compile with AVRGCC.

