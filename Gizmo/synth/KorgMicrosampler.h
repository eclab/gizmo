#ifndef __KORG_MICROSAMPLER_H__
#define __KORG_MICROSAMPLER_H__

/*** NOTES ON THE KORG MICROSAMPLER

The Microsampler is a hot mess when it comes to MIDI.  It responds to pitch bend but in an ugly stepped way.
Only two of its knobs send CC.  Only a few of its features can be controlled via sysex, and Korg has no 
sysex document at all -- you have to go discover it for yourself.  It also has broken MIDI clock behavior: 
if you send it a MIDI START, it will turn on its pattern sequencer whether you want that or not.  And if 
you set it to emit clock (via MIDI CLK set to "Auto" or "Internal"), then it will emit MIDI clock pulse 
messages immediately with no START, STOP, or CONTINUE, in violation of who knows how many MIDI specs.

What a disaster.

You can use Gizmo to clean up the clock by filtering it.  As to sysex: the code here will let you
map NRPN to sysex messages (to the degree the Microsampler responds to them).  In most cases you will
first need to send an NRPN message to set up a SAMPLE or PATTERN (or maybe FX PARAMETER,
then you will send one or more NRPN messages to change various features.  The current FX type can 
be set, and the effect changing FX parameters will be based on the FX type.

All NRPN parameters should be 14 bit (MSB + LSB) and should be sent through the MIDI IN channel, 
not the MIDI CONTROL channel.

***/


/// Length of a K4 SysEx packet
#define KORG_MICROSAMPLER_SYSEX_LENGTH 10

/// ADDITIONAL NRPN PARAMETERS 
#define KORG_MICROSAMPLER_SAMPLE_PARAMETER	50
#define KORG_MICROSAMPLER_PATTERN_PARAMETER	51
#define KORG_MICROSAMPLER_FX_PARAMETER		52
#define KORG_MICROSAMPLER_NO_PARAMETER		16383

#define KORG_MICROSAMPLER_COUNTDOWN			(32)


/// Our local.kawai struct, storing the current source, drum key, and effect settings,
/// plus the current default values for parameter 17.
struct _korgMicrosamplerLocal
	{
	uint8_t sampleParameter;
	uint8_t patternParameter;
	uint8_t effectsParameter;
	uint16_t lastParameter;
	uint16_t lastValue;
	char data[KORG_MICROSAMPLER_SYSEX_LENGTH];
	uint8_t countDown;
	};


/// State function for STATE_UTILITY_KORG_MICROSAMPLER
void stateSynthKorgMicrosampler();

#endif