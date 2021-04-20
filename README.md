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
 - Press the Select button to cycle between parameters (exposure length, time between
   exposures, exposure count, and the options submenu).
   Alternatively, you can hold Select and rotate the knob to move through
   this menu in either direction.
 - The options submenu includes mirror lockup time, half-press setting (never,
   first shot in a series, every shot), brightness, encoder knob direction,
   and battery voltage).
 - Rotate the fancy control knob to adjust the currently visible parameter.
   If this parameter is exposure length or time between exposures, it will be adjusted
   in discrete stops.
 - It is possible to set the minutes and seconds to arbitrary values via the Set button.
   Press Set and the minutes value will flash. Turn the knob to set it to any value, then
   press Set again. The process will repeat for the seconds value.
 - Press the control knob in to start an exposure sequence, or enter/exit the options submenu.
 - You can adjust display brightness at any time by holding Set and turning the knob.
 - Press Set while looking at "Opts" to save current settings to non-volatile memory,
   where they will persist after changing batteries, etc.
 - While the exposure sequence is running,
   - Turn the control knob adjust the display brightness.
   - Press Select to toggle between displaying remaning time vs remaining
     exposure count.
   - Push the control knob to stop the exposure sequence.
 - Set the exposure count to 0 to take an unbounded number of shots. The counter will
   show the number of exposures complete, rather than the number remaining
   (i.e., counting up, not down).
 - Push and hold the control knob to turn the device off. (Press any button to turn it
   back on later.) The device will power itself down after 20 minutes of inactivity.
 
Parts:
 - Atmel ATmega328P microcontroller (the program fits in ATmega88 or larger).
 - 4-digit 7-segment LED display, common anode. The original PCB uses COM-09483 from SparkFun,
   model YSD-439AR6B-35. Other compatible devices include Lite-ON LTC-4627, Para Light A-394,
   Vishay TDCR1050M).
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

