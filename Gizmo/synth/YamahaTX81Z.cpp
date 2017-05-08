////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z

void sendYamahaTX81ZSysex(uint16_t parameter, uint16_t value)
    {
    // the real length of this array may vary, the length is stored in local.synth.type.yamahaTX81Z.dataLength
    char data[] = { 
        0x43,                   // Yamaha 
        options.channelOut, 
        0x0,
        0x0,
        0x0,
        0x0,
        0x0
        };
    
    if (parameter < 200)                                // VCED
        {
        data[2] = 0x12;
        data[3] = (parameter >> 7);
        data[4] = (value >> 7);
        local.synth.type.yamahaTX81Z.dataLength = 5;
        }
        
    //// WARNING --- THERE IS A BUG HERE
    //// The Manual (p. 67 states that ACED, PCED, and Remote Switch
    //// all have the same group and subgroup number.  This is impossible  
    //// because PCED's parameter numbers overlap with ACED and Remote Switch.
    //// I need to determine what the correct values are. 
    
    else if (parameter < 400)                   // ACED
        {
        data[2] = 0x13;         // bug
        data[3] = (parameter >> 7);
        data[4] = (value >> 7);
        local.synth.type.yamahaTX81Z.dataLength = 5;
        }
    else if (parameter < 600)                   // PCED
        {
        data[2] = 0x13; // bug
        data[3] = (parameter >> 7);
        data[4] = (value >> 7);
        local.synth.type.yamahaTX81Z.dataLength = 5;
        }
    else if (parameter < 800)                   // Remote Switch
        {
        data[2] = 0x13; //  bug
        data[3] = (parameter >> 7);
        data[4] = ((value >> 7) ? 0x7F : 0x0);
        local.synth.type.yamahaTX81Z.dataLength = 5;
        }
    else if (parameter < 1000)                  // Micro Tune Octave
        {
        data[2] = 0x10;
        data[3] = 0x7D;  // 125
        data[4] = (parameter >> 7);
        data[5] = (value >> 7);
        data[6] = (value & 127);
        local.synth.type.yamahaTX81Z.dataLength = 7;
        }
    else if (parameter < 1200)                  // Micro Tune Full
        {
        data[2] = 0x10;
        data[3] = 0x7E;  // 126
        data[4] = (parameter >> 7);
        data[5] = (value >> 7);
        data[6] = (value & 127);
        local.synth.type.yamahaTX81Z.dataLength = 7;
        }
    else if (parameter < 1400)          // Program Change Table
        {
        data[2] = 0x10;
        data[3] = 0x7F;  // 127
        data[4] = (parameter >> 7);
        data[5] = (value >> 7);
        data[6] = (value & 127);
        local.synth.type.yamahaTX81Z.dataLength = 7;
        }
    else if (parameter < 1600)          // System Data
        {
        data[2] = 0x10;
        data[3] = 0x7B;  // 123
        data[4] = (parameter >> 7);
        data[5] = (value >> 7);
        local.synth.type.yamahaTX81Z.dataLength = 6;
        }
    else if (parameter < 1800)
        {
        data[2] = 0x10;
        data[3] = 0x7C;  // 124
        data[4] = (parameter >> 7);
        data[5] = (value >> 7);
        local.synth.type.yamahaTX81Z.dataLength = 6;
        }
    else
        {
        // bad data
        return;
        }
    
    // If the timer was set, we have to wait.  Put the item in the queue.  This may
    // displace any previous items placed in the queue.
    if (local.synth.type.yamahaTX81Z.countDown > 0)
        {
        memcpy(local.synth.type.yamahaTX81Z.data, data, YAMAHA_TX81Z_SYSEX_LENGTH);
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        MIDI.sendSysEx(local.synth.type.yamahaTX81Z.dataLength, data);
        local.synth.type.yamahaTX81Z.countDown = YAMAHA_TX81Z_COUNTDOWN;
        TOGGLE_OUT_LED();
        }
    }


void stateSynthYamahaTX81Z()
    {
    if (entry)
        {
        memset(local.synth.passMIDIData, 1, 25);
        local.synth.passMIDIData[MIDI_NRPN_14_BIT] = 0;
        local.synth.type.yamahaTX81Z.data[0] = 0;
        local.synth.type.yamahaTX81Z.countDown = 0;
        local.synth.type.yamahaTX81Z.dataLength = 0;
        local.synth.type.yamahaTX81Z.lastParameter = YAMAHA_TX81Z_BAD_PARAMETER;
        entry = false;
        }

    // count down.  If we reach 0 and something is in the queue (data[0] != 0),
    // then send it and set the countown timer for future items.  Otherwise the countdown
    // timer stays at 0 so future items can be sent immediately.
    if (local.synth.type.yamahaTX81Z.countDown > 0)
        local.synth.type.yamahaTX81Z.countDown--;

    if ((local.synth.type.yamahaTX81Z.countDown == 0) && (local.synth.type.yamahaTX81Z.data[0] != 0))
        {
        MIDI.sendSysEx(local.synth.type.yamahaTX81Z.dataLength, local.synth.type.yamahaTX81Z.data);
        local.synth.type.yamahaTX81Z.data[0] = 0;  // null it out
        local.synth.type.yamahaTX81Z.countDown = YAMAHA_TX81Z_COUNTDOWN;
        TOGGLE_OUT_LED();
        }
    
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }

    if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        local.synth.type.yamahaTX81Z.lastParameter = itemNumber;
        local.synth.type.yamahaTX81Z.lastValue = itemValue;
        sendYamahaTX81ZSysex(itemNumber, itemValue);
        }
        
    if (updateDisplay)
        {
        clearScreen();

        if (local.synth.type.yamahaTX81Z.lastParameter == YAMAHA_TX81Z_BAD_PARAMETER)
            {
            clearScreen();  // again
            write3x5Glyphs(GLYPH_OFF);
            }
        else 
            {
            writeNumber(led, led2, local.synth.type.yamahaTX81Z.lastValue);
            }
        }
    }

#endif
