////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_KAWAI_K4


/// Send a K4 SysEx command corresponding to a given parameter number and its value.
/// If DRAW is 1, then we also draw on-screen the parameter number (at left) and the
/// SOURCE, DRUM, or EFFECT setting (at right).  The actual value is not drawn!
void sendKawaiK4Sysex(uint8_t number, uint8_t value)
    {
    uint8_t sysExArray[KAWAI_K4_SYSEX_LENGTH] = { 0x40, options.channelOut, 0x10, 0x0, 0x04, number, 0, 0 };

    if (number <= 69)
        sysExArray[6] = local.synth.type.kawaiK4.source << 1 | value >> 7;
    else if (number <= 81)
        sysExArray[6] = local.synth.type.kawaiK4.drum << 1 | value >> 7;
    else 
        sysExArray[6] = local.synth.type.kawaiK4.effect << 1 | value >> 7;
                
    sysExArray[7] = (uint8_t)(value & 0x127);
        
    // If the timer was set, we have to wait.  Put the item in the queue.  This may
    // displace any previous items placed in the queue.
    if (local.synth.type.kawaiK4.countDown > 0)
        {
        memcpy(local.synth.type.kawaiK4.data, sysExArray, KAWAI_K4_SYSEX_LENGTH);
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        MIDI.sendSysEx(KAWAI_K4_SYSEX_LENGTH, sysExArray);
        local.synth.type.kawaiK4.countDown = KAWAI_K4_COUNTDOWN;
        TOGGLE_OUT_LED();
        }
    }

void stateSynthKawaiK4()
    {
    if (entry)
        {
        memset(local.synth.passMIDIData, 1, 25);
        local.synth.passMIDIData[MIDI_NRPN_14_BIT] = 0;
        local.synth.type.kawaiK4.source = 0;
        local.synth.type.kawaiK4.drum = 0;
        local.synth.type.kawaiK4.effect = 0;
        local.synth.type.kawaiK4.p17 = 0;
        local.synth.type.kawaiK4.lastValue = 0;
        local.synth.type.kawaiK4.lastParameter = KAWAI_K4_NO_PARAMETER;
        local.synth.type.kawaiK4.data[0] = 0;
        local.synth.type.kawaiK4.countDown = 0;
        entry = false;
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }

    if (newItem && itemType == MIDI_NRPN_14_BIT)
        {
        local.synth.type.kawaiK4.lastParameter = (uint8_t) itemNumber;
        uint8_t val = (uint8_t)(itemValue >> 7);
        
        // count down.  If we reach 0 and something is in the queue (data[0] != 0),
        // then send it and set the countown timer for future items.  Otherwise the countdown
        // timer stays at 0 so future items can be sent immediately.
        if (local.synth.type.kawaiK4.countDown > 0)
            local.synth.type.kawaiK4.countDown--;

        if ((local.synth.type.kawaiK4.countDown == 0) && (local.synth.type.kawaiK4.data[0] != 0))
            {
            MIDI.sendSysEx(KAWAI_K4_SYSEX_LENGTH, local.synth.type.kawaiK4.data);
            local.synth.type.kawaiK4.data[0] = 0;  // null it out
            local.synth.type.kawaiK4.countDown = KAWAI_K4_COUNTDOWN;
            TOGGLE_OUT_LED();
            }
    

        if (itemNumber <= 88)  // highest Kawai message
            {
            sendKawaiK4Sysex((uint8_t)itemNumber, val);
            local.synth.type.kawaiK4.lastValue = val;
            }
        else if (itemNumber >= KAWAI_K4_PARAMETER_S1_MUTE && itemNumber <= KAWAI_K4_PARAMETER_VIBRATO_SHAPE)
            {
            switch(itemNumber)
                {
                case KAWAI_K4_PARAMETER_S1_MUTE:  // handled specially
                    local.synth.type.kawaiK4.p17 = (local.synth.type.kawaiK4.p17 & 254) | (val & 1);
                    break;
                case KAWAI_K4_PARAMETER_S2_MUTE:  // handled specially
                    local.synth.type.kawaiK4.p17 = (local.synth.type.kawaiK4.p17 & 253) | ((val & 2) << 1);                                   
                    break;
                case KAWAI_K4_PARAMETER_S3_MUTE:  // handled specially
                    local.synth.type.kawaiK4.p17 = (local.synth.type.kawaiK4.p17 & 250) | ((val & 4) << 2);
                    break;
                case KAWAI_K4_PARAMETER_S4_MUTE:  // handled specially
                    local.synth.type.kawaiK4.p17 = (local.synth.type.kawaiK4.p17 & 247) | ((val & 8) << 3);
                    break;
                case KAWAI_K4_PARAMETER_VIBRATO_SHAPE:  // handled specially
                    local.synth.type.kawaiK4.p17 = (local.synth.type.kawaiK4.p17 & 207) | ((val & 48) << 4);
                    break;
                }
                                                
            sendKawaiK4Sysex(17, local.synth.type.kawaiK4.p17);
            }
        else if (itemNumber == KAWAI_K4_SOURCE_PARAMETER)
            {
            if (val > 3) val = 0;
            local.synth.type.kawaiK4.source = val;
            }
        else if (itemNumber == KAWAI_K4_DRUM_PARAMETER)
            {
            if (val > 60) val = 0;
            local.synth.type.kawaiK4.drum = val;
            }
        else if (itemNumber == KAWAI_K4_EFFECT_PARAMETER)
            {
            if (val > 7) val = 0;
            local.synth.type.kawaiK4.effect = val;
            }
        }

    if (updateDisplay)
        {
        clearScreen();

        writeShortNumber(led, local.synth.type.kawaiK4.lastValue, true);

        if (local.synth.type.kawaiK4.lastParameter <= 88)
            {       
            writeShortNumber(led2, local.synth.type.kawaiK4.lastParameter, false);
            }
        else if (local.synth.type.kawaiK4.lastParameter == KAWAI_K4_PARAMETER_VIBRATO_SHAPE)
            {
            write3x5Glyph(led2, GLYPH_3x5_V, 0);  // write the V for vibrato shape
            }
        else if (local.synth.type.kawaiK4.lastParameter >= KAWAI_K4_PARAMETER_S1_MUTE && 
            local.synth.type.kawaiK4.lastParameter <= KAWAI_K4_PARAMETER_S4_MUTE)
            {
            writeShortNumber(led2, (itemNumber - KAWAI_K4_PARAMETER_S1_MUTE + 1), true);   // writes 1 for S1 etc.
            write3x5Glyph(led2, GLYPH_3x5_M, 0);  // write an the M for MUTE etc.
            }
        else if (local.synth.type.kawaiK4.lastParameter == KAWAI_K4_SOURCE_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_S, 0);  // write an S for SOURCE
            }
        else if (local.synth.type.kawaiK4.lastParameter == KAWAI_K4_DRUM_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_D, 0);  // write an P for PARAMETER
            }
        else if (local.synth.type.kawaiK4.lastParameter == KAWAI_K4_EFFECT_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_F, 0);
            write3x5Glyph(led2, GLYPH_3x5_X, 4);
            }
        else
            {
            clearScreen();  // again
            write3x5Glyphs(GLYPH_NONE);
            }
        }
    }
                        
#endif
