////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_KAWAI_K4


void stateSynthKawaiK4()
    {
    if (entry)
        {

// even though the data is going to be 7-bit raw (because I've set it so below),
// do a stupidity on my part, when handleControlChange() calls updateMIDI(..),
// it will do so with MIDI_CC_14_BIT.  And so I need to turn off THAT passthrough.
// This needs to be cleaned up.

        local.synth.passMIDIData[MIDI_CC_14_BIT] = false;
        setParseRawCC(true);
        entry = false;
        }
    
    synthUpdate();
    
    if (newItem && (itemType == MIDI_CC_7_BIT))
        {
        if (itemNumber == KAWAI_K4_NUMBER_PARAM)
            {
            local.synth.type.kawaiK4.number = itemValue;
            local.synth.parameterDisplay = 128 + GLYPH_3x5_S;
            local.synth.valueDisplay = itemValue;
            }
        else if (itemNumber == KAWAI_K4_HIGH_PARAM)
            {
            local.synth.type.kawaiK4.high = (itemValue > 0 ? 1 : 0);
            local.synth.parameterDisplay = 128 + GLYPH_3x5_PLUS;
            local.synth.valueDisplay = (itemValue > 0 ? 128 : 0);
            }
        else
            {
            if (itemNumber > 88) return;
            uint8_t sysex[10] = { 0xF0, 0x40, 0x0, 0x10, 0x0, 0x04, 0x0, 0x0, 0x0, 0xF7 };
            sysex[2] = itemChannel - 1;
            sysex[6] = itemNumber;
            if (itemNumber <= 69)
                {
                sysex[7] = ((local.synth.type.kawaiK4.number & 4) << 1) | (local.synth.type.kawaiK4.high);
                }
            else if (itemNumber <= 81)      // drum
                {
                sysex[7] = ((local.synth.type.kawaiK4.number & 64) << 1) | (local.synth.type.kawaiK4.high);
                }
            else    // effect
                {
                sysex[7] = ((local.synth.type.kawaiK4.number & 8) << 1) | (local.synth.type.kawaiK4.high);
                }
            sysex[8] = itemValue;
                        
            local.synth.parameterDisplay = DISPLAY_ONLY_VALUE;
            local.synth.valueDisplay = (local.synth.type.kawaiK4.high == 1 ? (itemValue + (uint16_t)128) : itemValue);
                        
            sendDelayedSysex(sysex, 10, itemValue, KAWAI_K4_COUNTDOWN);
            }
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }
    }
                        
#endif
