////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"



#ifdef INCLUDE_CONTROLLER

// stateControllerPlay() and stateController() have been inlined into the state machine to save space



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
#ifdef INCLUDE_EXTENDED_CONTROLLER
    const char* menuItems[10] = {  PSTR("OFF"), cc_p, nrpn_p, rpn_p, PSTR("PC"), PSTR("BEND"), PSTR("AFTERTOUCH"), PSTR("A VOLTAGE"), PSTR("B VOLTAGE")};
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
#ifdef INCLUDE_EXTENDED_CONTROLLER
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
    uint16_t max = (type == CONTROL_TYPE_CC ? 127 : 16383);
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
#ifdef INCLUDE_EXTENDED_CONTROLLER
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
#ifdef INCLUDE_EXTENDED_CONTROLLER
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

/***
	LFO1: 	Same Function as Knob 1
	ENV1:	Same Function as Knob 2
	LFO2:	Same Function as Button 1
	ENV2:	Same Function as Button 2
	
	Middle button: swap LFO / ENV
	 
	LFO Control			[NO SINE]		Random, Saw Up, Saw Down, Square (PWM?), Triangle
	Left Knob: Rate / PWM
	Right Knob: Amplitude / Wave Shape
	Right Button: Toggle Knob Function
	Middle Button Long: Off / Constant / Start on any Note On / Stop on All Notes Off
	
	For Saw Up/Down and Triangle, "PWM" means how long the wave is fully ON or fully OFF.
	For example, PWM of 25% for Saw Up means that we stay at 0 for 25% of the wave, then
	start moving up to the top for the remaining 75%.  PWM of 75% means that we start moving
	up to the top for 75%, then for 25% we stay at the top.
	
	For Random, "PWM" means how random we are -- the jump distance.  PWM of 1% means we choose
	between (say) +1 or -1 from the current value.  PWM of 10% means we choose a value between
	+13 and -13 away from the current value.  PWM of 100% means we choose a value between +127
	and -127 away from the currenrt value.  If the value is out of range, we select a new one.
			
	Env Control
	Left knob: Attack / Sustain / Delay
	Right knob: Decay / Release / Amplitude
	Right Button: Toggle Knob Function
	Right Button Long: Off / Start on any Note On / Stop on All Notes Off
*/

#endif
