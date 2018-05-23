////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000

void stateSynthOberheimMatrix1000()
    {
    if (entry)
        {
        setParseRawCC(true);
        entry = false;
        }
    
	synthUpdate();
    
    if (newItem && (itemType == MIDI_CC_7_BIT))
        {
        sendDelayedNRPN(itemNumber < 64 ? itemNumber - 10 : itemNumber - 20,
        				itemValue << 7,
        				itemChannel,
        				OBERHEIM_MATRIX_1000_COUNTDOWN);
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        setParseRawCC(false);
        goUpState(STATE_SYNTH);
        }
    }

#endif
