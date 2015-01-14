#!/bin/sh
avrdude -p m48 -c usbtiny -U flash:w:MkII.hex
