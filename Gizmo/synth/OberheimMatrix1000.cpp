////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000

void stateSynthOberheimMatrix1000()
    {
    if (entry)
        {
        local.synth.type.oberheimMatrix1000.modnum = 0;
        local.synth.passMIDIData[MIDI_NRPN_14_BIT] = false;
        local.synth.passMIDIData[MIDI_CC_7_BIT] = false;
        setParse14BitCC(false);  // this is the default anyway
        entry = false;
        }
            
    synthUpdate();
        
    // First convert 6 and 38 and 98, which are illegal
    // We don't bother with 99, which isn't a real Matrix 1000 parameter
    if (newItem && (itemType == MIDI_CC_7_BIT))
        {
        if (itemNumber == OBERHEIM_MATRIX_PARAM_6)
            itemNumber = 6;
        else if (itemNumber == OBERHEIM_MATRIX_PARAM_38)
            itemNumber = 38;
        else if (itemNumber == OBERHEIM_MATRIX_PARAM_98)
            itemNumber = 98;
        }

    // Now we do the translation
    if (newItem && ((itemType == MIDI_NRPN_14_BIT) || (itemType == MIDI_CC_7_BIT)))
        {
        if (itemNumber == OBERHEIM_MATRIX_MODNUM)
            {
            local.synth.type.oberheimMatrix1000.modnum = (uint8_t) (itemValue >> 7);
            local.synth.valueDisplay = local.synth.type.oberheimMatrix1000.modnum;
            }
        else if (itemNumber >= OBERHEIM_MATRIX_SOURCE &&
            itemNumber <= OBERHEIM_MATRIX_VALUE)
            {
            if (itemNumber == OBERHEIM_MATRIX_SOURCE)
                local.synth.type.oberheimMatrix1000.source = (uint8_t) (itemValue >> 7);
            else if (itemNumber == OBERHEIM_MATRIX_DESTINATION)
                local.synth.type.oberheimMatrix1000.destination = (uint8_t) (itemValue >> 7);
            if (itemNumber == OBERHEIM_MATRIX_VALUE)
                local.synth.type.oberheimMatrix1000.value = ((int8_t) (itemValue >> 7)) - 64;
                        
            uint8_t sysex[9] = { 0xF0, 0x10, 0x06, 0x0B, 0x0, 0x0, 0x0, 0x0, 0xF7 };
            sysex[4] = (uint8_t)local.synth.type.oberheimMatrix1000.modnum;
            sysex[5] = (uint8_t)local.synth.type.oberheimMatrix1000.source;
            sysex[6] = (uint8_t)local.synth.type.oberheimMatrix1000.value;
            sysex[7] = (uint8_t)local.synth.type.oberheimMatrix1000.destination;
            sendDelayedSysex(sysex, 7, local.synth.type.oberheimMatrix1000.modnum, OBERHEIM_MATRIX_1000_COUNTDOWN);
            }
        else 
            {
            sendDelayedNRPN(itemNumber, itemValue, itemChannel, OBERHEIM_MATRIX_1000_COUNTDOWN);
            }
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }
    }

#endif
