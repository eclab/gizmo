////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD

/*
  void debugArray(uint8_t* data, uint8_t len)
  {
  for(int i = 0; i < len; i++)
  debug(data[i]);
  }
*/

void sendWaldorfBlofeldSysex(uint8_t id, uint16_t parameter, uint8_t value)
    {
    // See Section 2.13 of blofeld_sysex_v1_04.txt
    char data[] = { 
        0x3E, 
        0x13, 
        id,
        0x20,
        0x00,   // Sound mode buffer
        (parameter >> 7),               // high byte first
        parameter & 127,       // low byte next
        (value > 127) ? 127 : value
        // checksum is omitted
        };
    
    // If the timer was set, we have to wait.  Put the item in the queue.  This may
    // displace any previous items placed in the queue.
    if (local.synth.type.waldorfBlofeld.countDown > 0)
        {
        memcpy(local.synth.type.waldorfBlofeld.data, data, WALDORF_BLOFELD_SYSEX_LENGTH);
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        MIDI.sendSysEx(WALDORF_BLOFELD_SYSEX_LENGTH, data);
        local.synth.type.waldorfBlofeld.countDown = WALDORF_BLOFELD_COUNTDOWN;
        TOGGLE_OUT_LED();
        }
    }


void stateSynthWaldorfBlofeld()
    {
    if (entry)
        {
        memset(local.synth.passMIDIData, 1, 25);
        local.synth.passMIDIData[MIDI_NRPN_14_BIT] = 0;
        local.synth.type.waldorfBlofeld.id = 0;
        local.synth.type.waldorfBlofeld.data[0] = 0;
        local.synth.type.waldorfBlofeld.countDown = 0;
        local.synth.type.waldorfBlofeld.lastParameter = WALDORF_BLOFELD_BAD_PARAMETER;
        entry = false;
        }

    // count down.  If we reach 0 and something is in the queue (data[0] != 0),
    // then send it and set the countown timer for future items.  Otherwise the countdown
    // timer stays at 0 so future items can be sent immediately.
    if (local.synth.type.waldorfBlofeld.countDown > 0)
        local.synth.type.waldorfBlofeld.countDown--;

    if ((local.synth.type.waldorfBlofeld.countDown == 0) && (local.synth.type.waldorfBlofeld.data[0] != 0))
        {
        MIDI.sendSysEx(WALDORF_BLOFELD_SYSEX_LENGTH, local.synth.type.waldorfBlofeld.data);
        local.synth.type.waldorfBlofeld.data[0] = 0;  // null it out
        local.synth.type.waldorfBlofeld.countDown = WALDORF_BLOFELD_COUNTDOWN;
        TOGGLE_OUT_LED();
        }
    
    if (potUpdated[LEFT_POT])
        {
        local.synth.type.waldorfBlofeld.id = pot[LEFT_POT] >> 6; // / 64
        }
    
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }

    if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        uint8_t val = (uint8_t) (itemValue >> 7);
        local.synth.type.waldorfBlofeld.lastParameter = (uint16_t) itemNumber;
        local.synth.type.waldorfBlofeld.lastValue = val;
        
        if (itemNumber == WALDORF_BLOFELD_ID_PARAMETER)
            {
            local.synth.type.waldorfBlofeld.id = val;
            }
        else if (itemNumber <= WALDORF_BLOFELD_HIGH_PARAMETER)
            {       
            sendWaldorfBlofeldSysex(local.synth.type.waldorfBlofeld.id, itemNumber, val);
            }
        else
            {
            local.synth.type.waldorfBlofeld.lastParameter = WALDORF_BLOFELD_BAD_PARAMETER;
            }
        }
        
    if (updateDisplay)
        {
        clearScreen();

        if (local.synth.type.waldorfBlofeld.lastParameter == WALDORF_BLOFELD_BAD_PARAMETER)
            {
            write3x5Glyphs(GLYPH_OFF);
            }
        else if (local.synth.type.waldorfBlofeld.lastParameter == WALDORF_BLOFELD_ID_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_I, 0);  
            write3x5Glyph(led2, GLYPH_3x5_D, 4); 
            writeShortNumber(led, local.synth.type.waldorfBlofeld.lastValue, false);
            }
        else
            {
            writeNumber(led, led2, local.synth.type.waldorfBlofeld.lastValue);
            }
        }
    }

#endif
