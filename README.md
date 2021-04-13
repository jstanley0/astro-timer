astro-timer
===========

hardware and firmware for an ATMEGAx8-powered astrophotography timer

![astro-timer](https://user-images.githubusercontent.com/713453/112564756-73facf00-8da1-11eb-8924-9507fb7b1897.jpg)

![astro-timer in a 3D-printed enclosure](https://user-images.githubusercontent.com/713453/114489144-de987100-9bcf-11eb-8922-67403adf2154.jpg)

---

This device controls a camera (in bulb mode) for a sequence of light frames
for astrophotography. The camera is connected to the 2.5mm jack.
The exposure length, exposure count, time between exposures, and optionally
mirror lockup time can be set. Additionally, screen brightness can be adjusted,
and preferences can be saved in NVRAM.

Controls (from left to right):
 - Set button
 - Select button
 - Fancy control knob

Usage:
 - The select button cycles between parameters (exposure length, time between
   exposures, exposure count, mirror lockup time, brightness)
 - When rotated, the fancy control knob adjusts the currently visible parameter.
   If this parameter is exposure length or time between exposures, it adjusts
   it one stop at a time. It is possible to set the minutes and seconds to
   arbitrary values via the Set button.
 - When pressed, the fancy control knob starts or stops the exposure sequence.
 - While the exposure sequence is running,
   - The set button adjusts the screen brightness.
   - The select button toggles between displaying remaning time vs remaining
     exposure count.

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

