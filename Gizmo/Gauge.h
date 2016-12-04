#ifndef __GAUGE_H__
#define __GAUGE_H__

/////// THE MIDI GAUGE
//
// The gauge can do the following:
//
// 1. Display Note on and Note Off Messages, showing pitch and velocity.
//
// 2. Optionally display Polyphonic Aftertouch Messages showing pitch and velocity.
//
// 3. Display Pitch bend values by drawing the entire 14-bit bend value.
//
// 4. Display Program Change messages as "PC" plus the 7-bit value.
//
// 5. Optionally display Channel Aftertouch Messages as "AT" plus the 7-bit value.
// 
// 6. Display all Polyphonic and Channel Aftertouch messages, as well as  by blinking a little LED.
// 
// 3. Display MIDI Clock, MIDI Time Code, and Active Sensing by 
//
// 2. Replay the 64 notes.  
// 
// 3. Toggle repeating (looping play) at the end of the most recent measure.   
// 
// 4. Toggle a click track (specify a note pitch and velocity to be played for 1/24 a beat, or cancel the same).
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
// As you play or record notes, a cursor moves across the screen to register NOTE ON messages.  With 64 messages, the
// cursor pass through the top four rows.  The next two rows are reserved for another cursor indicating the current
// measure.
//
//
// INTERFACE
//
// Root
//      Recorder                                STATE_RECORDER: choose a slot to load or empty.  If slot is not a recorder slot, format:
//              Format                          STATE_RECORDER_FORMAT, then play
//              [Then Play]                     STATE_RECORDER_PLAY
//                      Back Button: STATE_RECORDER_SURE, then STATE_RECORDER
//                      Middle Button:  play/stop
//                      Middle Button Long Press: start a 4-note count-off, then start recording
//                      Select Button:  save    STATE_RECORDER_SAVE
//                      Select Button Long Press: bring up menu         STATE_RECORDER_MENU
//                              MENU:
//                                      Repeat:                         Toggle repeat
//                                      Click:                          Provide a click note, or cancel the click
//                                      Options:                        STATE_OPTIONS (display options menu)



#include "All.h"


struct _gaugeLocal
    {
    uint8_t fastMidi[3];                            // display it or not?
    };
        

/// Most of Gauge's functions are embedded (inlined) in the TopLevel.cpp state machine. 



#endif __GAUGE_H__
