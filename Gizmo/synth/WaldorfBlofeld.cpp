////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD

void stateSynthWaldorfBlofeld()
    {
    if (entry)
        {
        entry = false;
        }
    
	synthUpdate();
    
    if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        // unpack
        uint8_t indexHi = (uint8_t)((itemNumber >> 8) & 127);
        uint8_t indexLo = (uint8_t)(itemNumber & 127);
        uint8_t value = (uint8_t)(itemNumber & 127);
        
        uint8_t sysex[10] = { 0xF0, 0x3E, 0x13, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0xF7 };
		sysex[6] = indexHi;
		sysex[7] = indexLo;
		sysex[8] = value;

        sendDelayedSysex(sysex, 10, value, WALDORF_BLOFELD_COUNTDOWN);
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }
    }


#endif
