////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License



// Synth list

#ifndef __SYNTH_H__
#define __SYNTH_H__


#include "synth/WaldorfBlofeld.h"
#include "synth/KawaiK4.h"
#include "synth/KawaiK5.h"
#include "synth/OberheimMatrix1000.h"
#include "synth/KorgMicrosampler.h"
#include "synth/YamahaTX81Z.h"

/** 

NOTES ON COUNTDOWNS

The countdown facility is how many ticks we should wait before we're
allowed to SEND another parameter.  A tick is exactly the amount of time
to send a single MIDI byte.  This Sis 1/3125 seconds.  Basically 1/3 of a ms.

You might need to slow things down for two reasons:

1. The synth can't take full-speed transmission.  

2. The incoming data (perhaps it's CC) is shorter than the outgoing data (perhaps NRPN or Sysex)
   and so constant sending will block.

*/


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
	
	// countdown facility.  See sendDelayedNRPN
#define NO_COUNTDOWN (0)
	uint8_t countDown;
	uint16_t parameter;		// also used for sysex length
	uint16_t value;			// also used to display a value for sysex
	uint8_t channel;
#define TYPE_SYSEX (255)
#define TYPE_NRPN (CONTROL_TYPE_NRPN)
#define TYPE_CC (CONTROL_TYPE_CC)
	uint8_t datatype;

	// display facility
	int16_t valueDisplay;
#define DISPLAY_NOTHING (253)
#define DISPLAY_ONLY_VALUE (254)
	uint8_t parameterDisplay;

#define MAX_SYNTH_SYSEX_OUTPUT (32)
	uint8_t sysex[MAX_SYNTH_SYSEX_OUTPUT];  // hopefully that's enough?  
	
    union 
    	{
    	struct _waldorfBlofeldLocal waldorfBlofeld;
    	struct _kawaiK4Local kawaiK4;
    	struct _oberheimMatrix1000Local oberheimMatrix1000;
    	struct _korgMicrosamplerLocal korgMicrosampler;
    	struct _yamahaTX81ZLocal yamahaTX81Z;
    	struct _kawaiK5Local kawaiK5;
    	} type;
    
    };


void synthUpdate();
void sendDelayedNRPN(uint16_t parameter, uint16_t value, uint8_t channel, uint8_t countdown);
void sendDelayedCC(uint8_t parameter, uint8_t value, uint8_t channel, uint8_t countdown);
void sendDelayedSysex(uint8_t* sysex, uint8_t length, int16_t displayValue, uint8_t countdown);


#endif

