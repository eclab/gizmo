#include "All.h"

// Masks for the red and green LEDs and the three buttons
GLOBAL uint8_t LED_RED_mask;
GLOBAL uint8_t LED_GREEN_mask;
GLOBAL uint8_t BACK_BUTTON_mask;
GLOBAL uint8_t MIDDLE_BUTTON_mask;
GLOBAL uint8_t SELECT_BUTTON_mask;

// Ports for the red and green LEDs and the three buttons
GLOBAL volatile uint8_t *port_LED_RED;
GLOBAL volatile uint8_t *port_LED_GREEN;
GLOBAL volatile uint8_t *port_BACK_BUTTON;
GLOBAL volatile uint8_t *port_MIDDLE_BUTTON;
GLOBAL volatile uint8_t *port_SELECT_BUTTON;

GLOBAL uint8_t potPin[2] = { A0, A1 };

