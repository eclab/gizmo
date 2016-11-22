#ifndef __MIDI_SHIELD__
#define __MIDI_SHIELD__

// Pinouts
// These are the digital and analog pinouts of various things on the MIDI shield
#define PIN_LED_RED 7
#define PIN_LED_GREEN 6
#define PIN_BACK_BUTTON 4
#define PIN_MIDDLE_BUTTON 3
#define PIN_SELECT_BUTTON 2
#define PIN_LEFT_POT    (A0)
#define PIN_RIGHT_POT   (A1)

extern uint8_t potPin[2]; //  = { PIN_LEFT_POT, PIN_RIGHT_POT };

// Masks for the red and green LEDs and the three buttons
extern uint8_t LED_RED_mask;
extern uint8_t LED_GREEN_mask;
extern uint8_t BACK_BUTTON_mask;
extern uint8_t MIDDLE_BUTTON_mask;
extern uint8_t SELECT_BUTTON_mask;

// Ports for the red and green LEDs and the three buttons
extern volatile uint8_t *port_LED_RED;
extern volatile uint8_t *port_LED_GREEN;
extern volatile uint8_t *port_BACK_BUTTON;
extern volatile uint8_t *port_MIDDLE_BUTTON;
extern volatile uint8_t *port_SELECT_BUTTON;

// Toggles the MIDI IN LED (red)
#define TOGGLE_IN_LED() (*(port_LED_RED) ^= (LED_RED_mask))

// Toggles the MIDI OUT LED (green)
#define TOGGLE_OUT_LED() (*(port_LED_GREEN) ^= (LED_GREEN_mask))

#endif

