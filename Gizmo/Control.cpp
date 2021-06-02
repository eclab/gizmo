////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"



#ifdef INCLUDE_CONTROLLER

// stateControllerPlay() and stateController() have been inlined into the state machine to save space

// SET CONTROLLER TYPE
// Lets the user set a controller type.   This is stored in &type.  If the user has selected
// CC, NRPN, or RPN, and thus must specify a parameter number, then we switch to nextState.
// If the user has seleted BEND or AFTERTOUCH and is fromButon, then we switch to nextState.
// Otherwise, we switch to returnState (including OFF and cancelling).
void setControllerType(uint8_t &type, uint8_t nextState, uint8_t returnState, uint8_t fromButton)
    {
    int8_t result;
    if (entry) 
        {
        backupOptions = options; 
        }
    const char* menuItems[7] = {  PSTR("OFF"), PSTR("CC"), PSTR("NRPN"), PSTR("RPN"), PSTR("BEND"), PSTR("AFTERTOUCH")};
    result = doMenuDisplay(menuItems, 7, STATE_NONE,  STATE_NONE, 1);
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
                goUpState(returnState);
                }
            else if ((type == CONTROL_TYPE_PITCH_BEND || type == CONTROL_TYPE_AFTERTOUCH))
                {
                if (fromButton)                // it's a button, we need to get button values
                    {
                    goDownState(nextState);
                    }
                else
                    {
                    saveOptions();
                    goUpState(returnState);
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
            goUpState(returnState);
            }
        break;
        }
    }


// SET CONTROLLER NUMBER
// Lets the user set a controller number for the given controller type, given a button (or not).  This is stored in &number.
// The original type, determined by setControllerType, ise in 'type'.  Backups are provided.
// If the user selects an item, we go to nextState.  If the user cancels, we go to returnState.
void setControllerNumber(uint8_t type, uint16_t &number, uint8_t backupType, uint16_t backupNumber, uint8_t nextState, uint8_t returnState, uint8_t fromButton)
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
            if (fromButton)
                {
                local.control.doIncrement = (type == CONTROL_TYPE_NRPN || type == CONTROL_TYPE_RPN);
                }
            else
                {
                saveOptions();
                }
            goDownState(nextState);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(returnState);
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
    else if (   
            ((((&onOff) == (&options.middleButtonControlOn)) || ((&onOff) == (&options.middleButtonControlOff))) &&
            (options.middleButtonControlType == CONTROL_TYPE_PITCH_BEND)) ||
            ((((&onOff) == (&options.selectButtonControlOn)) || ((&onOff) == (&options.selectButtonControlOff))) &&
            (options.selectButtonControlType == CONTROL_TYPE_PITCH_BEND))
        )  // ugh, all this work just to determine if we're doing pitch bend YUCK
        result = doNumericalDisplay(MIDI_PITCHBEND_MIN - 1, MIDI_PITCHBEND_MAX, 0, true, GLYPH_NONE);
    else
        result = doNumericalDisplay(-1, CONTROL_VALUE_INCREMENT - 1, ((int16_t)onOff) - 1, true, GLYPH_NONE);

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
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
    

void stateControllerModulationSetMode()
    {
    if (entry) 
        {
        backupOptions = options;
        }

    const char* menuItems[5] = {  PSTR("GATED"), PSTR("FADED"), PSTR("TRIGGERED"), PSTR("LOOPED"), PSTR("FREE") };
    uint8_t result = doMenuDisplay(menuItems, 5, STATE_NONE, STATE_NONE, 1);

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
    // the start wave value is going to be fadeStartControl *if* we're doing FADED, and we just *restarted* (that is,
    // fadeStartControl isn't negative) rather than *started*.  In all other cases, it's just the standard start wave of
    // the given index
    float _startWaveVal = (options.envelopeMode == ENVELOPE_MODE_FADED && startindex == 0 && local.control.noteOnCount > 0 && local.control.fadeStartControl >= 0 ? 
        local.control.fadeStartControl : (float)(WAVEVAL(startindex) << 7));

    // (currenttime - starttime) / (endtime - starttime) = (currentval - startval) / (endval - startval)
    // so currentval = (currenttime - starttime) / (endtime - starttime) * (endval - startval) + startval
    // Note that the vals are 1 off, so that's why we're subtracting 1
                                                   
    float _currentWaveControl = (float)((options.controlModulationClocked? pulseCount : tickCount) - local.control.waveStartTicks) / 
        (float)(local.control.waveEndTicks - local.control.waveStartTicks) * 
        (float)((WAVEVAL(endindex) << 7) - _startWaveVal) + _startWaveVal;
                
    if (_currentWaveControl < 0) _currentWaveControl = 0;
    if (_currentWaveControl > 16383) _currentWaveControl = 16383;
    local.control.fadeWaveControl = _currentWaveControl;                    // we update fadeWaveControl here.  Note: can't be negative.
        
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
    if (index == 0)
        local.control.fadeStartControl = local.control.fadeWaveControl;  // may be we're doing FADED?  So set the start control to the very last wave control immedately before resetting.
    local.control.wavePosition = index;
    if (!(index == 0 && options.envelopeMode == ENVELOPE_MODE_FADED && local.control.noteOnCount > 0))          // don't send a controller command with the new position, or we get a click
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
        local.control.fadeStartControl = -1;                    // < 0 so FADED works just like GATED the first time around.
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
        if ((local.control.noteOnCount == 0) && (options.envelopeMode == ENVELOPE_MODE_GATED || options.envelopeMode == ENVELOPE_MODE_FADED || options.envelopeMode == ENVELOPE_MODE_LOOPED))
            {
            local.control.wavePosition = -1;
            }
        }
    else if (newItem && itemType == MIDI_NOTE_ON && (options.envelopeMode == ENVELOPE_MODE_FADED))
        {
        local.control.wavePosition = -1;
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
    else if (local.control.wavePosition >= 0  && local.control.waveCountDown == 0)  // we've begun playing.  Need to update.
        {
        if ((options.controlModulationClocked? pulseCount : tickCount) >= local.control.waveEndTicks)   // Have we completed a stage?  
            {
            // Move to the next stage
            local.control.wavePosition++;
            if ((options.envelopeMode == ENVELOPE_MODE_GATED || options.envelopeMode == ENVELOPE_MODE_FADED || options.envelopeMode == ENVELOPE_MODE_TRIGGERED) &&
                (local.control.wavePosition == (ENVELOPE_SIZE - 1) || WAVETIME(local.control.wavePosition) == ENVELOPE_END))
                {
                // We're at the end of one-shot, do one last controller command, sort of a semi reset envelope.
                // Unlike resetWaveEnvelope, we can send a controller command here this because we're not resetting the envelope to 0, right?
                local.control.currentWaveControl = WAVEVAL(local.control.wavePosition) << 7;
                local.control.fadeWaveControl = local.control.currentWaveControl;
                local.control.fadeStartControl = local.control.fadeWaveControl;
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
            if ((options.envelopeMode == ENVELOPE_MODE_LOOPED || options.envelopeMode == ENVELOPE_MODE_FREE) &&                     /// we're looping AND
                (local.control.wavePosition == (ENVELOPE_SIZE - 1) || WAVETIME(local.control.wavePosition + 1) == ENVELOPE_END))                                        // it's the last wave
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
   Sample and Hold         : set RANGE to 255
   Random Walk                     : set RANGE to something smallish

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
   0...127                 // could be surprising if we have a 14-bit number and we go UP from 127
        
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
                
    if (range >= 16256)     // we got a 127 as our range value
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
    
void stateControllerPlay()
    {
    if (entry)
        {
        local.control.middleButtonToggle = 0;
        local.control.selectButtonToggle = 0;
        local.control.displayValue = -1;
        local.control.displayType = CONTROL_TYPE_OFF;
        local.control.potWaiting[0] = 0;
        local.control.potWaiting[1] = 0;
        local.control.potWaiting[2] = 0;
        local.control.potWaiting[3] = 0;
        entry = false;
        dontBypassOut = true;
        // update bypassOut on entry
        bypassOut = (bypass && !dontBypassOut);
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        dontBypassOut = false;
        // update bypassOut on exit
        bypassOut = (bypass && !dontBypassOut);
        goUpState(STATE_CONTROLLER);
        }
    else
        {
        // this region is redundant but simplifying to a common function call makes the code bigger 
        
        if (isUpdated(MIDDLE_BUTTON, PRESSED))
            {
            local.control.middleButtonToggle = !local.control.middleButtonToggle;
            if (options.middleButtonControlType != CONTROL_TYPE_OFF)
                {
                local.control.displayValue = ((local.control.middleButtonToggle ? options.middleButtonControlOn : options.middleButtonControlOff));
                        
                if (options.middleButtonControlType == CONTROL_TYPE_PITCH_BEND)
                    {
                    if (local.control.displayValue != 0) // if it's not "off"
                        {
                        local.control.displayValue--;
                        local.control.displayType = options.middleButtonControlType;
                        }
                    else
                        {
                        local.control.displayType = CONTROL_TYPE_OFF;
                        }
                    }
                else
                    {
                    // at this point local.control.displayValue is 0...129, where 0 is off and 129 is "INCREMENT", and 1...128 is 0...127
                    if (local.control.displayValue != 0) // if it's not "off"
                        {
                        local.control.displayValue--;  // map to 0...128, where 128 is "INCREMENT"

                        // now convert INCREMENT to DECREMENT
                        if (local.control.displayValue == CONTROL_VALUE_INCREMENT)
                            local.control.displayValue++;
                                                                
                        // Now move to MSB+LSB
                        local.control.displayValue = local.control.displayValue << 7;
                                                                
                        sendControllerCommand( local.control.displayType = options.middleButtonControlType, options.middleButtonControlNumber, local.control.displayValue, options.channelOut);
                                                        
                        }
                    else
                        local.control.displayType = CONTROL_TYPE_OFF;
                    }
                }
            }

        if (isUpdated(SELECT_BUTTON, PRESSED))
            {
            local.control.selectButtonToggle = !local.control.selectButtonToggle;
            if (options.selectButtonControlType != CONTROL_TYPE_OFF)
                {
                local.control.displayValue = ((local.control.selectButtonToggle ?  options.selectButtonControlOn :  options.selectButtonControlOff));

                if (options.selectButtonControlType == CONTROL_TYPE_PITCH_BEND)
                    {
                    if (local.control.displayValue != 0) // if it's not "off"
                        {
                        local.control.displayValue--;
                        local.control.displayType = options.selectButtonControlType;
                        }
                    else
                        {
                        local.control.displayType = CONTROL_TYPE_OFF;
                        }
                    }
                else
                    {
                    // at this point local.control.displayValue is 0...129, where 0 is off and 129 is "INCREMENT", and 1...128 is 0...127
                    if (local.control.displayValue != 0)    // if we're not OFF
                        {
                        local.control.displayValue--;  // map to 0...128, where 128 is "INCREMENT"
                                                                
                        // Now move to MSB+LSB
                        local.control.displayValue = local.control.displayValue << 7;
                                                                
                        sendControllerCommand( local.control.displayType = options.selectButtonControlType, options.selectButtonControlNumber, local.control.displayValue, options.channelOut); 
                        }
                    else
                        local.control.displayType = CONTROL_TYPE_OFF;
                    }
                }
            }
        
        if (potUpdated[LEFT_POT] && (options.leftKnobControlType != CONTROL_TYPE_OFF))
            {
            if (local.control.potUpdateValue[LEFT_POT] != pot[LEFT_POT] << 4)
                {
                local.control.potUpdateValue[LEFT_POT] = pot[LEFT_POT] << 4;
                local.control.potWaiting[LEFT_POT] = 1;
                }
            }
          
        if (potUpdated[RIGHT_POT] && (options.rightKnobControlType != CONTROL_TYPE_OFF))
            {
            if (local.control.potUpdateValue[RIGHT_POT] != pot[RIGHT_POT] << 4)
                {
                local.control.potUpdateValue[RIGHT_POT] = pot[RIGHT_POT] << 4;
                local.control.potWaiting[RIGHT_POT] = 1;
                }
            }
        
        if (potUpdated[A2_POT] && (options.a2ControlType != CONTROL_TYPE_OFF))
            {
            if (local.control.potUpdateValue[A2_POT] != pot[A2_POT] << 4)
                {
                local.control.potUpdateValue[A2_POT] = pot[A2_POT] << 4;
                local.control.potWaiting[A2_POT] = 1;
                }
            }

        if (potUpdated[A3_POT] && (options.a3ControlType != CONTROL_TYPE_OFF))
            {
            if (local.control.potUpdateValue[A3_POT] != pot[A3_POT] << 4)
                {
                local.control.potUpdateValue[A3_POT] = pot[A3_POT] << 4;
                local.control.potWaiting[A3_POT] = 1;
                }
            }

        // figure out who has been waiting the longest, if any.  The goal here is to only allow one out at a time and yet prevent starvation.
        // Yes, this will mess up right at the maximum time value, but that is very rare and it's only a starvation issue if any at all, and only for a bit.

        int8_t winner = -1;
        uint32_t winnerTime = MAX_32;
        if (local.control.potWaiting[LEFT_POT] && local.control.potUpdateTime[LEFT_POT] < winnerTime) 
            { 
            winner = LEFT_POT; winnerTime = local.control.potUpdateTime[LEFT_POT];
            }
        if (local.control.potWaiting[RIGHT_POT] && local.control.potUpdateTime[RIGHT_POT] < winnerTime) 
            { 
            winner = RIGHT_POT; winnerTime = local.control.potUpdateTime[RIGHT_POT];
            }
        if (local.control.potWaiting[A2_POT] && local.control.potUpdateTime[A2_POT] < winnerTime) 
            { 
            winner = A2_POT; winnerTime = local.control.potUpdateTime[A2_POT];
            }
        if (local.control.potWaiting[A3_POT] && local.control.potUpdateTime[A3_POT] < winnerTime) 
            { 
            winner = A3_POT; winnerTime = local.control.potUpdateTime[A3_POT];
            }
            
        // Do we have a winner?
        if (winner == LEFT_POT)
            {
            sendControllerCommand( local.control.displayType = options.leftKnobControlType, options.leftKnobControlNumber, local.control.potUpdateValue[LEFT_POT], options.channelOut);
            local.control.potUpdateTime[LEFT_POT] = currentTime;
            local.control.potWaiting[LEFT_POT] = 0;
            local.control.displayValue = local.control.potUpdateValue[LEFT_POT] ;
            }
        else if (winner == RIGHT_POT)
            {
            sendControllerCommand( local.control.displayType = options.rightKnobControlType, options.rightKnobControlNumber, local.control.potUpdateValue[RIGHT_POT], options.channelOut);
            local.control.potUpdateTime[RIGHT_POT] = currentTime;
            local.control.potWaiting[RIGHT_POT] = 0;
            local.control.displayValue = local.control.potUpdateValue[RIGHT_POT] ;
            }
        else if (winner == A2_POT)
            {
            sendControllerCommand( local.control.displayType = options.a2ControlType, options.a2ControlNumber, local.control.potUpdateValue[A2_POT], options.channelOut);
            local.control.potUpdateTime[A2_POT] = currentTime;
            local.control.potWaiting[A2_POT] = 0;
            local.control.displayValue = local.control.potUpdateValue[A2_POT] ;
            }
        else if (winner == A3_POT)
            {
            sendControllerCommand( local.control.displayType = options.a3ControlType, options.a3ControlNumber, local.control.potUpdateValue[A3_POT], options.channelOut);
            local.control.potUpdateTime[A3_POT] = currentTime;
            local.control.potWaiting[A3_POT] = 0;
            local.control.displayValue = local.control.potUpdateValue[A3_POT] ;
            }
        }
   
    if (updateDisplay)
        {
        clearScreen();
                
        // local.control.displayValue is now -1, meaning "OFF",
        // or it is a value in the range of MSB + LSB

        if (local.control.displayType != CONTROL_TYPE_OFF)  // isn't "off"
            {
            uint8_t msb = (uint8_t)(local.control.displayValue >> 7);
                                                                                        
            // if we needed a little bit more space, we could change this to something like
            // write3x5Glyphs(msb - CONTROL_VALUE_INCREMENT + GLYPH_INCREMENT);
            // except that GLYPH_INCREMENT comes SECOND.  We'd need to fix all that to make it
            // consistent.  It'd save us about 20 bytes maybe?
            if (msb == CONTROL_VALUE_INCREMENT)
                {
                write3x5Glyphs(GLYPH_INCREMENT);
                }
            else if (msb == CONTROL_VALUE_DECREMENT)
                {
                write3x5Glyphs(GLYPH_DECREMENT);
                }
            else
                {
                if (local.control.displayType == CONTROL_TYPE_PITCH_BEND)
                    {
                    writeNumber(led, led2, ((int16_t)(local.control.displayValue)) + (int16_t)(MIDI_PITCHBEND_MIN));
                    }
                else
                    writeShortNumber(led, msb, false);
                }
            }
        else
            {
            write3x5Glyphs(GLYPH_OFF);
            }
        }
    }

#endif
