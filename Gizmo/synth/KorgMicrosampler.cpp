////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "../All.h"

#ifdef INCLUDE_SYNTH_KORG_MICROSAMPLER

int count = 0;


void sendKorgMicrosamplerSysex(uint16_t parameter, uint16_t value)
    {
    uint8_t sysExArray[] = { 0x42, 0x32, 0x7F, 0x41, 0x00, 0x00, 0x00, 0x00, (value >> 7) , 0x00 };

    // we start by determining if we can divine the actual sample/pattern/fx 
    // from the parameter number, or if we have to extract it from the current
    // sample pattern
    uint16_t samplePatternFX = (parameter >> 7);             // MSB
    uint16_t number = (parameter & 127);                            // LSB

    if (number <= 9)
        {
        // BANKS
        if (number <= 7)
            {
            sysExArray[6] = number;
            }
        else if (number == 8)
            {
            sysExArray[6] = 0x10;
            sysExArray[8] = (value & 127);                  // LSB
            sysExArray[9] = ((value >> 7) & 127);   // MSB
            }
        else
            {
            sysExArray[6] = 0x12;
            }
        }
    else if (number <= 11)
        {
        if (samplePatternFX == 0) 
            samplePatternFX = local.synth.type.korgMicrosampler.patternParameter;

        // PATTERNS
        sysExArray[4] = 0x62;
                
        if (number == 10)
            {
            sysExArray[6] = samplePatternFX;
            }
        else
            {
            sysExArray[6] = samplePatternFX + 16;
            }
        }
    else if (number <= 30)
        {
        if (samplePatternFX == 0) 
            samplePatternFX = local.synth.type.korgMicrosampler.sampleParameter;

        // SAMPLES
        sysExArray[4] = samplePatternFX + 16;

        if (number <= 19)
            {
            sysExArray[6] = number - 12;
            }
        else if (number <= 22)
            {
            sysExArray[6] = number - 4;
            }
        else if (number <= 24)
            {
            sysExArray[6] = number - 2;
            }
        else if (number <= 26)
            {
            sysExArray[6] = number + 2;
            }
        else if (number == 27)
            {
            sysExArray[6] = 0x18;
            }
        else if (number == 28)
            {
            sysExArray[6] = 0x1D;
            }
        else if (number == 29)
            {
            sysExArray[6] = 0x19;
            }
        else if (number == 30)
            {
            sysExArray[6] = 0x1A;
            }
        }
    else if (number <= 34)
        {
        if (samplePatternFX == 0) 
            samplePatternFX = local.synth.type.korgMicrosampler.effectsParameter;
        else
            samplePatternFX = samplePatternFX - 1;  // make zero-based

        // EFFECTS
        sysExArray[4] = 0x50;

        if (samplePatternFX > 20)       // obviously a loose upper bound
            {
            samplePatternFX = 20;
            }
        
        if (number <= 33)               // FX TYPE, CONTROL 1, CONTROL 2
            {
            sysExArray[6] = number - 30;
            }
        else  // FX TUNED PARAMETER
            {
            sysExArray[6] = samplePatternFX + 16;
            }
        }
    else return;
    
    // If the timer was set, we have to wait.  Put the item in the queue.  This may
    // displace any previous items placed in the queue.
    if (local.synth.type.korgMicrosampler.countDown > 0)
        {
        memcpy(local.synth.type.korgMicrosampler.data, sysExArray, KORG_MICROSAMPLER_SYSEX_LENGTH);
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        MIDI.sendSysEx(KORG_MICROSAMPLER_SYSEX_LENGTH, sysExArray);
        local.synth.type.korgMicrosampler.countDown = KORG_MICROSAMPLER_COUNTDOWN;
        TOGGLE_OUT_LED();
        }
    }


void stateSynthKorgMicrosampler()
    {
    if (entry)
        {
        memset(local.synth.passMIDIData, 1, 25);
        local.synth.passMIDIData[MIDI_NRPN_14_BIT] = 0;
        local.synth.type.korgMicrosampler.lastValue = 0;
        local.synth.type.korgMicrosampler.sampleParameter = 0;
        local.synth.type.korgMicrosampler.patternParameter = 0;
        local.synth.type.korgMicrosampler.effectsParameter = 0;
        local.synth.type.korgMicrosampler.lastParameter = KORG_MICROSAMPLER_NO_PARAMETER;
        local.synth.type.korgMicrosampler.data[0] = 0;
        local.synth.type.korgMicrosampler.countDown = 0;
        entry = false;
        }
        
    // count down.  If we reach 0 and something is in the queue (data[0] != 0),
    // then send it and set the countown timer for future items.  Otherwise the countdown
    // timer stays at 0 so future items can be sent immediately.
    if (local.synth.type.korgMicrosampler.countDown > 0)
        local.synth.type.korgMicrosampler.countDown--;
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_SYNTH);
        }

    if (newItem && itemType == MIDI_NRPN_14_BIT)
        {
        local.synth.type.korgMicrosampler.lastParameter = itemNumber;
        local.synth.type.korgMicrosampler.lastValue = itemValue;
        
        if (itemNumber == KORG_MICROSAMPLER_SAMPLE_PARAMETER)
            {
            local.synth.type.korgMicrosampler.sampleParameter = (itemValue >> 7);
            }
        else if (itemNumber == KORG_MICROSAMPLER_PATTERN_PARAMETER)
            {
            local.synth.type.korgMicrosampler.patternParameter = (itemValue >> 7);
            }
        else if (itemNumber == KORG_MICROSAMPLER_FX_PARAMETER)
            {
            local.synth.type.korgMicrosampler.effectsParameter = (itemValue >> 7);
            }
        else if ((local.synth.type.korgMicrosampler.countDown == 0) && (local.synth.type.korgMicrosampler.data[0] != 0))
            {
            MIDI.sendSysEx(KORG_MICROSAMPLER_SYSEX_LENGTH, local.synth.type.korgMicrosampler.data);
            local.synth.type.korgMicrosampler.data[0] = 0;  // null it out
            local.synth.type.korgMicrosampler.countDown = KORG_MICROSAMPLER_COUNTDOWN;
            TOGGLE_OUT_LED();
            }
        else
            {
            sendKorgMicrosamplerSysex(itemNumber, itemValue);
            }
        }

    if (updateDisplay)
        {
        clearScreen();

        if (local.synth.type.korgMicrosampler.lastParameter == KORG_MICROSAMPLER_SAMPLE_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_S, 0);
            }
        else if (local.synth.type.korgMicrosampler.lastParameter == KORG_MICROSAMPLER_PATTERN_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_P, 0);
            }
        else if (local.synth.type.korgMicrosampler.lastParameter == KORG_MICROSAMPLER_FX_PARAMETER)
            {
            write3x5Glyph(led2, GLYPH_3x5_F, 0);
            write3x5Glyph(led2, GLYPH_3x5_X, 4);
            }

        if (local.synth.type.korgMicrosampler.lastParameter == 8)
            writeNumber(led, led2, local.synth.type.korgMicrosampler.lastValue);
        else
            writeShortNumber(led, (local.synth.type.korgMicrosampler.lastValue >> 7), false);

        if (local.synth.type.korgMicrosampler.lastParameter == KORG_MICROSAMPLER_NO_PARAMETER)
            {
            clearScreen();  // again
            write3x5Glyphs(GLYPH_OFF);
            }
        }
    }
                        
#endif
