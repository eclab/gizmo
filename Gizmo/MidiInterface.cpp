////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"


#ifdef INCLUDE_CC_LEFT_POT_PARAMETER_EQUIVALENTS
// If this is TRUE then the various left pot parameter equivalent CCs will
// do the same thing as the left pot parameter CC.  If false, they will do nothing
// (the application can still have them do something)
GLOBAL uint8_t leftPotParameterEquivalent = false;
#endif





////////// MIDI HANDLERS  


/// Yet ANOTHER small code-shortening function!
/// Updates incoming midi items only if they came in the proper channel, and returns TRUE
/// if this occurred. 

uint8_t updateMIDI(byte channel, uint8_t _itemType, uint16_t _itemNumber, uint16_t _itemValue)
    {
    TOGGLE_IN_LED();
#ifdef INCLUDE_SYNTH
    if (application == STATE_SYNTH &&
        local.synth.passMIDIData[_itemType])            // pass it through as instructed
        {
        newItem = NO_NEW_ITEM;
        return 0;
        }
    else 
#endif
#ifdef INCLUDE_EXTENDED_CONTROLLER
        if (state == STATE_CONTROLLER_PLAY_WAVE_ENVELOPE && 
            (channel == options.channelIn || options.channelIn == CHANNEL_OMNI) &&
            _itemType != MIDI_NOTE_ON &&
            _itemType != MIDI_NOTE_OFF)
            {
            newItem = NO_NEW_ITEM;
            return 0;
            }
        else 
#endif
            if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
                {
                newItem = NEW_ITEM;
                itemType = _itemType;
                itemNumber = _itemNumber;
                itemValue = _itemValue;
                itemChannel = channel;
                return 1;
                }
            else 
                {
                newItem = NO_NEW_ITEM;
                return 0;
                }
    }


// A code-shortening effort.  Processes a MIDI clock command (start/stop/continue/clock)
void handleClockCommand(uint8_t (*clockFunction)(uint8_t), midi::MidiType clockMIDI)
    {
    newItem = NEW_ITEM;
    itemType = MIDI_CLOCK;

    TOGGLE_IN_LED();
    
// Here's the logic.
// If we're NOT doing DIVISION:
//              IGNORE: sendClock()             -- we just pass it through, unless bypass is on [sendClock checks for bypass already so we don't do it here]
//              GENERATE or BLOCK:              -- ignore
//              CONSUME or USE:                 -- call the function to deal with it
//
// If we're doing DIVISION:
//              IGNORE:                                 -- handle division specially, unless bypass is on [again, sendClock and sendDividedClock check for bypass already]
//              GENERATE or BLOCK:              -- ignore
//              CONSUME or USE:                 -- call the function to deal with it

    if (options.clock == IGNORE_MIDI_CLOCK)

#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
        {
        if (bypass)
            {
            TOGGLE_OUT_LED();
            }
        else if (clockFunction == pulseClock)
            {
            sendDividedClock();
            }
        else if (clockFunction == startClock)
            {
            resetDividedClock();
            sendClock(clockMIDI, false);
            }
        else                            // continue or stop
            {
            sendClock(clockMIDI, false);
            }
        }
#else
        {
        sendClock(clockMIDI, false);
        }
#endif

        else if (USING_EXTERNAL_CLOCK())  // CONSUME, USE, or IGNORE (which we just handled already)
            {
            clockFunction(false);
            }
        else                    // GENERATE OR BLOCK
            {
            // do nothing
            }
    }

void handleStart()
    {
    newItem = NEW_ITEM;
    itemType = MIDI_START;
    
    handleClockCommand(startClock, MIDIStart);
    
#ifdef INCLUDE_MEASURE
    if (application == STATE_MEASURE)
        {
        local.measure.running = true;
        resetMeasure();
        }
#endif
    }

void handleStop()
    {
    newItem = NEW_ITEM;
    itemType = MIDI_STOP;
    
    handleClockCommand(stopClock, MIDIStop);

#ifdef INCLUDE_MEASURE
    if (application == STATE_MEASURE)
        {
        local.measure.running = false;
        }
#endif
    }

void handleContinue()
    {
    newItem = NEW_ITEM;
    itemType = MIDI_CONTINUE;
    
    handleClockCommand(continueClock, MIDIContinue);

#ifdef INCLUDE_MEASURE
    if (application == STATE_MEASURE)
        {
        local.measure.running = true;
        }
#endif
    }
  
void handleClock()
    {
    handleClockCommand(pulseClock, MIDIClock);
    }
    
    
    
// These are button states sent to use via NRPN messages
GLOBAL uint8_t buttonState[3] = { 0, 0, 0 };




void handleNoteOff(byte channel, byte note, byte velocity)
    {
#ifdef INCLUDE_CONTROL_BY_NOTE
    if (channel == options.channelControl)  // options.channelControl can be zero remember
        {
        lockoutPots = 1;

        switch (note)
            {
            case CC_BACK_BUTTON_PARAMETER:
                {
                buttonState[BACK_BUTTON] = 0;
                updateButtons(buttonState); 
                }
            break;
            case CC_MIDDLE_BUTTON_PARAMETER:
                {
                buttonState[MIDDLE_BUTTON] = 0;
                updateButtons(buttonState); 
                }
            break;
            case CC_SELECT_BUTTON_PARAMETER:
                {
                buttonState[SELECT_BUTTON] = 0;
                updateButtons(buttonState); 
                }
            break;
            }
        }
    else
#endif
        // is data coming in the default channel?
        if (updateMIDI(channel, MIDI_NOTE_OFF, note, velocity))
            {
#ifdef INCLUDE_ARPEGGIATOR
            if (application == STATE_ARPEGGIATOR && local.arp.playing && !bypass)
                {
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
                if (local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
                    {
                    uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
                    if (channelOut == 0)
                        channelOut = options.channelOut;
                    sendNoteOff(note, velocity, channelOut);
                    }
                else if (local.arp.performanceMode && options.arpeggiatorPlayAlongChannel == ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
                    {
                    // do nothing
                    }
                else
#endif             
                    arpeggiatorRemoveNote(note);
                }
            else
#endif

#ifdef INCLUDE_SPLIT
                if ((application == STATE_SPLIT) && local.split.playing && !bypass)
                    {
                    if (options.splitControls == SPLIT_MIX)
                        {
                        sendNoteOff(note, velocity, options.channelOut);
                        sendNoteOff(note, velocity, options.splitChannel);
                        TOGGLE_OUT_LED();
                        }
                    else
                        {
                        uint8_t sent = 0;
                        if (note >= options.splitNote)
                            { sendNoteOff(note, velocity, options.channelOut); sent = 1; }
                        if (((options.splitLayerNote != NO_NOTE) && (note <= options.splitLayerNote)) ||
                            note < options.splitNote)
                            { sendNoteOff(note, velocity, options.splitChannel); sent = 1; }
                        if (sent == 2)
                            TOGGLE_OUT_LED();
                        }
                    }
                else
#endif
                    { }
            }
        else
#ifdef INCLUDE_THRU
            if (!bypass && (state == STATE_THRU_PLAY))
                {
                // only pass through if the data's NOT coming in the default channel
                if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI)
                    && channel != options.thruMergeChannelIn)
                    {
                    if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
                        {
                        MIDI.sendNoteOff(note, velocity, channel);
                        TOGGLE_OUT_LED();
                        }
                    }
                else if (channel == options.thruMergeChannelIn)  //  merge hasn't been sent to newitem yet
                    {
                    newItem = NEW_ITEM;
                    itemType = MIDI_NOTE_OFF;
                    itemNumber = note;
                    itemValue = velocity;
                    itemChannel = channel;
                    }
                }
            else
#endif
                {
                if (!bypass)
                    {
                    MIDI.sendNoteOff(note, velocity, channel);
                    }
                TOGGLE_OUT_LED();
                }
    }

void handleNoteOn(byte channel, byte note, byte velocity)
    {
#ifdef INCLUDE_CONTROL_BY_NOTE
    if (channel == options.channelControl)  // options.channelControl can be zero remember
        {
        lockoutPots = 1;

        switch (note)
            {
            case CC_BACK_BUTTON_PARAMETER:
                {
                buttonState[BACK_BUTTON] = 1;
                updateButtons(buttonState); 
                }
            break;
            case CC_MIDDLE_BUTTON_PARAMETER:
                {
                buttonState[MIDDLE_BUTTON] = 1;
                updateButtons(buttonState); 
                }
            break;
            case CC_SELECT_BUTTON_PARAMETER:
                {
                buttonState[SELECT_BUTTON] = 1;
                updateButtons(buttonState); 
                }
            break;
            case CC_BYPASS_PARAMETER:
                {
                toggleBypass(CHANNEL_OMNI);
                }
            break;
            case CC_UNLOCK_PARAMETER:
                {
                lockoutPots = 0;
                }
            break;
            case CC_START_PARAMETER:
                {
                startClock(true);
                }
            break;
            case CC_STOP_PARAMETER:
                {
                stopClock(true);
                }
            break;
            case CC_CONTINUE_PARAMETER:
                {
                continueClock(true);
                }
            break;
            }
        }
    else
#endif
        // is data coming in the default channel?
        if (updateMIDI(channel, MIDI_NOTE_ON, note, velocity))
            {
#ifdef INCLUDE_ARPEGGIATOR
            if (!bypass && (application == STATE_ARPEGGIATOR && local.arp.playing 
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
                    && state != STATE_ARPEGGIATOR_PLAY_TRANSPOSE
#endif
                    ))
                {
                // the arpeggiation velocity shall be the velocity of the most recently added note
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
                if (local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
                    {
                    uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
                    if (channelOut == 0)
                        channelOut = options.channelOut;
                    sendNoteOn(note, velocity, channelOut);
                    }
                else if (local.arp.performanceMode && options.arpeggiatorPlayAlongChannel == ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
                    {
                    int16_t tr = note - (int16_t)(local.arp.transposeRoot);
                    while (tr < -128) tr += 12;
                    while (tr > 127) tr -= 12;
                    local.arp.transpose = (int8_t) tr;
                    }
                else
#endif             
                    arpeggiatorAddNote(note, velocity);
                }
            else
#endif

#ifdef INCLUDE_SPLIT
                // We don't have space for this on the Uno // 
                if ((application == STATE_SPLIT) && local.split.playing && !bypass)
                    {
                    if (options.splitControls == SPLIT_MIX)
                        {
                        // This is our current velocity function
                        uint8_t convertedVel =  (uint8_t)(((velocity) * (uint16_t)(velocity + 1))>> 7);
                        sendNoteOn(note, velocity, options.channelOut);
                        sendNoteOn(note, velocity - convertedVel, options.splitChannel);
                        local.split.lastMixVelocity = velocity;
                
                        TOGGLE_OUT_LED();
                        }
                    else
                        {
                        uint8_t sent = 0;
                        if (note >= options.splitNote)
                            { sendNoteOn(note, velocity, options.channelOut); sent++; }
                        if (((options.splitLayerNote != NO_NOTE) && (note <= options.splitLayerNote)) ||
                            note < options.splitNote)
                            { sendNoteOn(note, velocity, options.splitChannel); sent++; }
                        if (sent == 2)
                            TOGGLE_OUT_LED();
                        }
                    }
                else
#endif
                    { }
            }
        else
#ifdef INCLUDE_THRU
            if (!bypass && (state == STATE_THRU_PLAY))
                {
                // only pass through if the data's NOT coming in the default channel
                if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI)
                    && channel != options.thruMergeChannelIn)
                    {
                    if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
                        {
                        MIDI.sendNoteOn(note, velocity, channel);
                        TOGGLE_OUT_LED();
                        }
                    }
                else if (channel == options.thruMergeChannelIn)  //  merge hasn't been sent to newitem yet
                    {
                    newItem = NEW_ITEM;
                    itemType = MIDI_NOTE_ON;
                    itemNumber = note;
                    itemValue = velocity;
                    itemChannel = channel;
                    }
                }
            else
#endif
                {
                if (!bypass)
                    {
                    MIDI.sendNoteOn(note, velocity, channel);
                    }
                TOGGLE_OUT_LED();
                }
    }
  
void handleAfterTouchPoly(byte channel, byte note, byte pressure)
    {
    // (We don't have space for this on the Uno :-(  )
    // is data coming in the default channel?
    if (updateMIDI(channel, MIDI_AFTERTOUCH_POLY, note, pressure))
        {
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
        if (!bypass && (application == STATE_ARPEGGIATOR && local.arp.playing && local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE))
            {
            uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
            if (channelOut == 0)
                channelOut = options.channelOut;
            sendPolyPressure(note, pressure, channelOut);
            }
        else
#endif

#ifdef INCLUDE_SPLIT
            if ((application == STATE_SPLIT) && local.split.playing && !bypass)
                {
                if (options.splitControls == SPLIT_MIX)
                    {
                    sendPolyPressure(note, pressure, options.channelOut);
                    sendPolyPressure(note, pressure, options.splitChannel);
                    TOGGLE_OUT_LED();
                    }
                else
                    {
                    uint8_t sent = 0;
                    if (note >= options.splitNote)
                        { sendPolyPressure((uint8_t) note, pressure, options.channelOut); sent++; }
                    if (((options.splitLayerNote != NO_NOTE) && (note <= options.splitLayerNote)) ||
                        note < options.splitNote)
                        { sendPolyPressure((uint8_t) note, pressure,options.splitChannel); sent++; }
                    if (sent == 2)
                        TOGGLE_OUT_LED();
                    }
                }
            else
#endif
                { }
        }
    else
#ifdef INCLUDE_THRU
        if (!bypass && (state == STATE_THRU_PLAY))
            {
            // only pass through if the data's NOT coming in the default channel
            if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI)
                && channel != options.thruMergeChannelIn)
                {
                if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
                    {
                    MIDI.sendPolyPressure(note, pressure, channel);
                    TOGGLE_OUT_LED();
                    }
                }
            else if (channel == options.thruMergeChannelIn)  //  merge hasn't been sent to newitem yet
                {
                newItem = NEW_ITEM;
                itemType = MIDI_AFTERTOUCH_POLY;
                itemNumber = note;
                itemValue = pressure;
                itemChannel = channel;
                }
            }
        else
#endif    
            {
            if (!bypass)
                {
#ifdef INCLUDE_THRU
                if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
#endif
                    MIDI.sendPolyPressure(note, pressure, channel);
                }
            TOGGLE_OUT_LED();
            }
    }
    





//// CHANNEL CONTROL, NRPN, and RPN
////
//// My parser breaks these out and calls the sub-handlers
//// handleControlChange, handleNRPN, and handleRPN.
//// These are passed certain TYPES of data, indicating the
//// nature of the numerical value.

// Data types
#define VALUE 0                         // 14-bit values given to CC
#define VALUE_MSB_ONLY 1                // 
#define INCREMENT 2
#define DECREMENT 3
#define VALUE_7_BIT_ONLY 4  // this is last so we have a contiguous switch for NRPN and RPN handlers, which don't use this

void handleControlChange(byte channel, byte number, uint16_t value, byte type)
    {
#ifdef INCLUDE_CC_CONTROL
    if (channel == options.channelControl)  // options.channelControl can be zero remember
        {
        lockoutPots = 1;

        if ((number >= 64 && number < 96) ||
            (number >= 116 && number < 119))
            {
            newItem = NEW_ITEM;
            itemType = MIDI_CUSTOM_CONTROLLER;
            itemNumber = number;
            itemValue = value;
            itemChannel = channel;
            }                    
        else switch (number)
                 {
                 case CC_LEFT_POT_PARAMETER:
                     {
                     pot[LEFT_POT] = (value >> 4); 
                     potUpdated[LEFT_POT] = CHANGED;
                     }
                 break;
                 case CC_RIGHT_POT_PARAMETER:
                     {
                     pot[RIGHT_POT] = (value >> 4); 
                     potUpdated[RIGHT_POT] = CHANGED;
                     }
                 break;
#ifdef INCLUDE_CC_LEFT_POT_PARAMETER_EQUIVALENTS
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_1:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_2:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_3:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_4:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_5:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_6:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_7:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_8:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_9:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_10:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_11:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_12:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_13:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_14:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_15:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_16:
                     {
                     if (leftPotParameterEquivalent)
                         {
                         pot[LEFT_POT] = (value >> 4); 
                         potUpdated[LEFT_POT] = CHANGED;
                         }
                     else
                         {
                         newItem = NEW_ITEM;
                         itemType = MIDI_CUSTOM_CONTROLLER;
                         itemNumber = number;
                         itemValue = value;
                         itemChannel = channel;
                         }
                     }
                 break;
#endif
#ifdef INCLUDE_CC_CONTROL_LSB
                 case CC_LEFT_POT_PARAMETER_LSB:
                     {
                     pot[LEFT_POT] |= (value & 0x07); 
                     potUpdated[LEFT_POT] = CHANGED;
                     }
                 break;
                 case CC_RIGHT_POT_PARAMETER_LSB:
                     {
                     pot[RIGHT_POT] |= (value & 0x07); 
                     potUpdated[RIGHT_POT] = CHANGED;
                     }
                 break;
#ifdef INCLUDE_CC_LEFT_POT_PARAMETER_EQUIVALENTS
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_1_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_2_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_3_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_4_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_5_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_6_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_7_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_8_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_9_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_10_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_11_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_12_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_13_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_14_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_15_LSB:
                 case CC_LEFT_POT_PARAMETER_EQUIVALENT_16_LSB:
                     {
                     if (leftPotParameterEquivalent)
                         {
                         pot[LEFT_POT] |= (value & 0x07); 
                         potUpdated[LEFT_POT] = CHANGED;
                         }
                     else
                         {
                         newItem = NEW_ITEM;
                         itemType = MIDI_CUSTOM_CONTROLLER;
                         itemNumber = number;
                         itemValue = value;
                         itemChannel = channel;
                         }
                     }
                 break;
#endif
#endif
                 case CC_BACK_BUTTON_PARAMETER:
                     {
                     buttonState[BACK_BUTTON] = (value != 0);
                     updateButtons(buttonState); 
                     }
                 break;
                 case CC_MIDDLE_BUTTON_PARAMETER:
                     {
                     buttonState[MIDDLE_BUTTON] = (value != 0);
                     updateButtons(buttonState); 
                     }
                 break;
                 case CC_SELECT_BUTTON_PARAMETER:
                     {
                     buttonState[SELECT_BUTTON] = (value != 0);
                     updateButtons(buttonState); 
                     }
                 break;
                 case CC_BYPASS_PARAMETER:
                     {
                     toggleBypass(CHANNEL_OMNI);
                     }
                 break;
                 case CC_UNLOCK_PARAMETER:
                     {
                     lockoutPots = 0;
                     }
                 break;
                 case CC_START_PARAMETER:
                     {
                     startClock(true);
                     }
                 break;
                 case CC_STOP_PARAMETER:
                     {
                     stopClock(true);
                     }
                 break;
                 case CC_CONTINUE_PARAMETER:
                     {
                     continueClock(true);
                     }
                 break;
                 /*
                   case CC_LEFT_POT_RELATIVE_PARAMETER:
                   {
                   int16_t v = pot[LEFT_POT] + (int16_t)(value - 64) * MINIMUM_POT_DEVIATION;
                   if (v < 0) v = 0;
                   if (v > 1023) v = 1023;
                   pot[LEFT_POT] = (uint16_t) v; 
                   potUpdated[LEFT_POT] = CHANGED;
                   }
                   break;
                   case CC_RIGHT_POT_RELATIVE_PARAMETER:
                   {
                   int16_t v = pot[RIGHT_POT] + (int16_t)(value - 64) * MINIMUM_POT_DEVIATION;
                   if (v < 0) v = 0;
                   if (v > 1023) v = 1023;
                   pot[RIGHT_POT] = (uint16_t) v; 
                   potUpdated[RIGHT_POT] = CHANGED;
                   }
                   break;
                 */
                 }
        }
else
#endif

    // all values that come in are 14 bit with the MSB in the top 7 bits
    // and either the LSB *or* ZERO in the bottom 7 bits, EXCEPT for VALUE_7_BIT_ONLY,
    // which is just the raw number
    if (updateMIDI(channel, MIDI_CC_14_BIT, number, value))
        {
        newItem = NEW_ITEM;
        switch (type)
            {
            case VALUE:
                {
                itemType = MIDI_CC_14_BIT;
                }
            break;
            case VALUE_MSB_ONLY:
                {
                itemType = MIDI_CC_14_BIT;
                newItem = WAIT_FOR_A_SEC;
                }
            break;
            case INCREMENT:
                {
                // never happens
                }
            // FALL THRU
            case DECREMENT:
                {
                // never happens
                }
            // FALL THRU
            case VALUE_7_BIT_ONLY:
                {
                itemType = MIDI_CC_7_BIT;
                }
            break;
            }
        }
    } 
        
        
        
void handleNRPN(byte channel, uint16_t parameter, uint16_t value, uint8_t valueType)
    {
	if (channel == options.channelControl)  // options.channelControl can be zero remember
        {
        lockoutPots = 1;
                        
        switch (parameter)
            {
            case NRPN_LEFT_POT_PARAMETER:
                {
                pot[LEFT_POT] = (value >> 4); 
                potUpdated[LEFT_POT] = CHANGED;
                }
            break;
            case NRPN_RIGHT_POT_PARAMETER:
                {
                pot[RIGHT_POT] = (value >> 4); 
                potUpdated[RIGHT_POT] = CHANGED;
                }
            break;
            case NRPN_BACK_BUTTON_PARAMETER:
                {
                buttonState[BACK_BUTTON] = (value != 0);
                updateButtons(buttonState); 
                }
            break;
            case NRPN_MIDDLE_BUTTON_PARAMETER:
                {
                buttonState[MIDDLE_BUTTON] = (value != 0);
                updateButtons(buttonState); 
                }
            break;
            case NRPN_SELECT_BUTTON_PARAMETER:
                {
                buttonState[SELECT_BUTTON] = (value != 0);
                updateButtons(buttonState); 
                }
            break;
            case NRPN_BYPASS_PARAMETER:
                {
                toggleBypass(CHANNEL_OMNI);
                }
            break;
            case NRPN_UNLOCK_PARAMETER:
                {
                lockoutPots = 0;
                }
            break;
            case NRPN_START_PARAMETER:
                {
                startClock(true);
                }
            break;
            case NRPN_STOP_PARAMETER:
                {
                stopClock(true);
                }
            break;
            case NRPN_CONTINUE_PARAMETER:
                {
                continueClock(true);
                }
            break;
            }
        }
    else if (updateMIDI(channel, MIDI_NRPN_14_BIT, parameter, value))
        {
        switch (valueType)
            {
            case VALUE:
                {
                itemType = MIDI_NRPN_14_BIT;
                }
            break;
            case VALUE_MSB_ONLY:
                {
                itemType = MIDI_NRPN_14_BIT;
                newItem = WAIT_FOR_A_SEC;
                }
            break;
            case INCREMENT:
                {
                itemType = MIDI_NRPN_INCREMENT;
                }
            break;
            case DECREMENT:
                {
                itemType = MIDI_NRPN_DECREMENT;
                }
            break;
            }
        }
    }

void handleRPN(byte channel, uint16_t parameter, uint16_t value, uint8_t valueType)
    {
    if (updateMIDI(channel, MIDI_RPN_14_BIT, parameter, value))
        {
        switch (valueType)
            {
            case VALUE:
                {
                itemType = MIDI_NRPN_14_BIT;
                }
            break;
            case VALUE_MSB_ONLY:
                {
                itemType = MIDI_NRPN_14_BIT;
                newItem = WAIT_FOR_A_SEC;
                }
            break;
            case INCREMENT:
                {
                itemType = MIDI_RPN_INCREMENT;
                }
            break;
            case DECREMENT:
                {
                itemType = MIDI_RPN_DECREMENT;
                }
            break;
            }
        }
    }    
    
    
///// INTRODUCTION TO THE CC/RPN/NRPN PARSER
///// The parser is located in handleGeneralControlChange(...), which
///// can be set up to be the handler for CC messages by the MIDI library.
/////
///// CC messages take one of a great many forms, which we handle in the parser
/////
///// 7-bit CC messages:
///// 1. number >=64 and < 96 or >= 102 and < 120, with value
/////           -> handleControlChange(channel, number, value, VALUE_7_BIT_ONLY)
/////
///// Potentially 7-bit CC messages, with MSB:
///// 1. number >= 0 and < 32, other than 6, with value
/////           -> handleControlChange(channel, number, value * 128 + 0, VALUE_MSB_ONLY)
/////
///// Full 14-bit CC messages:
///// 1. number >= 0 and < 32, other than 6, with MSB
///// 2. same number + 32, with LSB
/////           -> handleControlChange(channel, number, MSB * 128 + LSB, VALUE)
/////    NOTE: this means that a 14-bit CC message will have TWO handleControlChange calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 14-bit CC messages:
///// 1. number >= 32 and < 64, other than 38, with LSB, where number is 32 more than the last MSB.
/////           -> handleControlChange(channel, number, former MSB * 128 + LSB, VALUE)
/////
///// Lonely 14-bit CC messages (LSB only)
///// 1. number >= 32 and < 64, other than 38, with LSB, where number is NOT 32 more than the last MSB.
/////           -> handleControlChange(channel, number, 0 + LSB, VALUE)
/////           
/////
///// NRPN Messages:
///// All NRPN Messages start with:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
/////           At this point NRPN MSB is set to 0
/////
///// NRPN Messages then may have any sequence of:
///// 3.1 number == 6, with value   (MSB)
/////           -> handleNRPN(channel, parameter, value * 128 + 0, VALUE_MSB_ONLY)
/////                           At this point we set the NRPN MSB
///// 3.2 number == 38, with value   (LSB)
/////           -> handleNRPN(channel, parameter, current NRPN MSB * 128 + value, VALUE_MSB_ONLY)
///// 3.3 number == 96, with value   (Increment)
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, INCREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, INCREMENT)
/////       Also reset current NRPN MSB to 0
///// 3.4 number == 97, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, DECREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, DECREMENT)
/////       Also reset current NRPN MSB to 0
/////
/////
///// RPN Messages:
///// All RPN Messages start with:
///// 1. number == 99, with MSB of RPN parameter
///// 2. number == 98, with LSB of RPN parameter
/////           At this point RPN MSB is set to 0
/////
///// RPN Messages then may have any sequence of:
///// 3.1 number == 6, with value   (MSB)
/////           -> handleRPN(channel, parameter, value * 128 + 0, VALUE_MSB_ONLY)
/////                           At this point we set the RPN MSB
///// 3.2 number == 38, with value   (LSB)
/////           -> handleRPN(channel, parameter, current RPN MSB * 128 + value, VALUE_MSB_ONLY)
///// 3.3 number == 96, with value   (Increment)
/////       If value == 0
/////                   -> handleRPN(channel, parameter, 1, INCREMENT)
/////       Else
/////                   -> handleRPN(channel, parameter, value, INCREMENT)
/////       Also reset current RPN MSB to 0
///// 3.4 number == 97, with value
/////       If value == 0
/////                   -> handleRPN(channel, parameter, 1, DECREMENT)
/////       Else
/////                   -> handleRPN(channel, parameter, value, DECREMENT)
/////       Also reset current RPN MSB to 0
/////

///// NULL messages:            [RPN 127 with value of 127]
///// 1. number == 101, value = 127
///// 2. number == 100, value = 127
/////           [nothing happens, but parser resets]
/////
/////
///// The big problem we have is that the MIDI spec allows a bare MSB or LSB to arrive and that's it!
///// We don't know if another one is coming.  If a bare LSB arrives we're supposed to assume the MSB is 0.
///// But if the bare MSB comes we don't know if the LSB is next.  So we either have to ignore it when it
///// comes in (bad bad bad) or send two messages, one MSB-only and one MSB+LSB.  
///// This happens for CC, RPN, and NRPN.
/////
/////
///// Our parser maintains four bytes in a struct called _controlParser:
/////
///// 0. status.  This is one of:
/////             INVALID: the struct holds junk.  CC: the struct is building a CC.  
/////                     RPN_START, RPN_END: the struct is building an RPN.
/////                     NRPN_START, NRPN_END: the struct is building an NRPN.
///// 1. controllerNumberMSB.  In the low 7 bits.
///// 2. controllerNumberLSB.  In the low 7 bits.
///// 3. controllerValueMSB.  In the low 7 bits. This holds the previous MSB for potential "continuing" messages.

// Parser status values
#define INVALID 0
#define CC 1
#define NRPN_START 2
#define NRPN_END 3
#define RPN_START 4
#define RPN_END 5

struct _controlParser
    {
    uint8_t status = INVALID;
        
    // The high bit of the controllerNumberMSB is either
    // NEITHER_RPN_NOR_NRPN or it is RPN_OR_NRPN. 
    uint8_t controllerNumberMSB;
        
    // The high bit of the controllerNumberLSB is either
    // RPN or it is NRPN
    uint8_t controllerNumberLSB;
        
    // The controllerValueMSB is either a valid MSB or it is 
    // NO_MSB (128).
    uint8_t controllerValueMSB;
    
    // The controllerValueLSB is either a valid LSB or it is 
    // NO_LSB (128).
    uint8_t controllerValueLSB;
    
#ifdef INCLUDE_PROVIDE_RAW_CC
    uint8_t parseRawCC = false;
    uint8_t parse14BitCC = false;
#endif
    };
        


// We have two parsers: one for MIDI IN messages, and one for MIDI CONTROL messages
GLOBAL struct _controlParser midiInParser;
GLOBAL struct _controlParser midiControlParser;

#ifdef INCLUDE_PROVIDE_RAW_CC
void setParseRawCC(uint8_t val)
    {
    midiInParser.parseRawCC = val;
    }
void setParse14BitCC(uint8_t val)
    {
    midiInParser.parse14BitCC = val;
    }
#endif

void parse(_controlParser* parser, byte channel, byte number, byte value)
    {
    // BEGIN PARSER
    
#ifdef INCLUDE_PROVIDE_RAW_CC
    if (parser == &midiInParser && parser->parseRawCC )
        {
        parser->status = INVALID;
        handleControlChange(channel, number, value, VALUE_7_BIT_ONLY);
        }
    else
#endif
    
        // potentially 14-bit CC messages: the MSB was sent
        if (number < 6 || 
            (number > 6 && number < 32))
            {
            parser->status = CC;
            parser->controllerValueMSB = value;
            parser->controllerNumberMSB = number;
            handleControlChange(channel, number, value << 7, VALUE_MSB_ONLY);
            }
                
    // LSB for 14-bit CC messages, including continuation
        else if (number >= 32 && number < 64 && number != 38)
            {
            if (parser->status != CC || parser->controllerNumberMSB + 32 != number)  // need to reset
                {
                parser->status = CC;
                parser->controllerValueMSB = 0;
                }
            
            // okay we're ready to go now       
            
            if (parser->status == CC)
                {
#ifdef INCLUDE_PROVIDE_RAW_CC
                if (parser->parse14BitCC)
                    {
                    handleControlChange(channel, number, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
                    }
                else
#endif
                    {
                    handleControlChange(channel, number, value, VALUE_7_BIT_ONLY);
                    }
                }
            else parser->status = INVALID;
            }
                
    // 7-bit only CC messages, including channel mode
        else if ((number >= 64 && number < 96) || 
            (number >= 102 && number < 128))
            {
            parser->status = INVALID;
            handleControlChange(channel, number, value, VALUE_7_BIT_ONLY);
            }
                
    // Start of NRPN
        else if (number == 99)
            {
            parser->status = NRPN_START;
            parser->controllerNumberMSB = value;
            }

    // End of NRPN
        else if (number == 98)
            {
            parser->controllerValueMSB = 0;
            if (parser->status == NRPN_START)
                {
                parser->status = NRPN_END;
                parser->controllerNumberLSB = value;
                parser->controllerValueLSB = 0;
                parser->controllerValueMSB = 0;
                }
            else parser->status = INVALID;
            }
        
    // Start of RPN or NULL
        else if (number == 101)
            {
            if (value == 127)  // this is the NULL termination tradition, see for example http://www.philrees.co.uk/nrpnq.htm
                {
                parser->status = INVALID;
                }
            else
                {
                parser->status = RPN_START;
                parser->controllerNumberMSB = value;
                }
            }

    // End of RPN or NULL
        else if (number == 100)
            {
            parser->controllerValueMSB = 0;
            if (value == 127)  // this is the NULL termination tradition, see for example http://www.philrees.co.uk/nrpnq.htm
                {
                parser->status = INVALID;
                }
            else if (parser->status == RPN_START)
                {
                parser->status = RPN_END;
                parser->controllerNumberLSB = value;
                parser->controllerValueLSB = 0;
                parser->controllerValueMSB = 0;
                }
            }

        else   // we're currently parsing NRPN or RPN
            {
            uint16_t controllerNumber =  (((uint16_t) parser->controllerNumberMSB) << 7) | parser->controllerNumberLSB ;
            
            if (parser->status == NRPN_END)
                {
                if (number == 6)
                    {
                    parser->controllerValueMSB = value;
                    handleNRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | parser->controllerValueLSB, VALUE);
                    }
                                
                // Data Entry LSB for RPN, NRPN
                else if (number == 38)
                    {
                    parser->controllerValueLSB = value;
                    handleNRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | parser->controllerValueLSB, VALUE);
                    }
                                
                // Data Increment for RPN, NRPN
                else if (number == 96)
                    {
                    handleNRPN(channel, controllerNumber , (value ? value : 1), INCREMENT);
                    }

                // Data Decrement for RPN, NRPN
                else if (number == 97)
                    {
                    handleNRPN(channel, controllerNumber, (value ? value : 1), DECREMENT);
                    }
                else parser->status = INVALID;
                }
            else if (parser->status == RPN_END)
                {
                if (number == 6)
                    {
                    parser->controllerValueMSB = value;
                    handleRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | parser->controllerValueLSB, VALUE);
                    }
                                
                // Data Entry LSB for RPN, NRPN
                else if (number == 38)
                    {
                    parser->controllerValueLSB = value;
                    handleRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | parser->controllerValueLSB, VALUE);
                    }
                                
                // Data Increment for RPN, NRPN
                else if (number == 96)
                    {
                    handleRPN(channel, controllerNumber, (value ? value : 1), INCREMENT);
                    }

                // Data Decrement for RPN, NRPN
                else if (number == 97)
                    {
                    handleRPN(channel, controllerNumber, (value ? value : 1), DECREMENT);
                    }
                else parser->status = INVALID;
                }
            // Can only be 120...127, which should never appear here
            else parser->status = INVALID;
            }
    }
  

void handleGeneralControlChange(byte channel, byte number, byte value)
    {
    // generally we want to pass control changes through, EXCEPT if they're controlling us,
    // so we block the MIDI Control Channel if any.
    
    if (channel == options.channelControl)
        {
        parse(&midiControlParser, channel, number, value);
        }
                
    else if (!bypass)
        {
        // Some applications have special handling of channel In.
        if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
            {
            parse(&midiInParser, channel, number, value);

#ifdef INCLUDE_SPLIT
            // If we're doing keyboard splitting, we want to route control changes to the right place
            if (application == STATE_SPLIT && local.split.playing)
                {
                if ((options.splitControls == SPLIT_CONTROLS_RIGHT) || (options.splitControls == SPLIT_MIX))
                    MIDI.sendControlChange(number, value, options.channelOut);
                else
                    {
                    MIDI.sendControlChange(number, value, options.splitChannel);
                    }
                TOGGLE_OUT_LED();
                }
            else
#endif
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
			if (application == STATE_STEP_SEQUENCER)
				{
				// Are we playing along?  Route to the play along channel
				if (local.stepSequencer.performanceMode && options.stepSequencerPlayAlongChannel != CHANNEL_TRANSPOSE)
					{
					uint8_t channelOut = options.stepSequencerPlayAlongChannel;
					if (channelOut == 0)
						channelOut = options.channelOut;
					MIDI.sendControlChange(number, value, channelOut);  // generally pass through control changes
					TOGGLE_OUT_LED();
					}
				// If we're not playing along, route to Midi Out
				else
					{
					MIDI.sendControlChange(number, value, options.channelOut);
					TOGGLE_OUT_LED();
					}
				}
			else
#endif
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
			if (application == STATE_ARPEGGIATOR)
				{
				// Are we playing along?  Route to the play along channel
				if (local.arp.playing && local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
					{
					uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
					if (channelOut == 0)
						channelOut = options.channelOut;
					MIDI.sendControlChange(number, value, channelOut);  // generally pass through control changes
					TOGGLE_OUT_LED();
					}
				// If we're not playing along, route to Midi Out
				else
					{
					MIDI.sendControlChange(number, value, options.channelOut);  // generally pass through control changes
					TOGGLE_OUT_LED();
					}
				}
			else
#endif
			// block it off entirely.  Nobody wants it
                    { }
            }
        else 
#ifdef INCLUDE_THRU
		// Thru should route through everything that's NOT coming in the default channel and is NOT being blocked
		if (state == STATE_THRU_PLAY)
			{
			if (!options.thruBlockOtherChannels)
				{
				// Note this does NOT include the merge channel
				MIDI.sendControlChange(number, value, channel);  // generally pass through control changes
				TOGGLE_OUT_LED();
				}
			}
		else
#endif
	// In general we pass through everything else
			{
			MIDI.sendControlChange(number, value, channel);
			TOGGLE_OUT_LED();
			}
		}
    else
        {
        TOGGLE_OUT_LED();
        }
    }




// This is a little support function meant to shorten code in the following functions.
void toggleLEDsAndSetNewItem(byte _itemType)
    {
    newItem = NEW_ITEM;
    itemType = _itemType;
    TOGGLE_IN_LED(); 
    TOGGLE_OUT_LED();
    }

void handleProgramChange(byte channel, byte number)
    {
    uint8_t isChannelIn = updateMIDI(channel, MIDI_PROGRAM_CHANGE, number, 1);
    if (!bypass) 
        {
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
			if (application == STATE_ARPEGGIATOR)
				{
				// Are we playing along?  Route to the play along channel
				if (local.arp.playing && local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
					{
					uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
					if (channelOut == 0)
						channelOut = options.channelOut;
                    MIDI.sendProgramChange(number, channelOut);
					TOGGLE_OUT_LED();
					}
				// If we're not playing along, route to Midi Out
				else
					{
                    MIDI.sendProgramChange(number, options.channelOut);
					TOGGLE_OUT_LED();
					}
				}
			else
#endif
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
			if (application == STATE_STEP_SEQUENCER)
				{
				// Are we playing along?  Route to the play along channel
				if (local.stepSequencer.performanceMode && options.stepSequencerPlayAlongChannel != CHANNEL_TRANSPOSE)
					{
					uint8_t channelOut = options.stepSequencerPlayAlongChannel;
					if (channelOut == 0)
						channelOut = options.channelOut;
                    MIDI.sendProgramChange(number, channelOut);
					TOGGLE_OUT_LED();
					}
				// If we're not playing along, route to Midi Out
				else
					{
                    MIDI.sendProgramChange(number, options.channelOut);
					TOGGLE_OUT_LED();
					}
				}
			else
#endif
#ifdef INCLUDE_SPLIT
            // One exception: if we're doing keyboard splitting, we want to route control changes to the right place
            if (application == STATE_SPLIT && local.split.playing && (channel == options.channelIn || options.channelIn == CHANNEL_OMNI))
                {
                if ((options.splitControls == SPLIT_CONTROLS_RIGHT) || (options.splitControls == SPLIT_MIX))
                    MIDI.sendProgramChange(number, options.channelOut);
                else
                    MIDI.sendProgramChange(number, options.splitChannel);
                TOGGLE_OUT_LED();
                }
            else
#endif
#ifdef INCLUDE_THRU
                if (state == STATE_THRU_PLAY)
                    {
                    // only pass through if the data's NOT coming in the default channel
                    if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI)
                        && channel != options.thruMergeChannelIn)
                        {
                        MIDI.sendProgramChange(number, channel);
                        TOGGLE_OUT_LED();
                        }
                    else if (channel == options.thruMergeChannelIn)  //  merge hasn't been sent to newitem yet
                        {
                        newItem = NEW_ITEM;
                        itemType = MIDI_PROGRAM_CHANGE;
                        itemNumber = number;
                        itemValue = 1;
                        itemChannel = channel;
                        }
                    }
                else
#endif
                    if (!isChannelIn)
                        {
#ifdef INCLUDE_THRU
                        if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
#endif
                            {
                            MIDI.sendProgramChange(number, channel);
                            TOGGLE_OUT_LED();
                            }
                        }
        }
    else
        TOGGLE_OUT_LED();
    }
  
void handleAfterTouchChannel(byte channel, byte pressure)
    {
    uint8_t isChannelIn = updateMIDI(channel, MIDI_AFTERTOUCH, 1, pressure);
    if (!bypass) 
        {
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
        if (!bypass && (application == STATE_ARPEGGIATOR && local.arp.playing && local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE))
            {
            uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
            if (channelOut == 0)
                channelOut = options.channelOut;
            MIDI.sendAfterTouch(pressure, channelOut);
            }
        else
#endif

#ifdef INCLUDE_SPLIT
            // One exception: if we're doing keyboard splitting, we want to route control changes to the right place
            if (application == STATE_SPLIT && local.split.playing && (channel == options.channelIn || options.channelIn == CHANNEL_OMNI))
                {
                if ((options.splitControls == SPLIT_CONTROLS_RIGHT) || (options.splitControls == SPLIT_MIX))
                    MIDI.sendAfterTouch(pressure, options.channelOut);
                else
                    MIDI.sendAfterTouch(pressure, options.splitChannel);
                TOGGLE_OUT_LED();
                }
            else
#endif
#ifdef INCLUDE_THRU
                if (state == STATE_THRU_PLAY)
                    {
                    // only pass through if the data's NOT coming in the default channel
                    if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI)
                        && channel != options.thruMergeChannelIn)
                        {
                        MIDI.sendAfterTouch(pressure, channel);
                        TOGGLE_OUT_LED();
                        }
                    else if (channel == options.thruMergeChannelIn)  //  merge hasn't been sent to newitem yet
                        {
                        newItem = NEW_ITEM;
                        itemType = MIDI_AFTERTOUCH;
                        itemNumber = 1;
                        itemValue = pressure;
                        itemChannel = channel;
                        }
                    }
                else
#endif
                    if (!isChannelIn)
                        { 
#ifdef INCLUDE_THRU
                        if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
#endif
                            {
                            MIDI.sendAfterTouch(pressure, channel);
                            TOGGLE_OUT_LED();
                            }
                        }
        }
    else
        TOGGLE_OUT_LED();
    }
  
void handlePitchBend(byte channel, int bend)
    {
    uint8_t isChannelIn = updateMIDI(channel, MIDI_PITCH_BEND, 1, (uint16_t) bend + 8192);
    if (!bypass) 
        {
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
        if (application == STATE_ARPEGGIATOR)
        	{
        	// are we playing along?  Route to the play along channel
        	if (local.arp.playing && local.arp.performanceMode && options.arpeggiatorPlayAlongChannel != ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE)
				{
				uint8_t channelOut = options.arpeggiatorPlayAlongChannel;
				if (channelOut == 0)
					channelOut = options.channelOut;
				MIDI.sendPitchBend(bend, channelOut);
                    TOGGLE_OUT_LED();
				}
			else if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
				{
				MIDI.sendPitchBend(bend, options.channelOut);
                    TOGGLE_OUT_LED();
				}
			}
        else
#endif
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
        if (application == STATE_STEP_SEQUENCER)
        	{
        	// are we playing along?  Route to the play along channel
        	if (local.stepSequencer.performanceMode && options.stepSequencerPlayAlongChannel != CHANNEL_TRANSPOSE)
				{
				uint8_t channelOut = options.stepSequencerPlayAlongChannel;
				if (channelOut == 0)
					channelOut = options.channelOut;
				MIDI.sendPitchBend(bend, channelOut);
                    TOGGLE_OUT_LED();
				}
			else if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
				{
				MIDI.sendPitchBend(bend, options.channelOut);
                    TOGGLE_OUT_LED();
				}
			}
        else
#endif
#ifdef INCLUDE_SPLIT
            // One exception: if we're doing keyboard splitting, we want to route control changes to the right place
            if (application == STATE_SPLIT && local.split.playing && (channel == options.channelIn || options.channelIn == CHANNEL_OMNI))
                {
                if ((options.splitLayerNote == NO_NOTE) && (options.splitControls != SPLIT_MIX))
                    {
                    if (options.splitControls == SPLIT_CONTROLS_RIGHT)
                        MIDI.sendPitchBend(bend, options.channelOut);
                    else
                        MIDI.sendPitchBend(bend, options.splitChannel);
                    TOGGLE_OUT_LED();
                    }
                else    // send to both
                    {
                    MIDI.sendPitchBend(bend, options.channelOut);
                    MIDI.sendPitchBend(bend, options.splitChannel);
                    TOGGLE_OUT_LED();
                    }
                }
            else
#endif
#ifdef INCLUDE_THRU
                if (state == STATE_THRU_PLAY)
                    {
                    // only pass through if the data's NOT coming in the default channel
                    if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI)
                        && channel != options.thruMergeChannelIn)
                        {
                        MIDI.sendPitchBend(bend, channel);
                        TOGGLE_OUT_LED();
                        }
                    else if (channel == options.thruMergeChannelIn)  //  merge hasn't been sent to newitem yet
                        {
                        newItem = NEW_ITEM;
                        itemType = MIDI_PITCH_BEND;
                        itemNumber = 1;
                        itemValue = (uint16_t) bend + 8192;
                        itemChannel = channel;
                        }
                    }
                else
#endif
                    if (!isChannelIn)
                        { 
#ifdef INCLUDE_THRU
                        if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
#endif
                            {
                            MIDI.sendPitchBend(bend, channel);
                            TOGGLE_OUT_LED();
                            }
                        }
        }
    else
        TOGGLE_OUT_LED();
    }
  
void handleTimeCodeQuarterFrame(byte data)
    {
    toggleLEDsAndSetNewItem(MIDI_TIME_CODE);
    //itemType = MIDI_TIME_CODE;

    // always pass through.
    MIDI.sendTimeCodeQuarterFrame(data);
    }
  

void handleSystemExclusive(byte* array, unsigned size)
    {
    toggleLEDsAndSetNewItem(MIDI_SYSTEM_EXCLUSIVE);
    //itemType = MIDI_SYSTEM_EXCLUSIVE;

    // always pass through
    MIDI.sendSysEx(size, array);
    }
  
void handleSongPosition(unsigned beats)
    {
    toggleLEDsAndSetNewItem(MIDI_SONG_POSITION);
    //itemType = MIDI_SONG_POSITION;

    // always pass through
    MIDI.sendSongPosition(beats);
    }

void handleSongSelect(byte songnumber)
    {
    toggleLEDsAndSetNewItem(MIDI_SONG_SELECT);
    //itemType = MIDI_SONG_SELECT;

    // always pass through
    MIDI.sendSongSelect(songnumber);
    }
  
void handleTuneRequest()
    {
    toggleLEDsAndSetNewItem(MIDI_TUNE_REQUEST);
    //itemType = MIDI_TUNE_REQUEST;

    // always pass through
    MIDI.sendTuneRequest();
    }
  
void handleActiveSensing()
    {
    toggleLEDsAndSetNewItem(MIDI_ACTIVE_SENSING);
    //itemType = MIDI_ACTIVE_SENSING;
        
    // always pass through
    MIDI.sendRealTime(MIDIActiveSensing);
    }
  
void handleSystemReset()
    {
    toggleLEDsAndSetNewItem(MIDI_SYSTEM_RESET);
    //itemType = MIDI_SYSTEM_RESET;

    // always pass through
    MIDI.sendRealTime(MIDISystemReset);
    }






/// MIDI OUT SUPPORT
///
/// The following functions foo(...) are called instead of the MIDI.foo(...)
/// equivalents in many cases because they filter or modify the commands before
/// they are sent out: either blocking them because we're in bypass mode,
/// or changing the volume, or transposing, or toggling the MIDI out LED.


/// Sends out pressure, transposing as appropriate
void sendPolyPressure(uint8_t note, uint8_t pressure, uint8_t channel)
    {
    if (bypassOut) return;
  
#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    MIDI.sendPolyPressure((uint8_t) n, pressure, channel);
#else
    MIDI.sendPolyPressure(note, pressure, channel);
#endif
    TOGGLE_OUT_LED();
    }
            

/// Sends a note on, transposing as appropriate, and adjusting
/// the velocity as appropriate.  Also sends out CV.
void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    if (bypassOut) return;
  
#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    uint16_t v = velocity;
    if (options.volume < 3)
        v = v >> (3 - options.volume);
    else if (options.volume > 3)
        v = v << (options.volume - 3);
    if (v > 127) v = 127;
    MIDI.sendNoteOn((uint8_t) n, (uint8_t) v, channel);
#else
    MIDI.sendNoteOn(note, velocity, channel);
#endif

    TOGGLE_OUT_LED();
    }

/// Sends a note off, transposing as appropriate
void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    if (bypassOut) return;

#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    MIDI.sendNoteOff((uint8_t) n, velocity, channel);
#else
    MIDI.sendNoteOff(note, velocity, channel);
#endif
    // dont' toggle the LED because if we're going really fast it toggles
    // the LED ON and OFF for a noteoff/noteon pair and you can't see the LED
    }


         
/// Sends an all notes off on ALL channels, regardless of whether bypass is turned on or not.
void sendAllSoundsOffDisregardBypass(uint8_t channel)
    {
    if (channel == CHANNEL_OMNI)
        {
        for(uint8_t i = LOWEST_MIDI_CHANNEL; i <= HIGHEST_MIDI_CHANNEL; i++)
            {
#ifdef USE_ALL_NOTES_OFF
            MIDI.sendControlChange(123, 0, i);          // All Notes Off
#endif
#ifdef USE_ALL_SOUNDS_OFF
            MIDI.sendControlChange(120, 0, i);              // All Sound Off
#endif
            }
        }
    else
        {
#ifdef USE_ALL_NOTES_OFF
        MIDI.sendControlChange(123, 0, channel);        // All Notes Off
#endif
#ifdef USE_ALL_SOUNDS_OFF
        MIDI.sendControlChange(120, 0, channel);        // All Sound Off
#endif
        }
    }
        

/// Sends an all notes off on ALL channels, but only if bypass is off.
void sendAllSoundsOff(uint8_t channel)
    {
    if (!bypassOut) 
        sendAllSoundsOffDisregardBypass(channel);
    }




#define DONT_SEND_14_BIT_CC

// SEND CONTROLLER COMMAND
// Sends a controller command, one of:
// CC, NRPN, RPN, PC
// [Only if INCLUDE_EXTENDED_CONTROL_SIGNALS]: Pitch Bend, Aftertouch
//
// These are defined by the CONTROL_TYPE_* constants defined elsewhere
//
// Some commands (CC, NRPN, RPN) have COMMAND NUMBERS.  The others ignore the provided number.
//
// Each command is associated with a VALUE.  All values are 14 bits and take the form
// of an MSB (the high 7 bits) and an LSB (the low 7 bits).  Some data expects lower
// resolution numbers than that -- in this case you must shift your data so it's zero-
// padded on the right.  For example, if you want to pass in a 7-bit
// number (for PC, some CC values, or AFTERTOUCH) you should do so as (myval << 7).
// If you want to pass in a signed PITCH BEND value (an int16_t from -8192...8191), you
// should do so as (uint16_t myval + MIDI_PITCHBEND_MIN).
//
// It's good practice to send a NULL RPN after sending an RPN or NRPN, but it's 50% more data
// in an already slow command.  So Gizmo doesn't do it by default.  You can make Gizmo do it
// with #define INCLUDE_SEND_NULL_RPN   [I'd set that somewhere in All.h]
//
// Here are the valid numbers and values for different commands:
//
// COMMAND      NUMBERS         VALUES          NOTES
// OFF          [everything is ignored, this is just a NOP]
// CC           0-31            0-16383         1. If you send 7-bit data (zero-padded, shifted << 7) then the LSB will not be sent.
//                                                 Also LSB not sent if DONT_SEND_14_BIT_CC is defined (which is the default).  So this normally doesn't occur.
// CC           32-127          0-127           1. Zero-pad your 7-bit data (shift it << 7).
//                                                                      2. Some numbers are meant for special functions.  Unless you know what you're doing,
//                                                                         it'd be wise not to send on numbers 6, 32--63, 96--101, or 120--127
// NRPN/RPN     0-16383         0-16383         1. If you send 7-bit data (zero-padded) then the LSB will not be sent.
//                                                                      2. The NULL RPN terminator will only be sent if #define INCLUDE_SEND_NULL_RPN is on.
//                                                                         By default it's off.  See the end of http://www.philrees.co.uk/nrpnq.htm
//                                                                      3. You can also send in an RPN/NRPN DATA INCREMENT or DECREMENT.  To do this, pass in the
//                                                                         value CONTROL_VALUE_INCREMENT [or DECREMENT] * 128 + DELTA.  A DELTA of 0 is the 
//                                                                         same as 1 and is the most common usage.
// PC           [ignored]       0-127           1. Zero-pad your 7-bit data (shift it << 7)
// BEND         [ignored]       0-16383         1. BEND is normally signed (-8192...8191).  If you have the value as a signed int16_t,
//                                                                         pass it in as (uint16_t) (myval + MIDI_PITCHBEND_MIN)
// AFTERTOUCH [ignored] 0-127           1. Zero-pad your 7-bit data (shift it << 7)


void sendControllerCommand(uint8_t commandType, uint16_t commandNumber, uint16_t fullValue, uint8_t channel)
    {
    uint8_t msb = fullValue >> 7;
    uint8_t lsb = fullValue & 127;
    
    if (bypassOut)
        return;
        
    // amazingly, putting this here reduces the code by 4 bytes.  Though it could
    // easily be removed as the switch below handles it!
    if (commandType == CONTROL_TYPE_OFF)
        return;
    
    if (channel == 0)                // we do not output
        return;
    
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
            
            MIDI.sendControlChange(commandNumber, msb, channel);
#ifdef DONT_SEND_14_BIT_CC
// do nothing
#else
            if (commandNumber < 32 && lsb != 0)             // send optional lsb if appropriate
                {
                MIDI.sendControlChange(commandNumber + 32, lsb, channel);
                }
#endif
            TOGGLE_OUT_LED(); 
            }
        break;
        case CONTROL_TYPE_NRPN:
            {
            // Send 99 for NRPN, or 101 for RPN
            MIDI.sendControlChange(99, commandNumber >> 7, channel);
            MIDI.sendControlChange(98, commandNumber & 127, channel);  // LSB
            if (msb == CONTROL_VALUE_INCREMENT)
                {
                MIDI.sendControlChange(96, lsb, channel);
                }
            else if (msb == CONTROL_VALUE_DECREMENT)
                {
                MIDI.sendControlChange(97, lsb, channel);
                }
            else
                {
                MIDI.sendControlChange(6, msb, channel);  // MSB
                if (lsb != 0)
                    MIDI.sendControlChange(38, lsb, channel);  // LSB
                }
#ifdef INCLUDE_SEND_NULL_RPN
            MIDI.sendControlChange(101, 127, channel);  // MSB of NULL command
            MIDI.sendControlChange(100, 127, channel);  // LSB of NULL command
#endif
            TOGGLE_OUT_LED(); 
            }
        break;
        // note merging these actually loses bytes.  I tried.
        case CONTROL_TYPE_RPN:
            {
            MIDI.sendControlChange(101, commandNumber >> 7, channel);
            MIDI.sendControlChange(100, commandNumber & 127, channel);  // LSB
            if (msb == CONTROL_VALUE_INCREMENT)
                {
                MIDI.sendControlChange(96, lsb, channel);
                }
            else if (msb == CONTROL_VALUE_DECREMENT)
                {
                MIDI.sendControlChange(97, lsb, channel);
                }
            else
                {
                MIDI.sendControlChange(6, msb, channel);  // MSB
                if (lsb != 0)
                    MIDI.sendControlChange(38, lsb, channel);  // LSB
                }
#ifdef INCLUDE_SEND_NULL_RPN
            MIDI.sendControlChange(101, 127, channel);  // MSB of NULL command
            MIDI.sendControlChange(100, 127, channel);  // LSB of NULL command
#endif
            TOGGLE_OUT_LED(); 
            }
        break;
        case CONTROL_TYPE_PC:
            {
            if (msb <= MAXIMUM_PC_VALUE)
                MIDI.sendProgramChange(msb, channel);
            TOGGLE_OUT_LED(); 
            }
        break;
        
#ifdef INCLUDE_EXTENDED_CONTROL_SIGNALS
        case CONTROL_TYPE_PITCH_BEND:
            {
            // move from 0...16k to -8k...8k
            MIDI.sendPitchBend(((int)fullValue) - MIDI_PITCHBEND_MIN, channel);
            }
        break;
        case CONTROL_TYPE_AFTERTOUCH:
            {
            MIDI.sendAfterTouch(msb, channel);
            }
        break;
#endif
        }
    }
        
        
        
#ifdef INCLUDE_SYSEX
        
void loadHeader(uint8_t bytes[])
    {
    bytes[0] = 0xF0;
    bytes[1] = 0x7D;
    bytes[2] = 'G';
    bytes[3] = 'I';
    bytes[4] = 'Z';
    bytes[5] = 'M';
    bytes[6] = 'O';
    bytes[7] = SYSEX_VERSION;
    }
        
void sendSlotSysex()
    {
    loadSlot(local.sysex.slot);

    // Our format is:
    // 0xF0
    // 0x7D                         [Private, Test, Educational Use]
    // G I Z M O
    // Version number       [Currently 0]
    // Sysex Type           [ 0 = slot, 1 = Arp]
    // Nybblized Data, high nybble first
    // checksum, just sum of data
    // 0xF7
    uint8_t bytes[sizeof(struct _slot) * 2 + 11];
    loadHeader(bytes);
    bytes[8] = SYSEX_TYPE_SLOT;
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _slot); i++)
        {
        bytes[9 + i * 2] = (uint8_t)((data.bytes[i] >> 4) & 0xF);  // unsigned char right shifts are probably logical shifts, but we mask anyway
        bytes[9 + i * 2 + 1] = (uint8_t)(data.bytes[i] & 0xF);
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];

        /*
        if (data.bytes[i] != ((uint8_t)(bytes[9 + i * 2] << 4) | (bytes[9 + i * 2 + 1] & 0xF)))
            debug(999);
        */
        }
    bytes[sizeof(struct _slot) * 2 + 11 - 2] = (sum & 127);
    bytes[sizeof(struct _slot) * 2 + 11 - 1] = 0xF7;
    MIDI.sendSysEx(sizeof(struct _slot) * 2 + 11, bytes, true);
    }        

void sendArpSysex()
    {
    LOAD_ARPEGGIO(local.sysex.slot);

    // Our format is:
    // 0xF0
    // 0x7D                         [Private, Test, Educational Use]
    // G I Z M O
    // Version number       [Currently 0]
    // Sysex Type           [ 0 = slot, 1 = Arp]
    // Nybblized Data, high nybble first
    // checksum, just sum of data
    // 0xF7
    uint8_t bytes[sizeof(struct _arp) * 2 + 11];
    loadHeader(bytes);
    bytes[8] = SYSEX_TYPE_ARP;
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _arp); i++)
        {
        bytes[9 + i * 2] = (uint8_t)((data.bytes[i] >> 4) & 0xF);  // unsigned char right shifts are probably logical shifts, but we mask anyway
        bytes[9 + i * 2 + 1] = (uint8_t)(data.bytes[i] & 0xF);
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];
        }
    bytes[sizeof(struct _arp) * 2 + 11 - 2] = (sum & 127);
    bytes[sizeof(struct _arp) * 2 + 11 - 1] = 0xF7;
    MIDI.sendSysEx(sizeof(struct _arp) * 2 + 11, bytes, true);
    }        
        
uint8_t receiveSlotSysex(unsigned char* bytes)
    {
    // verify checksum
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _slot); i++)
        {
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];
        }
    if ((sum & 127) != bytes[sizeof(struct _slot) * 2 + 11 - 2]) // second to last
        return false;
        
    // load
    for(uint16_t i = 0; i < sizeof(struct _slot); i++)
        {
        data.bytes[i] = (uint8_t)(bytes[9 + i * 2] << 4) | (bytes[9 + i * 2 + 1] & 0xF);
        }
    saveSlot(local.sysex.slot);
    return true;
    }

uint8_t receiveArpSysex(unsigned char* bytes)
    {
    // verify checksum
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _arp); i++)
        {
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];
        }
    if ((sum & 127) != bytes[sizeof(struct _arp) * 2 + 11 - 2]) // second to last
        return false;
        
    // load
    for(uint16_t i = 0; i < sizeof(struct _arp); i++)
        {
        data.bytes[i] = (uint8_t)(bytes[9 + i * 2] << 4) | (bytes[9 + i * 2 + 1] & 0xF);
        }
    SAVE_ARPEGGIO(local.sysex.slot);
    return true;
    }


void handleSysex(unsigned char* bytes, int len)
    {
    // We only process incoming sysex *at all*, even if to generate an error message, if we're
    // in STATE_SYSEX_GO.
    if (state != STATE_SYSEX_GO) return;

    // Our format is:
    // 0xF0
    // 0x7D                         [Private, Test, Educational Use]
    // G I Z M O
    // Version number       [Currently 0]
    // Sysex Type           [ 0 = slot, 1 = Arp]
    // Nybblized Data, high nybble first
    // checksum, just sum of data
    // 0xF7
        
    if (bytes[1] == 0x7D && 
        bytes[2] == 'G' && 
        bytes[3] == 'I' && 
        bytes[4] == 'Z' && 
        bytes[5] == 'M' && 
        bytes[6] == 'O' && 
        bytes[7] == SYSEX_VERSION)  // it's me
        {
        if (local.sysex.type == SYSEX_TYPE_SLOT && bytes[8] == SYSEX_TYPE_SLOT)
            {
            if (len != sizeof(struct _slot) * 2 + 11 || !receiveSlotSysex(bytes))
                {
                local.sysex.received = RECEIVED_BAD;
                }
            else
                {
                local.sysex.received++;
                if (local.sysex.received <= 0)  // previous was BAD or WRONG, or we wrapped around
                    local.sysex.received = 1;
                }
            }
        else if (local.sysex.type == SYSEX_TYPE_ARP && bytes[8] == SYSEX_TYPE_ARP)
            {
            if (len != sizeof(struct _arp) * 2 + 11 | !receiveArpSysex(bytes))
                {
                local.sysex.received = RECEIVED_BAD;
                }
            else
                {
                local.sysex.received++;
                if (local.sysex.received <= 0)  // previous was BAD or WRONG, or we wrapped around
                    local.sysex.received = 1;
                }
            }
        else
            {
            local.sysex.received = RECEIVED_WRONG;  // we're not the right type (slot vs. arp) to receive
            }
        }
    else
        {
        local.sysex.received = RECEIVED_BAD;
        }
    }   
        
#endif INCLUDE_SYSEX





