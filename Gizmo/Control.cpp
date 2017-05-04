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
#ifdef INCLUDE_EXTENDED_CONTROLLER
				if (nextState == STATE_CONTROLLER_PLAY_WAVE_NUMBER)
					goUpState(STATE_CONTROLLER_MODULATION);  // it's the wave envelope
				else
#endif
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
#ifdef INCLUDE_EXTENDED_CONTROLLER
					if (nextState == STATE_CONTROLLER_PLAY_WAVE_NUMBER)
						goUpState(STATE_CONTROLLER_MODULATION);  // it's the wave envelope
					else
#endif
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
#ifdef INCLUDE_EXTENDED_CONTROLLER
			if (nextState == STATE_CONTROLLER_PLAY_WAVE_NUMBER)
				goUpStateWithBackup(STATE_CONTROLLER_MODULATION);  // it's the wave envelope
			else
#endif
				goUpStateWithBackup(STATE_CONTROLLER);
            }
        break;
        }
    }


// SET CONTROLLER NUMBER
// Lets the user set a controller number for the given controller type.   
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
            if (nextState == STATE_CONTROLLER 
#ifdef INCLUDE_EXTENDED_CONTROLLER
            || nextState == STATE_CONTROLLER_MODULATION
#endif
            )  // we're not doing buttons
                saveOptions();
            else                                                                // we're doing buttons and either NRPN or RPN
                local.control.doIncrement = (type == CONTROL_TYPE_NRPN || type == CONTROL_TYPE_RPN);
            goDownState(nextState);
            }
        break;
        case MENU_CANCELLED:
            {
#ifdef INCLUDE_EXTENDED_CONTROLLER
            if (nextState == STATE_CONTROLLER_PLAY_WAVE_NUMBER)
            	goDownStateWithBackup(STATE_CONTROLLER_MODULATION);  // it's the wave envelope
            else
#endif
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
    

#ifdef INCLUDE_EXTENDED_CONTROLLER

void stateControllerModulationSetMode()
    {
    if (entry) 
        {
        backupOptions = options;
        }

    const char* menuItems[4] = {  PSTR("GATED"), PSTR("TRIGGERED"), PSTR("LOOPED"), PSTR("FREE") };
    uint8_t result = doMenuDisplay(menuItems, 4, STATE_NONE, STATE_NONE, 1);

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            if (options.envelopeMode != currentDisplay)
           		{
           		options.envelopeMode = currentDisplay;
            	saveOptions();
            	}
            }
        // FALL THRU
        case MENU_CANCELLED:
            {
            goUpStateWithBackup(STATE_CONTROLLER_MODULATION);
            }
        break;
        }
    }



// SET WAVE ENVELOPE
// Lets the user set a controller type.   This is stored in &type.  When the user is finished
// this function will go to the provided nextState (typically to set the controller number).
void setWaveEnvelope()
    {
    const char* menuItems[16] = { PSTR("1 VALUE"), PSTR("1 LENGTH"), PSTR("2 VALUE"), PSTR("2 LENGTH"), PSTR("3 VALUE"), PSTR("3 LENGTH"), PSTR("4 VALUE"), PSTR("4 LENGTH"), PSTR("5 VALUE"), PSTR("5 LEN"), PSTR("6 VALUE"), PSTR("6 LENGTH"), PSTR("7 VALUE"), PSTR("7 LENGTH"), PSTR("8 VALUE"), PSTR("8 LENGTH") };
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, STATE_NONE, 1);

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
			local.control.waveEnvelopeIndex = currentDisplay;  // get rid of "GO"
			goDownState(STATE_CONTROLLER_SET_WAVE_ENVELOPE_VALUE);
            }
        break; 
        case MENU_CANCELLED:
            {
            goUpState(STATE_CONTROLLER_MODULATION);
            }
        break;
        }
    }

#define WAVEVAL(i) (options.waveEnvelope[(i) * 2])
#define WAVETIME(i) (options.waveEnvelope[(i) * 2 + 1])

uint32_t computeWaveEndTicks(uint8_t index)
	{
	if (options.controlModulationClocked)
		{
		// Beats are 2 a second at 120bpm.  We have 8 steps a beat, or 
		// 16 a second.  MIDI clock pulses are 48 a second at 120bpm.  So
		// our steps are 1 every 3 clock pulses.
		return local.control.waveStartTicks + WAVETIME(index) * 3;
		}
	else
		{
		// We want to be approximately the same as 120bpm envelopes.  So we need
		// 16 steps a second.  Ticks are 3125 a second, we want approximately 195 ticks per step.
		return local.control.waveStartTicks + WAVETIME(index) * 195;
		}
	}
	
uint16_t computeWaveValue(uint8_t startindex, uint8_t endindex)
	{
	// (currenttime - starttime) / (endtime - starttime) = (currentval - startval) / (endval - startval)
	// so currentval = (currenttime - starttime) / (endtime - starttime) * (endval - startval) + startval
	// Note that the vals are 1 off, so that's why we're subtracting 1
	float _currentWaveControl = (float)((options.controlModulationClocked? pulseCount : tickCount) - local.control.waveStartTicks) / 
								(float)(local.control.waveEndTicks - local.control.waveStartTicks) * 
								(float)((WAVEVAL(endindex) << 7) - (WAVEVAL(startindex) << 7)) +
								(float)(WAVEVAL(startindex) << 7);
		
	if (_currentWaveControl < 0) _currentWaveControl = 0;
	if (_currentWaveControl > 16383) _currentWaveControl = 16383;
	
	uint16_t currentWaveControl = (uint16_t) _currentWaveControl;
	if (options.waveControlType == CONTROL_TYPE_CC || 
		options.waveControlType == CONTROL_TYPE_PC ||
		options.waveControlType == CONTROL_TYPE_AFTERTOUCH)  // we're only doing 7 bit, strip off the LSB
		{
		currentWaveControl = (currentWaveControl >> 7) << 7;
		}

	return currentWaveControl;
	}


void resetWaveEnvelope(uint8_t index)
	{
	local.control.wavePosition = index;
	local.control.currentWaveControl = WAVEVAL(local.control.wavePosition) << 7;
	sendControllerCommand(options.waveControlType, options.waveControlNumber, local.control.currentWaveControl, options.channelOut);
	local.control.waveCountDown = WAVE_COUNTDOWN;
	local.control.waveStartTicks = (options.controlModulationClocked? pulseCount : tickCount);
	local.control.waveEndTicks = computeWaveEndTicks(local.control.wavePosition);
	}

void playWaveEnvelope()
	{
    if (entry) 
        {
    	local.control.wavePosition = -1;
    	local.control.lastWavePosition = -1;
    	local.control.noteOnCount = 0;
    	local.control.waveCountDown = WAVE_COUNTDOWN;
    	entry = false;
        }

    if (local.control.waveCountDown > 0)
    	local.control.waveCountDown--;
    	
	if (newItem && itemType == MIDI_NOTE_OFF)
		{
		MIDI.sendNoteOff(itemNumber, itemValue, itemChannel);
 		if (local.control.noteOnCount > 0)
			{
			local.control.noteOnCount--;
			}
		if ((local.control.noteOnCount == 0) && (options.envelopeMode == ENVELOPE_MODE_GATED || options.envelopeMode == ENVELOPE_MODE_LOOPED))
			{
			local.control.wavePosition = -1;
			}
		}
    
    // If we haven't started yet and got a MIDI_NOTE_ON *or* we're free
	if (local.control.wavePosition == -1 && ((newItem && itemType == MIDI_NOTE_ON) || (options.envelopeMode == ENVELOPE_MODE_FREE)))
		{
		// Note that we've hacked updateMIDI so that we get Note On data but everything
		// else is passed through.  FIXME -- need a better hack
		
		resetWaveEnvelope(0);
		}
	else if (local.control.wavePosition >= 0  && local.control.waveCountDown == 0)	// we've begun playing.  Need to update.
		{
		if ((options.controlModulationClocked? pulseCount : tickCount) >= local.control.waveEndTicks)   // Have we completed a stage?  
			{
			// Move to the next stage
			local.control.wavePosition++;
			if ((options.envelopeMode == ENVELOPE_MODE_GATED || options.envelopeMode == ENVELOPE_MODE_TRIGGERED) &&
				(local.control.wavePosition == (ENVELOPE_SIZE - 1) || WAVETIME(local.control.wavePosition) == ENVELOPE_END))
				{
				// we're at the end of one-shot, do one last controller command, sort of a semi reset envelope
				local.control.currentWaveControl = WAVEVAL(local.control.wavePosition) << 7;
				sendControllerCommand(options.waveControlType, options.waveControlNumber, local.control.currentWaveControl, options.channelOut);
				local.control.waveCountDown = WAVE_COUNTDOWN;
				
				// now reset
				local.control.lastWavePosition = local.control.wavePosition;  // so we display it right
				local.control.wavePosition = -1;
				}
			else if ((options.envelopeMode == ENVELOPE_MODE_LOOPED || options.envelopeMode == ENVELOPE_MODE_FREE) &&
					 (local.control.wavePosition == ENVELOPE_SIZE || WAVETIME(local.control.wavePosition) == ENVELOPE_END))
				{
				// we're at the end of a loop
				resetWaveEnvelope(0);
				}
			else
				{
				// we're just going to the next stage
				resetWaveEnvelope(local.control.wavePosition);
				}
			}
		else  // we're in the middle of a stage
			{
			uint8_t endPos = local.control.wavePosition + 1;
			if ((options.envelopeMode == ENVELOPE_MODE_LOOPED || options.envelopeMode == ENVELOPE_MODE_FREE) &&			/// we're looping AND
				(local.control.wavePosition == (ENVELOPE_SIZE - 1) || WAVETIME(local.control.wavePosition + 1) == ENVELOPE_END))					// it's the last wave
					{
					endPos = 0;
					}
			uint16_t currentWaveControl = computeWaveValue(local.control.wavePosition, endPos);
			
			// Is this a different value than before?  If so, we update
			if (currentWaveControl != local.control.currentWaveControl)
				{
				local.control.currentWaveControl = currentWaveControl;
				sendControllerCommand(options.waveControlType, options.waveControlNumber, local.control.currentWaveControl, options.channelOut);
				local.control.waveCountDown = WAVE_COUNTDOWN;
				}
			}
		}

		// see hack above
    	if (newItem && itemType == MIDI_NOTE_ON)
    		{
    		MIDI.sendNoteOn(itemNumber, itemValue, itemChannel); 
    		if (local.control.noteOnCount < 255)
    			local.control.noteOnCount++;
			}
			
    if (updateDisplay)
        {
        if (local.control.wavePosition != -1)
        	local.control.lastWavePosition = local.control.wavePosition;
        	
        clearScreen();
        if (local.control.lastWavePosition == -1)
        	{
        	write3x5Glyphs(GLYPH_NONE);
        	}
        else 
        	{
        	writeShortNumber(led, local.control.lastWavePosition + 1, false);
        	writeShortNumber(led2, (uint8_t)(local.control.currentWaveControl >> 7), true);
        	}
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_CONTROLLER_MODULATION);
        }

	}




// SET WAVE ENVELOPE VALUE
// Lets the user set a controller type.   This is stored in &type.  When the user is finished
// this function will go to the provided nextState (typically to set the controller number).
void setWaveEnvelopeValue()
    {
    if (entry) 
        {
        backupOptions = options;
        }

	uint8_t result;
	if ((local.control.waveEnvelopeIndex & 1) == 0)  // EVEN: value
		{
		result = doNumericalDisplay(0, 127, options.waveEnvelope[local.control.waveEnvelopeIndex], false, false);
		}
	 else if (local.control.waveEnvelopeIndex == 1)  // first envelope not permitted to be off
	 	{
		result = doNumericalDisplay(0, ENVELOPE_END - 1, options.waveEnvelope[local.control.waveEnvelopeIndex], false, false);
	 	}
	 else // ODD : time
		{
		// -1 -> 255 (ENVELOPE_END) == STOP HERE
		result = doNumericalDisplay(-1, ENVELOPE_END - 1, 			
			// if the envelope is 255, change to -1.  Otherwise keep as it is (0...254)
			options.waveEnvelope[local.control.waveEnvelopeIndex] == ENVELOPE_END ? -1 : options.waveEnvelope[local.control.waveEnvelopeIndex], 
			true, false);
		}
	
	switch (result)
		{
		case NO_MENU_SELECTED:
			{
			}
		break;
		case MENU_SELECTED:
			{
			options.waveEnvelope[local.control.waveEnvelopeIndex] = (uint8_t) currentDisplay;
			if (currentDisplay == -1) 
				options.waveEnvelope[local.control.waveEnvelopeIndex] = ENVELOPE_END;  // should happen anyway but just in case...
			if (backupOptions.waveEnvelope[local.control.waveEnvelopeIndex] != options.waveEnvelope[local.control.waveEnvelopeIndex])
				saveOptions();  // this also sets backupOptions to options
			}
		// FALL THRU
		case MENU_CANCELLED:
			{
			goUpStateWithBackup(STATE_CONTROLLER_SET_WAVE_ENVELOPE);
			}
		break;
		}
	}

#endif
    
    


/***

	Types: Saw Up, Triangle, Sawdown
		   Square
		   Random
		   S&H
	Rate:  Based on Note Pulse
	Mod:   Saw<->Tri<->SawDown
		   Square PulseWidth
		   Random Walk
		   S&H Walk
	Range: Button 1 On/Off

	Left Knob: Saw Up, Triangle, Saw Down, Square Up 1/8 1/6 1/4 1/2 Square Down 1/3 1/4 1/6 1/8 Random 1 2 4 8 16 32 64 Infinity

    LFO1:   Same Function as Knob 1
    ENV1:   Same Function as Knob 2
    LFO2:   Same Function as Button 1
    ENV2:   Same Function as Button 2
        
    Middle button: swap LFO / ENV
         
    LFO Control                     [NO SINE]               Random, Saw Up, Saw Down, Square (PWM?), Triangle
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
