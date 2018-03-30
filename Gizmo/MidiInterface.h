////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __MIDI_INTERFACE__
#define __MIDI_INTERFACE__

//// MIDI CHANNELS
#define CHANNEL_OFF 0
#define CHANNEL_OMNI 17                         // For INPUT
#define NUM_MIDI_CHANNELS 16
#define LOWEST_MIDI_CHANNEL 1
#define HIGHEST_MIDI_CHANNEL 16


//// MIDI CLOCK OPTIONS SETTINGS
// We have five options for handling the MIDI clock (or producing our own)
// These are the possible settings for options.clock

#define USE_MIDI_CLOCK 0         // Use external MIDI clock, and pass it through
#define CONSUME_MIDI_CLOCK 1     // Use external MIDI clock, but don't pass it through
#define IGNORE_MIDI_CLOCK 2      // Use our own internal clock, but pass any MIDI clock through
#define GENERATE_MIDI_CLOCK 3    // Use our own internal clock and emit a MIDI clock from it, rather than passing through any MIDI clock
#define BLOCK_MIDI_CLOCK 4       // Use our own internal clock.  Don't pass through or emit any MIDI clock.


// Some useful predicates
#define USING_EXTERNAL_CLOCK() (options.clock <= CONSUME_MIDI_CLOCK)	// are we using any external clock?


// These are the values that can be used in:
// options.middleButtonControlOn, options.middleButtonControlOff
// options.selectButtonControlOn, options.selectButtonControlOff
// options.leftKnobControlType
// options.rightKnobControlType

#define CONTROL_TYPE_OFF 0
#define CONTROL_TYPE_CC 1
#define CONTROL_TYPE_NRPN 2
#define CONTROL_TYPE_RPN 3
#define CONTROL_TYPE_PC 4
#ifdef INCLUDE_EXTENDED_CONTROL_SIGNALS
#define CONTROL_TYPE_PITCH_BEND 5
#define CONTROL_TYPE_AFTERTOUCH 6
#endif


#define CONTROL_VALUE_INCREMENT 128
#define CONTROL_VALUE_DECREMENT 129

// in midi_Defs.h there is an enum with various MIDI commands. 
// I have to use a few of them to pass in clock commands, but I 
// don't like the enum names ("Clock", "Stop", etc.) which are too
// simple and collide with other symbols of mine.  So here I have 
// defined some #defines to do the same thing.

#define    MIDIClock                 ((midi::MidiType) 0xF8)    ///< System Real Time - Timing Clock
#define    MIDIStart                 ((midi::MidiType) 0xFA)    ///< System Real Time - Start
#define    MIDIContinue              ((midi::MidiType) 0xFB)    ///< System Real Time - Continue
#define    MIDIStop                  ((midi::MidiType) 0xFC)    ///< System Real Time - Stop
#define    MIDIInvalidType           ((midi::MidiType) 0x00)    ///< For notifying errors
#define    MIDINoteOff               ((midi::MidiType) 0x80)    ///< Note Off
#define    MIDINoteOn                ((midi::MidiType) 0x90)    ///< Note On
#define    MIDIAfterTouchPoly        ((midi::MidiType) 0xA0)    ///< Polyphonic AfterTouch
#define    MIDIChannelControl        ((midi::MidiType) 0xB0)    ///< Control Change / Channel Mode
#define    MIDIProgramChange         ((midi::MidiType) 0xC0)    ///< Program Change
#define    MIDIAfterTouchChannel     ((midi::MidiType) 0xD0)    ///< Channel (monophonic) AfterTouch
#define    MIDIPitchBend             ((midi::MidiType) 0xE0)    ///< Pitch Bend
#define    MIDISystemExclusive       ((midi::MidiType) 0xF0)    ///< System Exclusive
#define    MIDITimeCodeQuarterFrame  ((midi::MidiType) 0xF1)    ///< System Common - MIDI Time Code Quarter Frame
#define    MIDISongPosition          ((midi::MidiType) 0xF2)    ///< System Common - Song Position Pointer
#define    MIDISongSelect            ((midi::MidiType) 0xF3)    ///< System Common - Song Select
#define    MIDITuneRequest           ((midi::MidiType) 0xF6)    ///< System Common - Tune Request
#define    MIDIActiveSensing         ((midi::MidiType) 0xFE)    ///< System Real Time - Active Sensing
#define    MIDISystemReset           ((midi::MidiType) 0xFF)    ///< System Real Time - System Reset







/// ALL ITEM TYPES
/// itemType can be any of the following

// These are grouped first -- they're the "fast MIDI" messages and have some FALL THRUs, see STATE_GAUGE
#define MIDI_CLOCK                      0
#define MIDI_TIME_CODE          1
#define MIDI_ACTIVE_SENSING     2

// These are the channel messages and must be grouped together, see the >= and <= in STATE_GAUGE
#define MIDI_NOTE_ON   3
#define MIDI_NOTE_OFF    4
#define MIDI_AFTERTOUCH 5
#define MIDI_AFTERTOUCH_POLY 6
#define MIDI_PROGRAM_CHANGE     7
#define MIDI_PITCH_BEND 8
#define MIDI_CC_7_BIT    9                // simple CC messages
#define MIDI_CC_14_BIT    10              // all other CC messages other than NRPN, RPN
#define MIDI_NRPN_14_BIT    11            // NRPN DATA messages
#define MIDI_RPN_14_BIT    12             // RPN DATA messages
#define MIDI_NRPN_INCREMENT    13
#define MIDI_RPN_INCREMENT    14
#define MIDI_NRPN_DECREMENT    15
#define MIDI_RPN_DECREMENT    16
// End Group

// Other messages
#define MIDI_SYSTEM_EXCLUSIVE   17
#define MIDI_SONG_POSITION      18
#define MIDI_SONG_SELECT        19
#define MIDI_TUNE_REQUEST       20
#define MIDI_START      21
#define MIDI_CONTINUE   22
#define MIDI_STOP       23
#define MIDI_SYSTEM_RESET       24

// Messages sent on the control channel which aren't handled by Gizmo proper.
#define MIDI_CUSTOM_CONTROLLER	32


// newItem can be any of the following
#define NO_NEW_ITEM 0
#define NEW_ITEM 1
#define WAIT_FOR_A_SEC 2        // indicates that LSB data might be coming through any minute now.  Or not.


// NEW INCOMING MIDI DATA STORAGE
extern uint8_t  newItem;				// Is there an item?  NO_NEW_ITEM (0), NEW_ITEM (1), or WAIT_FOR_A_SEC (2)
extern uint8_t  itemType;				// What kind of MIDI item is it?  one of Item Types above
extern uint16_t itemNumber;             // Note on/off/poly aftertouch use this for NOTE PITCH, otherwise it's the parameter for PC/CC/NRPN/RPN
extern uint16_t itemValue;              // Note on/off use this for NOTE VELOCITY, poly and channel aftertouch uses this for PRESSURE, otherwise it's the value for PC/CC/NRPN/RPN/PITCH BEND
extern uint8_t itemChannel;				// Channel of the incoming item.  One of 1...16



//// REMOTE CONTROL VIA NRPN or CC

// The following NRPN or CC parameters will control Gizmo.
// Buttons (including BYPASS and UNLOCK) are 1 when pressed, 0 when unpressed.
// Pots should be sent 14-bit, but only range from 0...1023 inclusive

#define NRPN_LEFT_POT_PARAMETER                 0
#define NRPN_RIGHT_POT_PARAMETER                1
#define NRPN_BACK_BUTTON_PARAMETER              2
#define NRPN_MIDDLE_BUTTON_PARAMETER            3
#define NRPN_SELECT_BUTTON_PARAMETER            4
#define NRPN_BYPASS_PARAMETER                   5
#define NRPN_UNLOCK_PARAMETER                   6
#define NRPN_START_PARAMETER                    7
#define NRPN_STOP_PARAMETER                     8       
#define NRPN_CONTINUE_PARAMETER                 9

// The non-relative Pot CC parameters and extra parameters S...Z are 14-bit (MSB+LSB).
// All others are 7-bit.

#define CC_LEFT_POT_PARAMETER                 14
#define CC_RIGHT_POT_PARAMETER                15
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_1	  16
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_2	  17	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_3	  18	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_4	  19
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_5	  20	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_6	  21	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_7	  22	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_8	  23	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_9	  24
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_10	  25	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_11   26	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_12	  27	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_13	  28	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_14	  29	
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_15	  30
#define CC_LEFT_POT_PARAMETER_EQUIVALENT_16	  31	
#define CC_BACK_BUTTON_PARAMETER              102
#define CC_MIDDLE_BUTTON_PARAMETER            103
#define CC_SELECT_BUTTON_PARAMETER            104
#define CC_BYPASS_PARAMETER                   105
#define CC_UNLOCK_PARAMETER                   106
#define CC_START_PARAMETER                    107
#define CC_STOP_PARAMETER                     108     
#define CC_CONTINUE_PARAMETER                 109
//#define CC_LEFT_POT_RELATIVE_PARAMETER        110
//#define CC_RIGHT_POT_RELATIVE_PARAMETER       111
#define CC_EXTRA_PARAMETER_A			      64
#define CC_EXTRA_PARAMETER_B			      65
#define CC_EXTRA_PARAMETER_C			      66
#define CC_EXTRA_PARAMETER_D			      67
#define CC_EXTRA_PARAMETER_E			      68
#define CC_EXTRA_PARAMETER_F			      69
#define CC_EXTRA_PARAMETER_G			      70
#define CC_EXTRA_PARAMETER_H			      71
#define CC_EXTRA_PARAMETER_I			      72
#define CC_EXTRA_PARAMETER_J			      73
#define CC_EXTRA_PARAMETER_K			      74
#define CC_EXTRA_PARAMETER_L			      75
#define CC_EXTRA_PARAMETER_M			      76
#define CC_EXTRA_PARAMETER_N			      77
#define CC_EXTRA_PARAMETER_O			      78
#define CC_EXTRA_PARAMETER_P			      79
#define CC_EXTRA_PARAMETER_Q				  80
#define CC_EXTRA_PARAMETER_R				  81
#define CC_EXTRA_PARAMETER_S				  82
#define CC_EXTRA_PARAMETER_T				  83
#define CC_EXTRA_PARAMETER_U				  84
#define CC_EXTRA_PARAMETER_V				  85
#define CC_EXTRA_PARAMETER_W				  86
#define CC_EXTRA_PARAMETER_X				  87
#define CC_EXTRA_PARAMETER_Y				  88
#define CC_EXTRA_PARAMETER_Z				  89
#define CC_EXTRA_PARAMETER_1				  90
#define CC_EXTRA_PARAMETER_2				  91
#define CC_EXTRA_PARAMETER_3				  92
#define CC_EXTRA_PARAMETER_4				  93
#define CC_EXTRA_PARAMETER_5				  94
#define CC_EXTRA_PARAMETER_6				  95
#define CC_EXTRA_PARAMETER_7				  116
#define CC_EXTRA_PARAMETER_8				  117
#define CC_EXTRA_PARAMETER_9				  118
#define CC_EXTRA_PARAMETER_10				  119



#ifdef INCLUDE_CC_LEFT_POT_PARAMETER_EQUIVALENTS
// If this is TRUE then the various left pot parameter equivalent CCs will
// do the same thing as the left pot parameter CC.  If false, they will do nothing
// (the application can still have them do something)
extern uint8_t leftPotParameterEquivalent;
#endif

//// MIDI HANDLERS

void handleStart();
void handleStop();
void handleContinue();
void handleClock();
void handleNoteOff(byte channel, byte note, byte velocity);
void handleNoteOn(byte channel, byte note, byte velocity);
void handleAfterTouchPoly(byte channel, byte note, byte pressure);
void handleGeneralControlChange(byte channel, byte number, byte value);
void handleProgramChange(byte channel, byte number);
void handleAfterTouchChannel(byte channel, byte pressure);
void handlePitchBend(byte channel, int bend);
void handleTimeCodeQuarterFrame(byte data);
void handleSystemExclusive(byte* array, unsigned size);
void handleSongPosition(unsigned beats);
void handleSongSelect(byte songnumber);
void handleTuneRequest();
void handleActiveSensing();
void handleSystemReset();


//// SENDING MIDI
void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
void sendPolyPressure(uint8_t note, uint8_t pressure, uint8_t channel);
void sendAllSoundsOffDisregardBypass(uint8_t channel=CHANNEL_OMNI);
void sendAllSoundsOff(uint8_t channel=CHANNEL_OMNI);


// SEND CONTROLLER COMMAND
// Sends a controller command, one of:
// CC, NRPN, RPN, PC
// [Only if INCLUDE_EXTENDED_CONTROL_SIGNALS]: Pitch Bend, Aftertouch
//
// These are defined by the CONTROL_TYPE_* constants defined elsewhere
//
// Some commands (CC, NRPN, RPN) have COMMAND NUMBERS.  The others ignore the provided number.
//
// Each command is associated with a VALUE.  All values are 14 bits and take the form
// of an MSB (the high 7 bits) and an LSB (the low 7 bits).  Some data expects lower
// resolution numbers than that -- in this case you must shift your data so it's zero-
// padded on the right.  For example, if you want to pass in a 7-bit
// number (for PC, some CC values, or AFTERTOUCH) you should do so as (myval << 7).
// If you want to pass in a signed PITCH BEND value (an int16_t from -8192...8191), you
// should do so as (uint16_t myval + MIDI_PITCHBEND_MIN).
//
// It's good practice to send a NULL RPN after sending an RPN or NRPN, but it's 50% more data
// in an already slow command.  So Gizmo doesn't do it by default.  You can make Gizmo do it
// with #define INCLUDE_SEND_NULL_RPN   [I'd set that somewhere in All.h]
//
// Here are the valid numbers and values for different commands:
//
// COMMAND	NUMBERS    	VALUES		NOTES
// OFF		[everything is ignored, this is just a NOP]
// CC		0-31		0-16383		1. If you send 7-bit data (zero-padded, shifted << 7) then the LSB will not be sent.
// CC		32-127		0-127		1. Zero-pad your 7-bit data (shift it << 7).
//									2. Some numbers are meant for special functions.  Unless you know what you're doing,
//									   it'd be wise not to send on numbers 6, 32--63, 96--101, or 120--127
// NRPN/RPN	0-16383		0-16383		1. If you send 7-bit data (zero-padded) then the LSB will not be sent.
//									2. The NULL RPN terminator will only be sent if #define INCLUDE_SEND_NULL_RPN is on.
//									   By default it's off.  See the end of http://www.philrees.co.uk/nrpnq.htm
//									3. You can also send in an RPN/NRPN DATA INCREMENT or DECREMENT.  To do this, pass in the
//									   value CONTROL_VALUE_INCREMENT [or DECREMENT] * 128 + DELTA.  A DELTA of 0 is the 
//									   same as 1 and is the most common usage.
// PC		[ignored]	0-127		1. Zero-pad your 7-bit data (shift it << 7)
// BEND		[ignored]	0-16383		1. BEND is normally signed (-8192...8191).  If you have the value as a signed int16_t,
//									   pass it in as (uint16_t) (myval + MIDI_PITCHBEND_MIN)
// AFTERTOUCH [ignored]	0-127		1. Zero-pad your 7-bit data (shift it << 7)

void sendControllerCommand(uint8_t commandType, uint16_t commandNumber, uint16_t fullValue, uint8_t channel);



//// LOCAL 

#define SYSEX_VERSION 0

#define NO_SYSEX_SLOT (-1)
#define SYSEX_TYPE_SLOT 0
#define SYSEX_TYPE_ARP 1
#define RECEIVED_WRONG (-1)
#define RECEIVED_BAD (-2)
#define RECEIVED_NONE (0)

struct _sysexLocal
    {
    int8_t slot;
    uint8_t type;
    int8_t received;
    };


#ifdef INCLUDE_SYSEX
void sendSlotSysex();
void sendArpSysex();
void handleSysex(unsigned char* bytes, int len);
#endif INCLUDE_SYSEX

#ifdef INCLUDE_PROVIDE_RAW_CC
void setParseRawCC(uint8_t val);
#endif



#endif __MIDI_INTERFACE__

