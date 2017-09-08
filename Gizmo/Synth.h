////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License



// Synth list

#ifndef __SYNTH_H__
#define __SYNTH_H__

#include "synth/WaldorfBlofeld.h"
#include "synth/KawaiK4.h"
#include "synth/OberheimMatrix1000.h"
#include "synth/KorgMicrosampler.h"
#include "synth/YamahaTX81Z.h"

struct _synthLocal
    {
    // Should we pass through various MIDI data?  See item Types in MidiInterface.h,
    // 		for example, passMIDIData[MIDI_NOTE_ON]
    // Note that the following data is always passed through, you can't intercept it right now:
    // 		Time Code, Active Sensing, Sysex, Song Position, Song Select, Tune Request, System Reset
    // Note also that the following data is handled (passed through, blocked, generated) by Gizmo, 
    // and you also can't intercept it right now:
    //		Clock, Start, Continue, Stop
    uint8_t passMIDIData[25];
    
    union 
    	{
    	struct _waldorfBlofeldLocal waldorfBlofeld;
    	struct _kawaiK4Local kawaiK4;
    	struct _oberheimMatrix1000Local oberheimMatrix1000;
    	struct _korgMicrosamplerLocal korgMicrosampler;
    	struct _yamahaTX81ZLocal yamahaTX81Z;
    	} type;
    
    };

#endif

