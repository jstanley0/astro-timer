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

// valid brightness levels: 0-3
void display_set_brightness(uint8_t bright);
