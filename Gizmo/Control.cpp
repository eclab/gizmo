#include "All.h"


// stateControllerPlay() and stateController() have been inlined into the state machine to save space


////// BUTTON TOGGLES
//
// The Control application emits different things depending on what the "toggle state" of the
// buttons are when they are pushed.  

GLOBAL uint8_t middleButtonToggle;
GLOBAL uint8_t selectButtonToggle;  // perhaps these two could be compressed, they're just booleans


// SEND CONTROLLER COMMAND
// Sends a controller command when the user modifies a button or pot.  Command types can be any
// of the CONTROL_TYPE_* constants above.  RPN and NRPN take 7-bit numbers but shift them by 7
// into the MSB.  RPN and NRPN have 14-bit command numbers of course.
// CC permits command numbers 0...119.  PC has no command number.  PC and CC have command values 
// 0...127.   Also sending to VOLTAGE assumes you're providing a value 0...1023, and the command 
// number is ignored.
void sendControllerCommand(uint8_t commandType, uint16_t commandNumber, uint8_t value)
    {
    if (commandType == CONTROL_TYPE_OFF)
        return;
    
    if (options.channelOut == 0)                // we do not output
        return;
    
    clearScreen();
    if (commandType != CONTROL_TYPE_OFF) 
        writeShortNumber(led, value, false);           
    
    switch(commandType)
        {
        case CONTROL_TYPE_OFF:
            {
            // do nothing
            }
        break;
        case CONTROL_TYPE_CC:
            {
            if (value < CONTROL_VALUE_INCREMENT)  // if it's 128, we ignore it, it's wrong
                MIDI.sendControlChange(commandNumber, value, options.channelOut);
            TOGGLE_OUT_LED(); 
            }
        break;
        case CONTROL_TYPE_NRPN:
            {
            // Send 99 for NRPN, or 101 for RPN
            MIDI.sendControlChange(99, commandNumber >> 7, options.channelOut);
            MIDI.sendControlChange(98, commandNumber & 127, options.channelOut);  // LSB
            if (value >= CONTROL_VALUE_INCREMENT)
                {
                MIDI.sendControlChange(96, 1, options.channelOut);  // MSB
                }
            else
                {
                MIDI.sendControlChange(6, value, options.channelOut);  // MSB
                //MIDI.sendControlChange(38, ..., options.channelOut);  // We don't send the LSB
                }
            MIDI.sendControlChange(101, 127, options.channelOut);  // LSB of NULL command
            MIDI.sendControlChange(100, 127, options.channelOut);  // LSB of NULL command
            TOGGLE_OUT_LED(); 
            }
        break;
        // note merging these actually loses bytes.  I tried.
        case CONTROL_TYPE_RPN:
            {
            MIDI.sendControlChange(101, commandNumber >> 7, options.channelOut);
            MIDI.sendControlChange(100, commandNumber & 127, options.channelOut);  // LSB
            if (value >= CONTROL_VALUE_INCREMENT)
                {
                MIDI.sendControlChange(96, 1, options.channelOut);  // MSB
                }
            else
                {
                MIDI.sendControlChange(6, value, options.channelOut);  // MSB
                //MIDI.sendControlChange(38, ..., options.channelOut);  // We don't send the LSB
                }
            MIDI.sendControlChange(101, 127, options.channelOut);  // LSB of NULL command
            MIDI.sendControlChange(100, 127, options.channelOut);  // LSB of NULL command
            TOGGLE_OUT_LED(); 
            }
        break;
        case CONTROL_TYPE_PC:
            {
            if (value < CONTROL_VALUE_INCREMENT)
                MIDI.sendProgramChange(value, options.channelOut);
            TOGGLE_OUT_LED(); 
            }
        break;
#if defined(__AVR_ATmega2560__)
        case CONTROL_TYPE_VOLTAGE_A:
            {
            setPot(DAC_A, value);
            }
        break;
        case CONTROL_TYPE_VOLTAGE_B:
            {
            setPot(DAC_B, value);
            }
        break;
#endif
        }
    }





// SET CONTROLLER TYPE
// Lets the user set a controller type.   This is stored in &type.  When the user is finished
// this function will go to the provided nextState (typically to set the controller number).
void setControllerType(uint8_t &type, uint8_t nextState, uint8_t buttonOnState)
    {
    int8_t result;
    if (entry) 
        {
        backupOptions = options; 
        }
#if defined(__AVR_ATmega2560__)
    const char* menuItems[7] = {  PSTR("OFF"), cc_p, nrpn_p, rpn_p, PSTR("PC"), PSTR("A VOLTAGE"), PSTR("B VOLTAGE")};
    result = doMenuDisplay(menuItems, 7, STATE_NONE,  STATE_NONE, 1);
#else
    const char* menuItems[5] = {  PSTR("OFF"), cc_p, nrpn_p, rpn_p, PSTR("PC")};
    result = doMenuDisplay(menuItems, 5, STATE_NONE, STATE_NONE, 1);
#endif
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            type = currentDisplay;
            }
        break;
        case MENU_SELECTED:
            {
            if (type == CONTROL_TYPE_OFF)
                {
                saveOptions();
                goUpState(STATE_CONTROLLER);
                }
            else if ((type == CONTROL_TYPE_PC || type == CONTROL_TYPE_VOLTAGE_A || type == CONTROL_TYPE_VOLTAGE_B))
                {
                if (buttonOnState != STATE_NONE)                // it's a button, we need to get button values
                    {
                    goDownState(buttonOnState);
                    }
                else
                    {
                    saveOptions();
                    goUpState(STATE_CONTROLLER);
                    }
                }
            else // CC, NRPN, or RPN, we need to get a number and maybe button values
                {
                goDownState(nextState);
                }
            }
        break; 
        case MENU_CANCELLED:
            {
            goUpStateWithBackup(STATE_CONTROLLER);
            }
        break;
        }
    }

// This is set to 1 when we're doing NRPN or RPN, and doing buttons and need to display increment as an option
static uint8_t doIncrement;

// SET CONTROLLER NUMBER
// Lets the user set a controller number for the given conroller type.   
// This is stored in &number.  The user can cancel everything and the type and
// number will be reset.
void setControllerNumber(uint8_t type, uint16_t &number, uint8_t backupType, uint16_t backupNumber, uint8_t nextState)
    {
    uint16_t max = (type == CONTROL_TYPE_CC ? 119 : 16383);
    uint8_t result = doNumericalDisplay(0, max, number, 0, OTHER_NONE);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            number = currentDisplay;
            }
        break;
        case MENU_SELECTED:
            {
            if (nextState == STATE_CONTROLLER)  // we're not doing buttons
                saveOptions();
            else                                                                // we're doing buttons and either NRPN or RPN
                doIncrement = (type == CONTROL_TYPE_NRPN || type == CONTROL_TYPE_RPN);
            goDownState(nextState);
            }
        break;
        case MENU_CANCELLED:
            {
            goDownStateWithBackup(STATE_CONTROLLER);
            }
        break;
        }
    }

// SET CONTROLLER BUTTON ON OFF
// Lets the user set a controller number for the given conroller type.   
// This is stored in &number.  The user can cancel everything and the type and
// number will be reset.
void setControllerButtonOnOff(uint8_t &onOff, int8_t backupOnOff, int8_t nextState)
    {
    if (entry) 
        {
        backupOptions = options;
        }
    
    // On/Off options are stored as 0 = Off, 1 = value 0, 2 = value 1, ..., 128 = value 127, 129 = CONTROL_VALUE_INCREMENT.
    // So we have to shift things here.
        
    uint8_t result;
    
    if (doIncrement)
        // note that if it's the MIDDLE BUTTON we do decrement, else we do increment
        result = doNumericalDisplay(-1, CONTROL_VALUE_INCREMENT, ((int16_t)onOff) - 1, true, 
            (((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) ? OTHER_DECREMENT: OTHER_INCREMENT);
    else
        result = doNumericalDisplay(-1, CONTROL_VALUE_INCREMENT - 1, ((int16_t)onOff) - 1, true, OTHER_NONE);

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            onOff = (uint8_t) (currentDisplay + 1);
            }
        break;
        case MENU_SELECTED:
            {
            if (nextState == STATE_CONTROLLER)  // we're all done
                {
                doIncrement = false;
                saveOptions();
                }
            goDownState(nextState);
            }
        break;
        case MENU_CANCELLED:
            {
            doIncrement = false;
            goDownStateWithBackup(STATE_CONTROLLER);
            }
        break;
        }
    }
            
