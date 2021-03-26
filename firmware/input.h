#pragma once

// resources used: timer1

void input_init();

#define BUTTON_START  0x01
#define BUTTON_SELECT 0x02
#define BUTTON_SET    0x04
#define BUTTON_HOLD   0x10     // button was held

// wait for the next input cycle (~50ms) and return input status
void input_poll(uint8_t *button_mask, int8_t *encoder_diff);
