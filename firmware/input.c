
// here's how button presses work:
// - a press is registered when a button is released.
// - a hold is registered when the same button has been down
//   for a specified number of cycles.  the button release
//   following the hold does not register.

#define BUTTON_SET    0x01
#define BUTTON_SELECT  0x02
#define BUTTON_START 0x04
#define BUTTON_HOLD  0x10     // button was held

#define REPEAT_THRESHOLD 15

uint8_t GetButtons()
{
    static uint8_t prevState = 0xff;
    static uint8_t repeat = 0;

    uint8_t curState = BUTTON_STATE();

    // if we've already registered a "hold"
    if (repeat >= REPEAT_THRESHOLD) {
        prevState = curState;
        if (curState == 7)
            repeat = 0;    // no buttons are down.
        return 0;
    }

    if (curState != prevState) {
        uint8_t pressed = ~prevState & curState;
        prevState = curState;
        return pressed;
    } else if (curState != 7) {
        // button(s) are being held
        if (++repeat == REPEAT_THRESHOLD) {
            return BUTTON_HOLD | ~(curState & 7);
        }
    }

    return 0;
}
