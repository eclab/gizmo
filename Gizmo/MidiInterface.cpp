////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#include "All.h"



#ifdef INCLUDE_VOLTAGE


////////// VOLTAGE SETTERS


// Sets DAC_A when appropriate
//
// TODO: Perhaps we should allow high-resolution pitch bend in
// this as well.  It'd require (1) storing the current note on, and
// (2) allowing the user to specify the pitch bend range.
void setPrimaryVoltage(uint8_t voltage, uint8_t on)
    {
    // should we bail?
    if (options.voltage == NO_VOLTAGE 
#ifdef INCLUDE_EXTENDED_CONTROLLER
        || (application == STATE_CONTROLLER && 
                (options.leftKnobControlType == CONTROL_TYPE_VOLTAGE_A ||
                options.rightKnobControlType != CONTROL_TYPE_VOLTAGE_A))
#endif
        )                 
        return; 

    // first always turn off gate if requested
    if (!on)
        {
        *port_VOLTAGE_GATE &= ~VOLTAGE_GATE_mask;
        }       

    // per setNote(), the only valid notes are MIDI# 36 ... 60, which
    // will correspond to 0...5V with 1V per octave
    setNote(DAC_A, voltage); 
        
    if (on) 
        {
        *port_VOLTAGE_GATE |= VOLTAGE_GATE_mask;
        }
    }


// Sets DAC_B when appropriate
void setSecondaryVoltage(uint8_t voltage)
    {
    // should we bail?
    if (options.voltage == NO_VOLTAGE 
#ifdef INCLUDE_EXTENDED_CONTROLLER
        || (application == STATE_CONTROLLER && 
                (options.leftKnobControlType == CONTROL_TYPE_VOLTAGE_B ||
                options.rightKnobControlType != CONTROL_TYPE_VOLTAGE_B))
#endif
        )

        // We translate 0...127 to 0...4095.  Wish it was higher
        // resolution than 7 bits, but there you have it.  :-(
        setValue(DAC_B, (((uint16_t) voltage) * 4095) / 127);
    }


#endif











////////// MIDI HANDLERS  


/// Yet ANOTHER small code-shortening function!
/// Updates incoming midi items only if they came in the proper channel, and returns TRUE
/// if this occurred. 

uint8_t updateMIDI(byte channel, uint8_t _itemType, uint16_t _itemNumber, uint16_t _itemValue)
    {
    TOGGLE_IN_LED();
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
void handleClockCommand(void (*clockFunction)(uint8_t), midi::MidiType clockMIDI)
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
//        local.measure.resetCounter = 0;
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

//#ifdef INCLUDE_MEASURE
//    if (application == STATE_MEASURE)
//        {
//        if (++local.measure.resetCounter >= MEASURE_COUNTER_MAX)
//            {
//            resetMeasure();
//            }
//        }
//#endif
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

//#ifdef INCLUDE_MEASURE
//    if (application == STATE_MEASURE)
//        {
//        local.measure.resetCounter = 0;
//        }
//#endif
    }
  
void handleClock()
    {
    handleClockCommand(pulseClock, MIDIClock);
    }



void handleNoteOff(byte channel, byte note, byte velocity)
    {
    // is data coming in the default channel?
    if (updateMIDI(channel, MIDI_NOTE_OFF, note, velocity))
        {
#ifdef INCLUDE_ARPEGGIATOR
        if (application == STATE_ARPEGGIATOR && local.arp.playing)
            arpeggiatorRemoveNote(note);
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
    // is data coming in the default channel?
    if (updateMIDI(channel, MIDI_NOTE_ON, note, velocity))
        {
#ifdef INCLUDE_ARPEGGIATOR
        if (application == STATE_ARPEGGIATOR && local.arp.playing)
            {
            // the arpeggiation velocity shall be the velocity of the most recently added note
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
#define VALUE 0                                 // 14-bit values given to CC
#define VALUE_MSB_ONLY 1                // 
#define INCREMENT 2
#define DECREMENT 3
#define VALUE_7_BIT_ONLY 4  // this is last so we have a contiguous switch for NRPN and RPN handlers, which don't use this


// These are button states sent to use via NRPN messages
GLOBAL uint8_t buttonState[3] = { 0, 0, 0 };



void handleControlChange(byte channel, byte number, uint16_t value, byte type)
    {
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

#ifdef INCLUDE_CC_CONTROL
    if (channel == options.channelControl)  // options.channelControl can be zero remember
        {
        lockoutPots = 1;
                        
        switch (number)
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
                toggleBypass();
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
#endif

    } 
        
        
        
void handleNRPN(byte channel, uint16_t parameter, uint16_t value, uint8_t valueType)
    {
    if (updateMIDI(channel, MIDI_NRPN_14_BIT, parameter, value))
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
                toggleBypass();
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
///// Potentially 7-bit CC messages:
///// 1. number >= 0 and < 32, other than 6, with value
/////           -> handleControlChange(channel, number, value, VALUE_MSB_ONLY)
/////
///// 14-bit CC messages:
///// 1. number >= 0 and < 32, other than 6, with MSB
///// 2. same number + 32, with LSB
/////           -> handleControlChange(channel, number, value, VALUE)
/////    NOTE: this means that a 14-bit CC message will have TWO handleControlChange calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 14-bit CC messages:
///// 1. same number again + 32, with LSB
/////           -> handleControlChange(channel, number, revised value, VALUE)
/////           It's not clear if this is valid but I think it is
/////
///// Potentially 7-bit NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 6, with value
/////           -> handleNRPN(channel, parameter, value, VALUE_MSB_ONLY)
/////
///// 14-bit NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 6, with MSB of value
///// 4. number == 38, with LSB of value
/////           -> handleNRPN(channel, parameter, value, VALUE)
/////    NOTE: this means that a 14-bit NRPN message will have TWO handleNRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 7-bit NRPN messages:
///// 4. number == 38 again, with value
/////           -> handleNRPN(channel, number, revised value, VALUE_MSB_ONLY)
/////
///// Continuing 14-bit NRPN messages A:
///// 3. number == 6 again, with MSB of value
/////           -> handleNRPN(channel, number, revised value, VALUE)
/////
///// Continuing 14-bit NRPN messages C:
///// 3. number == 6 again, with MSB of value
///// 4. number == 38 again, with LSB of value
/////           -> handleNRPN(channel, number, revised value, VALUE)
/////    NOTE: this means that a continuing 14-bit NRPN message will have TWO handleNRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Incrementing NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 96, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, INCREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, INCREMENT)
/////
///// Decrementing NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 97, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, DECREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, DECREMENT)
/////
///// Potentially 7-bit RPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 6, with value
/////           -> handleNRPN(channel, parameter, value, VALUE_MSB_ONLY)
/////
///// 14-bit RPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 6, with MSB of value
///// 4. number == 38, with LSB of value
/////           -> handleNRPN(channel, parameter, value, VALUE)
/////    NOTE: this means that a 14-bit NRPN message will have TWO handleNRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 7-bit RPN messages A:
///// 3. number == 6 again, with value
/////           -> handleRPN(channel, number, revised value, VALUE_MSB_ONLY)
/////
///// Continuing 14-bit RPN messages A:
///// 4. number == 38 again, with new LSB of value
/////           -> handleRPN(channel, number, revised value, VALUE)
/////
///// Continuing 14-bit RPN messages B:
///// 3. number == 6 again, with MSB of value
///// 4. number == 38 again, with LSB of value
/////           -> handleRPN(channel, number, revised value, VALUE)
/////    NOTE: this means that a continuing 14-bit RPN message will have TWO handleRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Incrementing RPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 96, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, INCREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, INCREMENT)
/////
///// Decrementing NRPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 97, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, DECREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, DECREMENT)
/////
///// NULL messages:
///// 1. number == 101, value = 127
///// 2. number == 100, value = 127
/////           [nothing happens, but parser resets]
/////
/////
///// The big problem we have is that the MIDI spec got the MSB and LSB backwards for their data
///// entry values, so we don't now if the LSB is coming and have to either ignore it when it comes
///// in or send two messages, one MSB-only and one MSB+LSB.  This happens for CC, RPN, and NRPN.
/////
/////
///// Our parser maintains four bytes in a struct called _controlParser:
/////
///// 0. status.  This is one of:
/////             INVALID: the struct holds junk.  CC: the struct is building a CC.  
/////                     RPN_START, RPN_END, RPN_MSB, RPN_INCREMENT_DECREMENT: the struct is building an RPN.
/////                     NRPN_START, NRPN_END, NRPN_MSB, NRPN_INCREMENT_DECREMENT: the struct is building an NRPN.
///// 1. controllerNumberMSB.  In the low 7 bits.
///// 2. controllerNumberLSB.  In the low 7 bits.
///// 3. controllerValueMSB.  In the low 7 bits. This holds the previous MSB for potential "continuing" messages.

// Parser status values
#define INVALID 0
#define CC 1
#define NRPN_START 2
#define NRPN_END 3
#define NRPN_MSB 4
#define NRPN_INCREMENT_DECREMENT 5
#define RPN_START 6
#define RPN_END 7
#define RPN_MSB 8
#define RPN_INCREMENT_DECREMENT 9

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
    };
        



// We have two parsers: one for MIDI IN messages, and one for MIDI CONTROL messages
GLOBAL struct _controlParser midiInParser;
GLOBAL struct _controlParser midiControlParser;
    

void parse(_controlParser* parser, byte channel, byte number, byte value)
    {

    // BEGIN PARSER
    
#ifdef INCLUDE_PROVIDE_RAW_CC
    if (options.midiInProvideRawCC && parser == &midiInParser)
        {
        parser->status = INVALID;
        handleControlChange(channel, number, value, VALUE_7_BIT_ONLY);
        }
    else
#endif
    
        // potentially 14-bit CC messages
        if (number < 6 || 
            (number > 6 && number < 32))
            {
            parser->status = CC;
            parser->controllerValueMSB = value;
            handleControlChange(channel, number, value << 7, VALUE_MSB_ONLY);
            }
                
    // LSB for 14-bit CC messages, including continuation
        else if (number >= 32 && number < 64 && number != 38)
            {
            if (parser->status == CC)
                {
                handleControlChange(channel, number, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
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
            if (parser->status == NRPN_START)
                {
                parser->status = NRPN_END;
                parser->controllerNumberLSB = value;
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
            if (value == 127)  // this is the NULL termination tradition, see for example http://www.philrees.co.uk/nrpnq.htm
                {
                parser->status = INVALID;
                }
            else if (parser->status == RPN_START)
                {
                parser->status = RPN_END;
                parser->controllerNumberLSB = value;
                }
            }

    // Data Entry MSB for NRPN and RPN
        else 
            {
            uint16_t controllerNumber =  (((uint16_t) parser->controllerNumberMSB) << 7) | parser->controllerNumberLSB ;
            if (number == 6)
                {
                if (parser->status == NRPN_END || parser->status == NRPN_MSB || parser->status == NRPN_INCREMENT_DECREMENT)
                    {
                    parser->controllerValueMSB = value;
                    handleNRPN(channel, controllerNumber, ((uint16_t)value) << 7, VALUE_MSB_ONLY);
                    parser->status = NRPN_MSB;
                    }
                else if (parser->status == RPN_END || parser->status == RPN_MSB || parser->status == RPN_INCREMENT_DECREMENT)
                    {
                    parser->controllerValueMSB = value;
                    handleRPN(channel, controllerNumber, ((uint16_t)value) << 7, VALUE_MSB_ONLY);
                    parser->status = RPN_MSB;
                    }
                else parser->status = INVALID;
                }
                
            // Data Entry LSB for RPN, NRPN
            else if (number == 38)
                {
                if (parser->status == NRPN_MSB)
                    {
                    handleNRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
                    }
                else if (parser->status == RPN_MSB)
                    {
                    handleRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
                    }
                else parser->status = INVALID;
                }
                
            // Data Increment for RPN, NRPN
            else if (number == 96)
                {
                if (parser->status == NRPN_END || parser->status == NRPN_MSB || parser->status == NRPN_INCREMENT_DECREMENT)
                    {
                    handleNRPN(channel, controllerNumber , (value ? value : 1), INCREMENT);
                    parser->status = NRPN_INCREMENT_DECREMENT;
                    }
                else if (parser->status == RPN_END || parser->status == RPN_MSB || parser->status == RPN_INCREMENT_DECREMENT)
                    {
                    handleRPN(channel, controllerNumber, (value ? value : 1), INCREMENT);
                    parser->status = RPN_INCREMENT_DECREMENT;
                    }
                else parser->status = INVALID;
                }

            // Data Decrement for RPN, NRPN
            else if (number == 97)
                {
                if (parser->status == NRPN_END || parser->status == NRPN_MSB || parser->status == NRPN_INCREMENT_DECREMENT)
                    {
                    handleNRPN(channel, controllerNumber, (value ? value : 1), DECREMENT);
                    parser->status = NRPN_INCREMENT_DECREMENT;
                    }
                else if (parser->status == RPN_END || parser->status == RPN_MSB || parser->status == RPN_INCREMENT_DECREMENT)
                    {
                    handleRPN(channel, controllerNumber, (value ? value : 1), DECREMENT);
                    parser->status = RPN_INCREMENT_DECREMENT;
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
        if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
            {
            parse(&midiInParser, channel, number, value);

#ifdef INCLUDE_SPLIT
            // One exception: if we're doing keyboard splitting, we want to route control changes to the right place
            if (application == STATE_SPLIT && local.split.playing && (channel == options.channelIn || options.channelIn == CHANNEL_OMNI))
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
#ifdef INCLUDE_THRU
                if (state == STATE_THRU_PLAY)
                    {
                    // only pass through if the data's NOT coming in the default channel
                    if ((channel != options.channelIn) && (options.channelIn != CHANNEL_OMNI))
                        // Note this does NOT include the merge channel
                        {
                        MIDI.sendControlChange(number, value, channel);  // generally pass through control changes
                        TOGGLE_OUT_LED();
                        }
                    }
                else
#endif
                    { }
            }
        else 
            {
#ifdef INCLUDE_THRU
            if (state != STATE_THRU_PLAY || !options.thruBlockOtherChannels)
#endif
                {
                MIDI.sendControlChange(number, value, channel);
                TOGGLE_OUT_LED();
                }
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

#ifdef INCLUDE_VOLTAGE
    setSecondaryVoltage(pressure);
#endif
    }
  
void handlePitchBend(byte channel, int bend)
    {
    uint8_t isChannelIn = updateMIDI(channel, MIDI_PITCH_BEND, 1, (uint16_t) bend + 8192);
    if (!bypass) 
        {
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
/// or changing the volume, or transposing, or toggling the MIDI out LED,
/// or sending out to Control Voltage.


/// Sends out pressure, transposing as appropriate
void sendPolyPressure(uint8_t note, uint8_t pressure, uint8_t channel)
    {
    if (bypass) return;
  
#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    MIDI.sendPolyPressure((uint8_t) n, pressure, channel);
#else
    MIDI.sendPolyPressure(note, pressure, channel);
#endif
#ifdef INCLUDE_VOLTAGE
    setSecondaryVoltage(pressure);
#endif
    TOGGLE_OUT_LED();
    }

/// Sets voltage to 0 on both DACs, which (I presume) means "Note off"
/// Perhaps this should be revisited.  This function is a helper function
/// for the functions below.
#ifdef INCLUDE_VOLTAGE
void turnOffVoltage()
    {
    setPrimaryVoltage(0, 0);
    setSecondaryVoltage(0);
    }
#endif
        

/// Sends a note on, transposing as appropriate, and adjusting
/// the velocity as appropriate.  Also sends out CV.
void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    if (bypass) return;
  
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

#ifdef INCLUDE_VOLTAGE
    if (channel == options.channelOut)
        {
        setPrimaryVoltage(note, 1);
        setSecondaryVoltage(velocity);
        }  
#endif
    TOGGLE_OUT_LED();
    }


/// Sends a note off, transposing as appropriate, and turning off the voltage
void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    if (bypass) return;

#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    MIDI.sendNoteOff((uint8_t) n, velocity, channel);
#else
    MIDI.sendNoteOff(note, velocity, channel);
#endif
    // dont' toggle the LED because if we're going really fast it toggles
    // the LED ON and OFF for a noteoff/noteon pair and you can't see the LED

#ifdef INCLUDE_VOLTAGE
    if (channel == options.channelOut)
        {
        turnOffVoltage();
        }
#endif
    }


         
/// Sends an all notes off on ALL channels, regardless of whether bypass is turned on or not.
/// Also turns off voltage.
void sendAllNotesOffDisregardBypass()
    {
    for(uint8_t i = LOWEST_MIDI_CHANNEL; i <= HIGHEST_MIDI_CHANNEL; i++)
        {
        MIDI.sendControlChange(123, 0, i);
        }
#ifdef INCLUDE_VOLTAGE
    turnOffVoltage();
#endif
    }
        

/// Sends an all notes off on ALL channels, but only if bypass is off.
/// Also turns off voltage.
void sendAllNotesOff()
    {
    if (!bypass) 
        sendAllNotesOffDisregardBypass();
    else
        {
#ifdef INCLUDE_VOLTAGE
        turnOffVoltage();
#endif
        }
    }



