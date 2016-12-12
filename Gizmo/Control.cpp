////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"


// stateControllerPlay() and stateController() have been inlined into the state machine to save space



// SEND CONTROLLER COMMAND
// Sends a controller command when the user modifies a button or pot.
//
// PERMITTED COMMAND TYPES
// Command types can be any of the CONTROL_TYPE_* constants above. 
// 
// PERMITTED COMMAND NUMBERS (PARAMETERS)
// CC permits command numbers 0...119, though obviously many of those should NOT be done.
// NRPN/RPN permit command numbers from 0...16383
// PC has no command number, nor does VOLTAGE_A or VOLTAGE_B.
//
// PERMITTED COMMAND VALUES
// All values are shifted to MSB+LSB, that is, have a range 0...16383, but its interpretation varies
// CC parameters 0...31 permit values from 0...16383
// Other CC parameters permit values from 0...127		(Shifted << 7)
// NRPN/RPN permit values from 0...16383.  Also, they may be "Increment" (CONTROL_VALUE_INCREMENT << 7) or "Decrement" (CONTROL_VALUE_DECREMENT << 7)
// PC permits values from 0...127 (Shifted << 7)
// VOLTAGE_A and VOLTAGE_B permit values from 0...16383 but shift them >> 2 to convert them to 0...4095, which is the DAC resolution

void sendControllerCommand(uint8_t commandType, uint16_t commandNumber, uint16_t fullValue)
    {
    uint8_t msb = fullValue >> 7;
    uint8_t lsb = fullValue & 127;
    
    if (commandType == CONTROL_TYPE_OFF)
        return;
    
    if (options.channelOut == 0)                // we do not output
        return;
    
    // TopLevel.cpp STATE_CONTROLLER_PLAY erases this anyway
    //clearScreen();
    //if (commandType != CONTROL_TYPE_OFF) 
     //   writeShortNumber(led, value, false);           
    
    switch(commandType)
        {
        case CONTROL_TYPE_OFF:
            {
            // do nothing
            }
        break;
        case CONTROL_TYPE_CC:
            {
            MIDI.sendControlChange(commandNumber, msb, options.channelOut);
            if (commandNumber < 32)		// send optional lsb if appropriate
            	{
            	MIDI.sendControlChange(commandNumber + 32, lsb, options.channelOut);
            	}
            TOGGLE_OUT_LED(); 
            }
        break;
        case CONTROL_TYPE_NRPN:
            {
            // Send 99 for NRPN, or 101 for RPN
            MIDI.sendControlChange(99, commandNumber >> 7, options.channelOut);
            MIDI.sendControlChange(98, commandNumber & 127, options.channelOut);  // LSB
            if (msb == CONTROL_VALUE_INCREMENT)
                {
                MIDI.sendControlChange(96, 1, options.channelOut);
                }
            else if (msb == CONTROL_VALUE_DECREMENT)
            	{
                MIDI.sendControlChange(97, 1, options.channelOut);
            	}
            else
                {
                MIDI.sendControlChange(6, msb, options.channelOut);  // MSB
                MIDI.sendControlChange(38, lsb, options.channelOut);  // LSB
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
            if (msb == CONTROL_VALUE_INCREMENT)
                {
                MIDI.sendControlChange(96, 1, options.channelOut);
                }
            else if (msb == CONTROL_VALUE_DECREMENT)
                {
                MIDI.sendControlChange(97, 1, options.channelOut);
                }
            else
                {
                MIDI.sendControlChange(6, msb, options.channelOut);  // MSB
                MIDI.sendControlChange(38, lsb, options.channelOut);  // LSB
                }
            MIDI.sendControlChange(101, 127, options.channelOut);  // LSB of NULL command
            MIDI.sendControlChange(100, 127, options.channelOut);  // LSB of NULL command
            TOGGLE_OUT_LED(); 
            }
        break;
        case CONTROL_TYPE_PC:
            {
            if (msb <= MAXIMUM_PC_VALUE)
                MIDI.sendProgramChange(msb, options.channelOut);
            TOGGLE_OUT_LED(); 
            }
        break;
#ifdef VOLTAGE        
        case CONTROL_TYPE_VOLTAGE_A:
            {
            setValue(DAC_A, fullValue >> 2);
            //setPot(DAC_A, value);
            }
        break;
        case CONTROL_TYPE_VOLTAGE_B:
            {
            setValue(DAC_B, fullValue >> 2);
            //setPot(DAC_B, value);
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
#ifdef VOLTAGE
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
#ifdef VOLTAGE
            else if ((type == CONTROL_TYPE_PC || type == CONTROL_TYPE_VOLTAGE_A || type == CONTROL_TYPE_VOLTAGE_B))
#else
            else if (type == CONTROL_TYPE_PC)
#endif
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
            
