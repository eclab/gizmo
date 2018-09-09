////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"

#ifdef INCLUDE_SYNTH

#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD
#include "synth/WaldorfBlofeld.cpp"
#endif

#ifdef INCLUDE_SYNTH_KAWAI_K4
#include "synth/KawaiK4.cpp"
#endif

#ifdef INCLUDE_SYNTH_KAWAI_K5
#include "synth/KawaiK5.cpp"
#endif

#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
#include "synth/OberheimMatrix1000.cpp"
#endif

#ifdef INCLUDE_SYNTH_KORG_MICROSAMPLER
#include "synth/KorgMicrosampler.cpp"
#endif

#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z
#include "synth/YamahaTX81Z.cpp"
#endif

void synthUpdate()
	{
	if (local.synth.countDown > 0)
		{
		local.synth.countDown--;
		if (local.synth.countDown == 0)
			{
			if (local.synth.datatype == TYPE_NRPN)
				{
	        	sendControllerCommand(CONTROL_TYPE_NRPN, local.synth.parameter, local.synth.value, local.synth.channel);
				local.synth.parameterDisplay = DISPLAY_ONLY_VALUE;
				local.synth.valueDisplay = local.synth.value;
				}
			else if (local.synth.datatype == TYPE_CC)
				{
        		sendControllerCommand(CONTROL_TYPE_CC, local.synth.parameter, local.synth.value, local.synth.channel);
				local.synth.parameterDisplay = local.synth.parameter;
				local.synth.valueDisplay = local.synth.value;
				}
			else if (local.synth.datatype == TYPE_SYSEX)
				{
       			MIDI.sendSysEx(local.synth.parameter, local.synth.sysex, true);
				local.synth.parameterDisplay = DISPLAY_ONLY_VALUE;
				local.synth.valueDisplay = local.synth.value;
				}
			}
		}
	
	if (updateDisplay)
		{
		clearScreen();
	
		if (local.synth.parameterDisplay == DISPLAY_NOTHING)
			{
			write3x5Glyphs(GLYPH_OFF);
			}
		else if (local.synth.parameterDisplay == DISPLAY_ONLY_VALUE)
			{
			writeNumber(led, led2, local.synth.valueDisplay);
			}
		else if (local.synth.parameterDisplay >= 128)
			{
			write3x5Glyph(led2, local.synth.parameterDisplay - 128, 0);
			writeShortNumber(led, local.synth.valueDisplay, true);
			}
		else
			{
			writeShortNumber(led, local.synth.valueDisplay, true);
			writeShortNumber(led2, local.synth.parameterDisplay, false);
			}
		}
	}

void sendDelayedNRPN(uint16_t parameter, uint16_t value, uint8_t channel, uint8_t countdown)
    {
    if (local.synth.countDown > 0)	// overwrite
        {
        local.synth.parameter = parameter;
        local.synth.value = value;
        local.synth.channel = channel;
        local.synth.datatype = TYPE_NRPN;
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        sendControllerCommand(CONTROL_TYPE_NRPN, parameter, value, channel);
        local.synth.countDown = countdown;
		local.synth.parameterDisplay = DISPLAY_ONLY_VALUE;
		local.synth.valueDisplay = local.synth.value;
        }
    }

void sendDelayedCC(uint8_t parameter, uint8_t value, uint8_t channel, uint8_t countdown)
    {
    if (local.synth.countDown > 0)
        {
        local.synth.parameter = parameter;
        local.synth.value = value;
        local.synth.channel = channel;
        local.synth.datatype = TYPE_CC;
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        sendControllerCommand(CONTROL_TYPE_CC, parameter, value, channel);
        local.synth.countDown = countdown;
		local.synth.parameterDisplay = parameter;
		local.synth.valueDisplay = local.synth.value;
        }
    }

void sendDelayedSysex(uint8_t* sysex, uint8_t length, int16_t displayValue, uint8_t countdown)
    {
    if (local.synth.countDown > 0 && length <= MAX_SYNTH_SYSEX_OUTPUT)
        {
        memcpy(local.synth.sysex, sysex, length);
        local.synth.parameter = length;
        local.synth.value = displayValue;
        local.synth.datatype = TYPE_SYSEX;
        }
    else        // otherwise send it out immediately and set the timer for future items
        {
        MIDI.sendSysEx(length, sysex, true);
        TOGGLE_OUT_LED();
        local.synth.countDown = countdown;
		local.synth.parameterDisplay = DISPLAY_ONLY_VALUE;
		local.synth.valueDisplay = displayValue;
        }
    }



#endif
