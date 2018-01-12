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
    const char* menuItems[7] = {  PSTR("OFF"), cc_p, nrpn_p, rpn_p, PSTR("PC"), PSTR("BEND"), PSTR("AFTERTOUCH")};
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
#ifdef INCLUDE_EXTENDED_CONTROLLER
				if (nextState == STATE_CONTROLLER_SET_WAVE_ENVELOPE_NUMBER)
					goUpState(STATE_CONTROLLER_WAVE_ENVELOPE);  // it's the wave envelope
				else if (nextState == STATE_CONTROLLER_SET_RANDOM_NUMBER)
					goUpState(STATE_CONTROLLER_RANDOM);  // it's the random generator
				else
#endif
                goUpState(STATE_CONTROLLER);
                }
#ifdef INCLUDE_EXTENDED_CONTROLLER
            else if ((type == CONTROL_TYPE_PC || type == CONTROL_TYPE_PITCH_BEND || type == CONTROL_TYPE_AFTERTOUCH))
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
					if (nextState == STATE_CONTROLLER_SET_WAVE_ENVELOPE_NUMBER)
						goUpState(STATE_CONTROLLER_WAVE_ENVELOPE);  // it's the wave envelope
					else if (nextState == STATE_CONTROLLER_SET_RANDOM_NUMBER)
						goUpState(STATE_CONTROLLER_RANDOM);  // it's the random generator
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
			if (nextState == STATE_CONTROLLER_SET_WAVE_ENVELOPE_NUMBER)
				goUpStateWithBackup(STATE_CONTROLLER_WAVE_ENVELOPE);  // it's the wave envelope
			else if (nextState == STATE_CONTROLLER_SET_RANDOM_NUMBER)
				goUpStateWithBackup(STATE_CONTROLLER_RANDOM);  // it's the random generator
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
    uint8_t result = doNumericalDisplay(0, max, number, 0, GLYPH_NONE);
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
            || nextState == STATE_CONTROLLER_WAVE_ENVELOPE
            || nextState == STATE_CONTROLLER_RANDOM
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
            if (nextState == STATE_CONTROLLER_WAVE_ENVELOPE)
            	goDownStateWithBackup(STATE_CONTROLLER_WAVE_ENVELOPE);  // it's the wave envelope
            else if (nextState == STATE_CONTROLLER_RANDOM)
            	goDownStateWithBackup(STATE_CONTROLLER_RANDOM);  // it's random lfo
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
            (((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) ? GLYPH_DECREMENT: GLYPH_INCREMENT);
#ifdef INCLUDE_EXTENDED_CONTROLLER
    else if (   
            ((((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) &&
            (options.middleButtonControlType == CONTROL_TYPE_PITCH_BEND)) ||
            ((((&onOff) == (&options.selectButtonControlOn)) || ((&onOff) == (&options.selectButtonControlOff))) &&
            (options.selectButtonControlType == CONTROL_TYPE_PITCH_BEND))
        )  // ugh, all this work just to determine if we're doing pitch bend YUCK
        result = doNumericalDisplay(MIDI_PITCHBEND_MIN - 1, MIDI_PITCHBEND_MAX, 0, true, GLYPH_NONE);
#endif
    else
        result = doNumericalDisplay(-1, CONTROL_VALUE_INCREMENT - 1, ((int16_t)onOff) - 1, true, GLYPH_NONE);

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
            goUpStateWithBackup(STATE_CONTROLLER_WAVE_ENVELOPE);
            }
        break;
        }
    }


void stateControllerResetWaveEnvelopeValuesGo()
	{
	// Length values for waves 2 through 8 are OFF by default.
	// Length value for wave 1 is 0 by default, hence we start at 3.
	memset(options.waveEnvelope, 0, sizeof(options.waveEnvelope));
	for(uint8_t i = 3; i < 16; i+=2)
		{
		options.waveEnvelope[i] = 255;
		}		
	saveOptions();
	goUpState(STATE_CONTROLLER_SET_WAVE_ENVELOPE_VALUE);
	}


// SET WAVE ENVELOPE
// Lets the user set a controller type.   This is stored in &type.  When the user is finished
// this function will go to the provided nextState (typically to set the controller number).
void stateControllerSetWaveEnvelope()
    {
    const char* menuItems[17] = { PSTR("1 VALUE"), PSTR("1 LENGTH"), PSTR("2 VALUE"), PSTR("2 LENGTH"), PSTR("3 VALUE"), PSTR("3 LENGTH"), PSTR("4 VALUE"), PSTR("4 LENGTH"), PSTR("5 VALUE"), PSTR("5 LEN"), PSTR("6 VALUE"), PSTR("6 LENGTH"), PSTR("7 VALUE"), PSTR("7 LENGTH"), PSTR("8 VALUE"), PSTR("8 LENGTH"), PSTR("RESET") };
    uint8_t result = doMenuDisplay(menuItems, 17, STATE_NONE, STATE_NONE, 1);

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
            goUpState(STATE_CONTROLLER_WAVE_ENVELOPE);
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
								(float)((WAVEVAL(endindex) << 7) - (float)(WAVEVAL(startindex) << 7)) +
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
	local.control.waveStartTicks = local.control.waveEndTicks; //(options.controlModulationClocked? pulseCount : tickCount);
	local.control.waveEndTicks = computeWaveEndTicks(local.control.wavePosition);
	}

void stateControllerPlayWaveEnvelope()
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
		
		
		// we set waveEndTicks here because resetWaveEnvelope sets waveStartTicks to waveEndTicks, and
		// then sets waveEndTicks to a new value.  This allows us to bootstrap what waveStartTicks should be.
		local.control.waveEndTicks = (options.controlModulationClocked? pulseCount : tickCount);
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
        	write3x5Glyphs(GLYPH_OFF);
        	}
        else 
        	{
        	writeShortNumber(led, local.control.lastWavePosition + 1, false);
        	writeShortNumber(led2, (uint8_t)(local.control.currentWaveControl >> 7), true);
        	}
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
		sendAllSoundsOff();  // we could get a stuck note when we leave
        goUpState(STATE_CONTROLLER_WAVE_ENVELOPE);
        }

	}




// SET WAVE ENVELOPE VALUE
// Lets the user set a controller type.   This is stored in &type.  When the user is finished
// this function will go to the provided nextState (typically to set the controller number).
void stateControllerSetWaveEnvelopeValue()
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


    
/**
Sample and Hold		: set RANGE to 255
Random Walk			: set RANGE to something smallish

GO
	Displays Random value
		Perhaps allow changing a knob to move to tempo or note speed?

MODES
	Gated
	Trigged
	Free
	
LENGTH
	0...255

RANGE
	0...255

INITIAL VALUE
	0...127			// could be surprising if we have a 14-bit number and we go UP from 127
	
SYNC/UNSYNC
	
*/


void seedRandomWalk()
	{
	randomSeed((uint16_t) (currentTime & 0xFFFF));
	}

// only generates random walk samples with 2^14 precision, for purposes of MIDI
// range may be any value 0...16383, though 0 makes little sense
uint16_t randomWalkSample(uint16_t current, uint16_t range)
	{
	// basic cases
	if (range == 0)
		return current;
		
	if (range >= 16256)	// we got a 127 as our range value
		{
		return random(0, 16384);  // yes, I note the 16384, not 16383.  It's exclusive, not inclusive.
		}
		
	int16_t rand = 0;

	// we want to do a uniform sampling around current, and not be biased by
	// the edges of the space.  But we can only try so many times to find an
	// in-bounds sample before it becomes an issue.  If we fail, we just return
	// the converse, which has to be within the space assuming range is valid. 

	for(uint8_t tries = 0; tries < MAX_RANDOM_TRIES ; tries++)
		{
		// random returns a uint16_t
		// since we're only dealing with numbers 0..2^14
		// the 32768 trick lets us find a random number in a safe region,
		// then make it signed in an easy fashion
		rand = ((int16_t) random(current + 32768 - range, current + 32768 + (range + 1))) - (int16_t) 32768;
		if (rand >= 0 && rand < 16384)
			{
			return (uint16_t)(rand);
			}
		}
		
	// if we're here, we failed after MAX_RANDOM_TRIES, so just do a simple biased sample.  We'll
	// take a sample of a range up to 16383.  If we're out of bounds, we subtract the random value instead.
	if (range > 16383)
		range = 16383;
	rand = ((int16_t) random(current + 32768 - range, current + 32768 + (range + 1))) - (int16_t) 32768;
	if (rand >= 0 && rand < 16384)
		return (uint16_t)(rand);
	else if ((current - rand + current) >= 0 && (current - rand + current) < 16384)
		return (uint16_t)(current - rand + current);
	
	// if we're here, then the range must be > 64 and we happened to hit on a current value that's
	// roughly 64 so we're exceeding by both adding and subtracting.  So we keep it as-is.  This
	// should be pretty rare.
	 return current;
	}
	
uint32_t computeRandomEndTicks()
	{
	// Note that these values are 3 times faster than the envelope values.  This makes our randomness
	// snappier but obviously we have less maximum (~5.3 seconds versus ~16 seconds)
	// We can't be any snappier than this when clocked.  So if we want to be faster
	// unclocked, it'd not be equivalent to the clocked values any more.   I think we'll stick
	// with equivalence.
	
	// Compare this code to computeWaveEndTicks()
	
	// Beats are 2 a second at 120bpm.  We have 24 steps a beat, or 
	// 48 a second.  MIDI clock pulses are 48 a second at 120bpm.  So
	// our steps are 1 per clock pulse.
	if (options.controlModulationClocked)
		{
		return local.control.waveStartTicks + options.randomLength * 1;
		}
	else
		{
		// We want to be approximately the same as 120bpm LFOs.  So we need
		// 48 steps a second.  Ticks are 3125 a second, we want approximately 65 ticks per step.
		return local.control.waveStartTicks + options.randomLength * 65;
		}
	}

uint16_t computeRandomValue(uint16_t startVal, uint16_t endVal)
	{
	// (currenttime - starttime) / (endtime - starttime) = (currentval - startval) / (endval - startval)
	// so currentval = (currenttime - starttime) / (endtime - starttime) * (endval - startval) + startval
	// Note that the vals are 1 off, so that's why we're subtracting 1
	float _currentWaveControl = (float)((options.controlModulationClocked? pulseCount : tickCount) - local.control.waveStartTicks) / 
								((float)local.control.waveEndTicks - local.control.waveStartTicks) * 
								(float)(endVal - (float)startVal) +
								(float)startVal;
		
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
	
	

void stateControllerPlayRandom()
	{
    if (entry) 
        {
    	local.control.wavePosition = -1;
    	local.control.lastWavePosition = -1;
    	local.control.noteOnCount = 0;
    	local.control.waveCountDown = WAVE_COUNTDOWN;
    	local.control.randomKeyDownOnce = 0;
    	seedRandomWalk();
    	backupOptions = options;  // cause we'll be fiddling with the options
    	entry = false;
        }

    if (local.control.waveCountDown > 0)
    	local.control.waveCountDown--;
    	
    if (potUpdated[LEFT_POT])
        {
        options.randomRange = pot[LEFT_POT] >> 3 + 1;  //  / 8
        if (options.randomRange > 127)
        	options.randomRange = 127;
		local.control.endWaveControl = randomWalkSample(local.control.endWaveControl, options.randomRange << 7);
        }
    if (potUpdated[RIGHT_POT])
    	{
        options.randomLength = pot[RIGHT_POT] >> 2;  //  / 4
		local.control.waveEndTicks = computeRandomEndTicks();
    	}


	if (newItem && itemType == MIDI_NOTE_OFF)
		{
		MIDI.sendNoteOff(itemNumber, itemValue, itemChannel);
 		if (local.control.noteOnCount > 0)
			{
			local.control.noteOnCount--;
			}
		if ((local.control.noteOnCount == 0) && (options.randomMode == RANDOM_MODE_GATED || options.randomMode == RANDOM_MODE_NOT_RESET || options.randomMode == RANDOM_MODE_SH_GATED || options.randomMode == RANDOM_MODE_SH_NOT_RESET))
			{
			local.control.wavePosition = -1;
			}
		}
    
    // If we haven't started yet and got a MIDI_NOTE_ON *or* we're free
	if (local.control.wavePosition == -1 && ((newItem && itemType == MIDI_NOTE_ON) || (options.randomMode == RANDOM_MODE_FREE) || (options.randomMode == RANDOM_MODE_SH_FREE)))
		{
		local.control.wavePosition = 0;
		if (((options.randomMode == RANDOM_MODE_NOT_RESET) || (options.randomMode == RANDOM_MODE_SH_NOT_RESET)) && local.control.randomKeyDownOnce)
			{
			local.control.startWaveControl = local.control.currentWaveControl;
			}
		else
			{
			local.control.startWaveControl = randomWalkSample(options.randomInitialValue << 7, options.randomRange << 7);
			local.control.randomKeyDownOnce = 1;
			}
		
		local.control.endWaveControl = randomWalkSample(local.control.startWaveControl, options.randomRange << 7);
 		local.control.currentWaveControl = local.control.startWaveControl;
		sendControllerCommand(options.randomControlType, options.randomControlNumber, local.control.currentWaveControl, options.channelOut);
		local.control.waveCountDown = WAVE_COUNTDOWN;
		local.control.waveStartTicks = (options.controlModulationClocked? pulseCount : tickCount);
		local.control.waveEndTicks = computeRandomEndTicks();
		}
	// we've begun playing.  Need to update.  But we don't update if length == FOREVER
	else if (local.control.wavePosition >= 0 && local.control.waveCountDown == 0 && options.randomLength != RANDOM_LENGTH_FOREVER)
		{
		if ((options.controlModulationClocked? pulseCount : tickCount) >= local.control.waveEndTicks)   // Have we completed a stage?  
			{
			// Reset
			local.control.startWaveControl = local.control.endWaveControl;
			local.control.endWaveControl = randomWalkSample(local.control.startWaveControl, options.randomRange << 7);
 			local.control.currentWaveControl = local.control.startWaveControl;
			sendControllerCommand(options.randomControlType, options.randomControlNumber, local.control.currentWaveControl, options.channelOut);
			local.control.waveCountDown = WAVE_COUNTDOWN;
			local.control.waveStartTicks = local.control.waveEndTicks; //(options.controlModulationClocked? pulseCount : tickCount);
			local.control.waveEndTicks = computeRandomEndTicks();
			}
		else if ((options.randomMode == RANDOM_MODE_GATED) || (options.randomMode == RANDOM_MODE_TRIGGERED) || (options.randomMode == RANDOM_MODE_NOT_RESET) || (options.randomMode == RANDOM_MODE_FREE))  // only interpolate if we're not doing S&H
			{
			uint16_t currentWaveControl = computeRandomValue(local.control.startWaveControl, local.control.endWaveControl);
			
			// Is this a different value than before?  If so, we update
			if (currentWaveControl != local.control.currentWaveControl)
				{
				local.control.currentWaveControl = currentWaveControl;
 				sendControllerCommand(options.randomControlType, options.randomControlNumber, local.control.currentWaveControl, options.channelOut);
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
        	write3x5Glyphs(GLYPH_OFF);
        	}
        else 
        	{
        	writeShortNumber(led, (uint8_t)(local.control.endWaveControl >> 7), false);
        	writeShortNumber(led2, (uint8_t)(local.control.currentWaveControl >> 7), true);
        	}
        	
		drawRange(led2, 0, 1, 16, options.randomRange >> 3);  // / 8
		drawRange(led, 0, 1, 16, options.randomLength >> 4);  // / 16
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
		sendAllSoundsOff();  // we could get a stuck note when we leave
    	options = backupOptions;  // cause we probably fiddled with the options
        goUpState(STATE_CONTROLLER_RANDOM);
        }
	}
	
	

void stateControllerRandomSetMode()
	{
    if (entry) 
        {
        backupOptions = options;
        }

    const char* menuItems[8] = {  PSTR("GATED"), PSTR("TRIGGERED"), PSTR("NOT RESET"), PSTR("FREE"), PSTR("SH GATED"), PSTR("SH TRIGGERED"), PSTR("SH NOT RESET"), PSTR("SH FREE") };
    uint8_t result = doMenuDisplay(menuItems, 8, STATE_NONE, STATE_NONE, 1);

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            if (options.randomMode != currentDisplay)
           		{
           		options.randomMode = currentDisplay;
            	saveOptions();
            	}
            }
        // FALL THRU
        case MENU_CANCELLED:
            {
            goUpStateWithBackup(STATE_CONTROLLER_RANDOM);
            }
        break;
        }
	}

#endif



#endif
