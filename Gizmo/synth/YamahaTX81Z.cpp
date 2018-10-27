////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z

#define VCED_GROUP      18
#define ACED_GROUP      19

void stateSynthYamahaTX81Z()
    {
    if (entry)
        {
        entry = false;
        }
    
    synthUpdate();
    
    if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        // unpack
        uint8_t parameter = (uint8_t)(itemNumber & 127);
        uint8_t value = (uint8_t)((itemValue >> 7) & 127);
        
        if (parameter > 117) return; // bad
        
        uint8_t sysex[7] = { 0xF0, 0x43, 0x0, 0x0, 0x0, 0x0, 0xF7 };
        sysex[2] = itemChannel + 16;
        sysex[5] = value;

        if (parameter <= 93)  // VCED
            {
            sysex[3] = VCED_GROUP;
            sysex[4] = parameter;
            }
        else
            {
            sysex[3] = ACED_GROUP;
            sysex[4] = parameter - 94;
            } 
        
        sendDelayedSysex(sysex, 7, itemValue, YAMAHA_TX81Z_COUNTDOWN);
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }
    }

#endif
