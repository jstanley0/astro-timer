#pragma once

// resources used: timer1

void input_init();

#define BUTTON_START  0x01
#define BUTTON_SELECT 0x02
#define BUTTON_SET    0x04
#define BUTTON_HOLD   0x10     // button was held

// not a real button; simulated by pressing Select and turning the encoder CCW
// (pressing Select and turning the encoder CW just emits a Select)
#define BUTTON_BACK   0x40

// wait for the next input cycle (~50ms) and return input status
void input_poll(uint8_t *button_mask, int8_t *encoder_diff);
