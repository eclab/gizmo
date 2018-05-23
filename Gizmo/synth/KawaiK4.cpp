////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_KAWAI_K4


void stateSynthKawaiK4()
    {
    if (entry)
        {
        entry = false;
        }
    
	synthUpdate();
    
    if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        // unpack
        uint8_t parameter = (uint8_t)((itemNumber >> 8) & 127);
        uint8_t source = (uint8_t)(itemNumber & 64);  // biggest source is 0-60 drum key
        
        if (parameter > 88) return; // bad
        
        uint8_t sysex[10] = { 0xF0, 0x40, 0x0, 0x10, 0x0, 0x04, 0x0, 0x0, 0x0, 0xF7 };
        sysex[2] = itemChannel;
		sysex[6] = parameter;
		sysex[7] = (source << 1) | (uint8_t)((itemValue >> 7) & 1);
		sysex[8] = (uint8_t)(itemValue & 127);

        sendDelayedSysex(sysex, 10, itemValue, KAWAI_K4_COUNTDOWN);
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }
    }
                        
#endif
