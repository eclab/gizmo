#ifndef __KAWAI_K4_H__
#define __KAWAI_K4_H__

/*** NOTES ON THE KAWAI K4

The K4 has an extensive MIDI facility but it lacks response to NRPN values, making
it very difficult to control in real time via MIDI.  Instead it has real-time response
to SysEx values.  This would be okay (the Novation controllers for example can send SysEx)
except that the SysEx commands have two unusual features.  First, they indicate both a
SOURCE (one of four), a DRUM KEY (one of 61), or an EFFECT (one of eight), and *then*
indicate the PARAMETER and its VALUE.  So you have THREE items to specify.  Second, 
one particular parameter (P17) is actually a conglomeration of four source mute parameters
and a vibrato shape.  All this results in making the K4 essentially impossible to set up
a control keyboard for.

The code here fixes this, kind of.  It allows you to FIRST send an NRPN message (100) to set up
a SOURCE (0...3), or a DRUM KEY (101) with values (0...60), or an EFFECT (102) with
values (0...7).   Then you may send NRPN messages for all 89 parameters (0...88) and they
will affect the appropriate source, drum key, or effect.  

Furthermore, you may set the Source 1 MUTE (103), Source 2 MUTE (104), Source 3 MUTE (105), or
the Source 4 MUTE (106) -- all values of 0...1 -- or you may set the VIBRATO SHAPE (107) to values
(0...3).  When you do any one of these, all the other four values will be sent together as a
SysEx command.  Initially default values will be used, but as you tweak these, they will be
remembered and their previous values will be sent next time.  It's not optimal but about as
good as we can do short of making SysEx bulk dump requests.

All NRPN parameters should be 14 bit (MSB + LSB) and should be sent through the MIDI IN channel, 
not the MIDI CONTROL channel.

***/


/// Length of a K4 SysEx packet
#define KAWAI_K4_SYSEX_LENGTH 		(8)

/// ADDITIONAL NRPN PARAMETERS (beyond 0...88)
#define KAWAI_K4_SOURCE_PARAMETER	100
#define KAWAI_K4_DRUM_PARAMETER		101
#define KAWAI_K4_EFFECT_PARAMETER	102
#define KAWAI_K4_PARAMETER_S1_MUTE	103
#define KAWAI_K4_PARAMETER_S2_MUTE	104
#define KAWAI_K4_PARAMETER_S3_MUTE	105
#define KAWAI_K4_PARAMETER_S4_MUTE	106
#define KAWAI_K4_PARAMETER_VIBRATO_SHAPE	107

#define KAWAI_K4_NO_PARAMETER		255

#define KAWAI_K4_COUNTDOWN			(32)

/// Our local.kawai struct, storing the current source, drum key, and effect settings,
/// plus the current default values for parameter 17.
struct _kawaiK4Local
	{
	uint8_t source;
	uint8_t drum;
	uint8_t effect;
	// parameter 17 is special and annoying
	uint8_t p17;
	uint8_t lastParameter;
	uint8_t lastValue;
	uint8_t countDown;
	char data[KAWAI_K4_SYSEX_LENGTH];
	};


/// State function for STATE_UTILITY_KAWAI_K4
void stateSynthKawaiK4();

#endif