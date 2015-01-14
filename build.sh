#!/bin/sh
avr-gcc -Wall -Os -DF_CPU=1000000 -mmcu=atmega48 -c MkII.c -o MkII.o
avr-gcc -Wall -Os -DF_CPU=1000000 -mmcu=atmega48 -o MkII.elf MkII.o
rm -f MkII.hex
avr-objcopy -j .text -j .data -O ihex MkII.elf MkII.hex
avr-size --format=avr --mcu=atmega48 MkII.elf
