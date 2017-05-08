////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000

void sendOberheimMatrix1000NRPN(uint8_t parameter, uint8_t value, uint8_t channel)
    {
    if (parameter <= 10 || parameter >= 120 || (parameter >= 64 && parameter <= 73))  // we need 64 in particular
        sendControllerCommand(CONTROL_TYPE_CC, parameter, value << 7, channel);
    else if (local.synth.type.waldorfBlofeld.countDown > 0)
        {
        local.synth.type.oberheimMatrix1000.storedParameter = parameter;
        local.synth.type.oberheimMatrix1000.storedValue = value;
        local.synth.type.oberheimMatrix1000.storedChannel = channel;
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        if (parameter < 64)
            sendControllerCommand(CONTROL_TYPE_NRPN, parameter - 10, value << 7, channel);
        else
            sendControllerCommand(CONTROL_TYPE_NRPN, parameter - 20, value << 7, channel);
        local.synth.type.oberheimMatrix1000.countDown = OBERHEIM_MATRIX_1000_COUNTDOWN;
        }
    }

void stateSynthOberheimMatrix1000()
    {
    if (entry)
        {
        setParseRawCC(true);
        local.synth.type.oberheimMatrix1000.lastParameter = OBERHEIM_MATRIX_1000_NO_PARAMETER;
        local.synth.type.oberheimMatrix1000.storedParameter = OBERHEIM_MATRIX_1000_NO_PARAMETER;
        entry = false;
        }
    
    // count down.  If we reach 0 and something is in the queue (data[0] != 0),
    // then send it and set the countown timer for future items.  Otherwise the countdown
    // timer stays at 0 so future items can be sent immediately.
    if (local.synth.type.oberheimMatrix1000.countDown > 0)
        local.synth.type.oberheimMatrix1000.countDown--;

    if ((local.synth.type.oberheimMatrix1000.countDown == 0) && (local.synth.type.oberheimMatrix1000.storedParameter != OBERHEIM_MATRIX_1000_NO_PARAMETER))
        {
        uint8_t parameter = local.synth.type.oberheimMatrix1000.storedParameter;
        uint8_t value = local.synth.type.oberheimMatrix1000.storedValue;
        uint8_t channel = local.synth.type.oberheimMatrix1000.storedChannel;
                
        if (parameter < 64)
            sendControllerCommand(CONTROL_TYPE_NRPN, parameter - 10, value << 7, channel);
        else
            sendControllerCommand(CONTROL_TYPE_NRPN, parameter - 20, value << 7, channel);
        
        local.synth.type.oberheimMatrix1000.storedParameter = OBERHEIM_MATRIX_1000_NO_PARAMETER;
        local.synth.type.oberheimMatrix1000.countDown = OBERHEIM_MATRIX_1000_COUNTDOWN;
        }
    

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        setParseRawCC(false);
        goUpState(STATE_SYNTH);
        }

    if (newItem && (itemType == MIDI_CC_7_BIT))
        {
        local.synth.type.oberheimMatrix1000.lastParameter = itemNumber;
        local.synth.type.oberheimMatrix1000.lastValue = itemValue;
        
        sendOberheimMatrix1000NRPN(itemNumber, itemValue, itemChannel);
        }
        
    if (updateDisplay)
        {
        clearScreen();

        if (local.synth.type.oberheimMatrix1000.lastParameter != OBERHEIM_MATRIX_1000_NO_PARAMETER)
            {
            writeShortNumber(led, local.synth.type.oberheimMatrix1000.lastValue, true);
            writeShortNumber(led2, local.synth.type.oberheimMatrix1000.lastParameter, false);
            }
        else
            {
            write3x5Glyphs(GLYPH_OFF);
            }
        }
    }

#endif
