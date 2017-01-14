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
// Other CC parameters permit values from 0...127               (Shifted << 7)
// NRPN/RPN permit values from 0...16383.  Also, they may be "Increment" (CONTROL_VALUE_INCREMENT << 7) or "Decrement" (CONTROL_VALUE_DECREMENT << 7)
// PC permits values from 0...127 (Shifted << 7)
// VOLTAGE_A and VOLTAGE_B permit values from 0...16383 but shift them >> 2 to convert them to 0...4095, which is the DAC resolution

void sendControllerCommand(uint8_t commandType, uint16_t commandNumber, uint16_t fullValue)
    {
    uint8_t msb = fullValue >> 7;
    uint8_t lsb = fullValue & 127;
    
    // amazingly, putting this here reduces the code by 4 bytes.  Though it could
    // easily be removed as the switch below handles it!
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
            // There are two kinds of CC messages: ones with an MSB only,
            // and ones with an MSB + [optional] LSB.  The second kind are messages whose
            // command number is 0...31.  The first kind are messages > 31.
            
            MIDI.sendControlChange(commandNumber, msb, options.channelOut);
            if (commandNumber < 32)             // send optional lsb if appropriate
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
            MIDI.sendControlChange(101, 127, options.channelOut);  // MSB of NULL command
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
            MIDI.sendControlChange(101, 127, options.channelOut);  // MSB of NULL command
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
#if defined(__AVR_ATmega2560__)        
		case CONTROL_TYPE_PITCH_BEND:
			{
			// move from 0...16k to -8k...8k
			MIDI.sendPitchBend(((int)fullValue) - MIDI_PITCHBEND_MIN, options.channelOut);
			}
		break;
		case CONTROL_TYPE_AFTERTOUCH:
			{
			MIDI.sendAfterTouch(msb, options.channelOut);
			}
		break;
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
#if defined(__AVR_ATmega2560__)
	const char* menuItems[9] = {  PSTR("OFF"), cc_p, nrpn_p, rpn_p, PSTR("PC"), PSTR("BEND"), PSTR("AFTERTOUCH"), PSTR("A VOLTAGE"), PSTR("B VOLTAGE")};
	result = doMenuDisplay(menuItems, 9, STATE_NONE,  STATE_NONE, 1);
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
#if defined(__AVR_ATmega2560__)
            else if ((type == CONTROL_TYPE_PC || type == CONTROL_TYPE_VOLTAGE_A || type == CONTROL_TYPE_VOLTAGE_B || type == CONTROL_TYPE_PITCH_BEND || type == CONTROL_TYPE_AFTERTOUCH))
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
                local.control.doIncrement = (type == CONTROL_TYPE_NRPN || type == CONTROL_TYPE_RPN);
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
// Lets the user set a controller number for the given controller type.   
// This is stored in &number.  The user can cancel everything and the type and
// number will be reset.
void setControllerButtonOnOff(uint16_t &onOff, int8_t nextState)
    {
    if (entry) 
        {
        backupOptions = options;
        }
    
    // On/Off options are stored as 0 = Off, 1 = value 0, 2 = value 1, ..., 128 = value 127, 129 = CONTROL_VALUE_INCREMENT.
    // So we have to shift things here.
        
    uint8_t result;
    
    if (local.control.doIncrement)
        // note that if it's the MIDDLE BUTTON we do decrement, else we do increment
        result = doNumericalDisplay(-1, CONTROL_VALUE_INCREMENT, ((int16_t)onOff) - 1, true, 
            (((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) ? OTHER_DECREMENT: OTHER_INCREMENT);
#if defined(__AVR_ATmega2560__) 
    else if (	
    			((((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) &&
    		 		(options.middleButtonControlType == CONTROL_TYPE_PITCH_BEND)) ||
    			((((&onOff) == (&options.selectButtonControlOn)) || ((&onOff) == (&options.selectButtonControlOff))) &&
    		 		(options.selectButtonControlType == CONTROL_TYPE_PITCH_BEND))
    		 )  // ugh, all this work just to determine if we're doing pitch bend YUCK
        result = doNumericalDisplay(MIDI_PITCHBEND_MIN - 1, MIDI_PITCHBEND_MAX, 0, true, OTHER_NONE);
#endif
    else
        result = doNumericalDisplay(-1, CONTROL_VALUE_INCREMENT - 1, ((int16_t)onOff) - 1, true, OTHER_NONE);

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
#if defined(__AVR_ATmega2560__) 
            if (
            		((((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) &&
    		 			(options.middleButtonControlType == CONTROL_TYPE_PITCH_BEND)) ||
    				((((&onOff) == (&options.selectButtonControlOn)) || ((&onOff) == (&options.selectButtonControlOff))) &&
    		 			(options.selectButtonControlType == CONTROL_TYPE_PITCH_BEND))
	    		 )  // ugh, all this work just to determine if we're doing pitch bend YUCK
            	{
            	if (currentDisplay == MIDI_PITCHBEND_MIN - 1)
            		onOff = 0;
            	else
	            	onOff = (uint16_t) (currentDisplay - MIDI_PITCHBEND_MIN) + 1;
            	}
            else
#endif
            	{
	            onOff = (uint8_t) (currentDisplay + 1);
	            }
            }
        break;
        case MENU_SELECTED:
            {
            if (nextState == STATE_CONTROLLER)  // we're all done
                {
                local.control.doIncrement = false;
                saveOptions();
                }
            goDownState(nextState);
            }
        break;
        case MENU_CANCELLED:
            {
            local.control.doIncrement = false;
            goDownStateWithBackup(STATE_CONTROLLER);
            }
        break;
        }
    }

/*** LFOS AND ENVELOPES

2 LFOs, tied to the BUTTON VALUES: Max is BUTTON ON, Min is BUTTON OFF, if either button is OFF, nothing is done?
	- RATE
	- WAVE
	- RESET ON NOTE ON

2 ENVs, tied to the KNOB VALUES: Max is KNOB 

*/
