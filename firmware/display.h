#pragma once

// resources used: timer0

void display_init();

volatile uint8_t display[5];

#define LETTER_C 0b01100011
#define LETTER_L 0b11100011
#define LETTER_B 0b11000001
#define LETTER_S 0b01001001
#define LETTER_A 0b00010001
#define LETTER_V 0b10000011
#define LETTER_E 0b01100001
#define LETTER_O 0b00000011
#define LETTER_F 0b01110001
#define LETTER_v 0b11000111
#define LETTER_H 0b10010001
#define LETTER_1 0b10011111
#define LETTER_T 0b11100001
#define LETTER_P 0b00110001
#define DECIMAL  0b11111110
#define MINUS_SIGN 0b11111101

void DisplayAlnum(char letter, uint8_t num, uint8_t blink_mask, uint8_t dp);

#define HIGH_POS 0
#define LOW_POS  2
#define EMPTY '\xFF'
#define EXTRA_POS 4
// this is plexed with either segment A, or both dots separately to A and B, depending on the hardware
#define COLON 0b00111111
// this is plexed with either segment C or DP depending on the hardware
#define APOS 0b11011110

// strip - bit 0 = don't display tens place if num < 10
//         bit 1 = ...           ones place if num == 0
// dp = bit 0 = low digit decimal point on; bit 1 = high digit decimal point on
void DisplayNum(uint8_t num, uint8_t pos, uint8_t blink_mask, uint8_t strip, uint8_t dp);

// display a three digit number followed by a letter, optionally with decimal point and/or degree sign
// acceptable num range: -99 to 999; dp_pos is offset from left
void Display3(int16_t num, uint8_t letter, uint8_t dp_pos, uint8_t degree);

// display a byte in hex at the given position
void DisplayHex(uint8_t num, uint8_t pos);

// valid brightness levels: 0-5
void display_set_brightness(uint8_t bright);

// indeterminate progress indicator
void display_spin();
