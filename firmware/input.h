#pragma once

// resources used: timer1

void input_init();

#define BUTTON_START  0x01
#define BUTTON_SELECT 0x02
#define BUTTON_SET    0x04
#define BUTTON_HOLD   0x08     // one of the above buttons was held

// synthetic buttons made from click+turn combinations
#define BUTTON_BACK   0x10
#define BRIGHT_DOWN   0x20
#define BRIGHT_UP     0x40

// wait for the next input cycle (~50ms) and return input status
void input_poll(uint8_t *button_mask, int8_t *encoder_diff);
