////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_KAWAI_K5

void stateSynthKawaiK5()
    {
    if (entry)
        {
        entry = false;
        }
    
    	synthUpdate();
    

    if (newItem && (itemType == MIDI_NRPN_14_BIT)
        {
        // unpack
        uint8_t cursor = (uint8_t)(itemNumber & 255);
        uint8_t subStatus = (uint8_t)((itemNumber >> 8) & 2);
        uint8_t valueLo = (uint8)(itemValue & 15);
        uint8_t valueHi = (uint8)((itemValue >> 4) & 15);
        
        if (subStatus == 3) subStatus = 2;
        
        uint8_t sysex[11] = { 0xF0, 0x40, 0x0, 0x10, 0x0, 0x02, 0x0, 0x0, 0x0, 0x0, 0xF7 };
        
        sysex[2] = itemChannel;
        sysex[6] = (subStatus << 1) | ((cursor >> 7) & 1);
        sysex[7] = (cursor & 127);
        sysex[8] = valueHi;
        sysex[9] = valueLo;
        
        sendDelayedSysex(sysex, 11, itemValue, NO_COUNTDOWN);
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }
    }

#endif
