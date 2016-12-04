#ifndef __GAUGE_H__
#define __GAUGE_H__



/////// THE MIDI GAUGE
//
// The gauge can do the following:
//
// 1. Display Note on, Note Off, and Poly Aftertouch Messages, showing pitch and velocity.
// 
// 2. Display Pitch bend values by drawing the entire 14-bit bend value.
//
// 3. Display Program Change messages as "PC" plus the 7-bit value.
//
// 4. Display Channel Aftertouch Messages as "AT" plus the 7-bit value.
// 
// 5. Display MIDI Clock, MIDI Time Code, and Active Sensing with small LEDs
//
// 6. Display Channel Control, Channel Mode, NRPN, or RPN messages with scrolled text
//
// 7. Display Sysex, Song Position, Song Select, Tune Request, Start, Continue, Stop, or System Reset with
//    four letters of text (see below).
//
//
// GLOBALS (TEMPORARY DATA)
//
// Temporary data is stored in local.gauge.
//
//
// OPTIONS
//
// Affecting the Gauge include:
//
// options.channelIn
//
//
// DISPLAY
// 
// If you receive NOTE ON, NOTE OFF, or POLYPHONIC AFTERTOUCH, the screen will display both the note pitch and volume/pressure.
// If you receive PITCH BEND the screen will show the full bend value.
// If you receive CHANNEL AFTERTOUCH, "AT" will be displayed followed by the value.
// If you receive PROGRAM CHANGE, "PC" will be displayed, followed by the value.
// If you receive CHANNEL CONTROL, CHANNEL MODE, NRPN, or RPN, text will be scrolled: first the MSB will be shown, then 
//     					"CC", "CHANNEL MODE", "NRPN", or "RPN", then the parameter number, then (if 14-bit) the MSB+LSB 
//						will be shown in parentheses
// If you receive SysEx, Song Position, Song Select, Tune Request, Start, Continue, Stop, or System Reset, then
// 						SYSX, SPOS, SSEL, TREQ, STRT, CONT, STOP, or RSET will be shown.
// If you receive MIDI Clock, then LED(2,5) on the right Matrix will light
// If you receive MIDI Time Code, then LED(2,6) on the right Matrix will light
// If you receive Active Sensing, then LED(2,7) on the right Matrix will light
//
//
// INTERFACE
//
// Root
//      Gauge                   STATE_GAUGE
//              Back Button: 	STATE_ROOT 




#include "All.h"


struct _gaugeLocal
    {
    uint8_t fastMidi[3];                            // display it or not?
    };
        

/// Most of Gauge's functions are embedded (inlined) in the TopLevel.cpp state machine. 



#endif __GAUGE_H__
