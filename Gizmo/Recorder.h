////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __RECORDER_H__
#define __RECORDER_H__

#include "All.h"




/////// THE RECORDER
//
// The recorder can do the following:
//
// 1. Record at least 64 notes spread over 21 measures.  Why 21?  Because 24 pulses per beat, 4 beats per measure, 21 measures is 2016,
//    and our timestamp is 11 bytes (2048).  Why AT LEAST 64?  Becasue it's possible some NOTE ON messages won't have corresponding
//    NOTE OFF messages if they don't occur until after measure 21.  But it'll be no more than 64 + 16 = 80.
//
// 2. Replay the 64 notes looping. 
// 
// 3. Toggle a click track (specify a note pitch and velocity to be played for 1/24 a beat, or cancel the same).
//
//
// STORAGE
//
// A slot first holds a 2-byte LENGTH (in bytes) for the buffer, then a 1-byte NUMBER OF NOTE ON MESSAGES stored in the buffer,
// then finally the buffer, of size 384.
//
// Note on and Note Off events are packed in order they occur.  A Note On message is 4 bytes, conisting of:
//
// 0 [indicates a Note ON], then 4 bits ID, then 11 bits timestamp, then 8 bits for pitch (1 bit unused of course), then 8 bits for velocity (1 bit unused)
//
// A Note Off message is 2 bytes, consisting of:
//
// 1 [indicates a Note OFF], then 4 bits ID, then 11 bits timestamp
//
// It's 64 notes because if we have a note off for each note on.  For each such pair we have 6 bytes, and 384 / 6 = 64.
//
//
// GLOBALS (TEMPORARY DATA)
//
// Temporary data is stored in local.recorder.
//
//
// OPTIONS
//
// Permanent options special to the Arpeggiator are:
//
// options.recorderRepeat                               Toggle for Repeat
// options.recorderClick                                Note to play for click track (or NO_NOTE)
// options.recorderClickVelocity                Velocity to play for click track
//
// Other permanent options affecting the Arpeggiator include:
//
// options.channelIn
// options.channelOut
// options.transpose
// options.volume
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



// The recorder may have no more than this number of notes
// playing at any one time.
#define MAX_RECORDER_NOTES_PLAYING 16

// status values
#define RECORDER_STOPPED 0
#define RECORDER_PLAYING 1
#define RECORDER_RECORDING 2
#define RECORDER_TICKING_OFF 3

// LOCAL

struct _recorderLocal
    {
    // timestamp -- how far we've played in time (up to 2015), as measured in pulses.
    // Before we have started playing, this value is -1.
    int16_t tick;

    // where we are in the buffer
    uint16_t bufferPos;

    // Notes currently outstanding.
    uint8_t notes[MAX_RECORDER_NOTES_PLAYING];
    
    // How many NOTE ONs we have played so far
    uint8_t currentPos;
    
    // one of the status values above: describes what we're doing right now
    uint8_t status;
    
    // the "pre count" prior to recording.
    uint8_t tickoff;
    
    // Number of notes recorded so far
    uint8_t numNotes;

    // Number of iterations played so far
    uint8_t iterations;

    // Are we scheduled to play?
    uint8_t playScheduled;
    };


#define MAX_RECORDER_NOTES 64
#define MAXIMUM_RECORDER_TICK   (2015)
#define RECORDER_BUFFER_SIZE    (SLOT_DATA_SIZE - 3)
#define RECORDER_SIZE_OF_NOTE_ON        (4)
#define RECORDER_SIZE_OF_NOTE_OFF       (2)

/*
// Low 9 bits 
#define GET_RECORDER_LENGTH() ((uint16_t)(data.slot.data.recorder.length & 511))
#define SET_RECORDER_LENGTH(val) (data.slot.data.recorder.length = ((data.slot.data.recorder.length & (127 << 9)) | (val & 511)))
// High 4 bits
#define GET_RECORDER_SPEED() ((uint8_t)((data.slot.data.recorder.length >> 12) & 15))
#define SET_RECORDER_SPEED(val) (data.slot.data.recorder.length = ((data.slot.data.recorder.length & 511) | ((((uint16_t)val) & 15) << 12)))
*/

struct _recorder
    {
    uint16_t length;                        // how many bytes are stored in the buffer (up to 384) (Low 9 bits) | 
    uint8_t repeat;							// how much should we repeat and where should we continue?  This is 1 time, 2, times, ..., 16 times (Low 4 bits) | 1, ..., 9 (High 4 bits)
    uint8_t buffer[RECORDER_BUFFER_SIZE];
    };



// Plays OR Records the song
void stateRecorderPlay();

// Removed for space, since we're not allowing repeating as an *option* any more.
//void stateRecorderMenu();

/// Resets the recorder entirely.  Called on MIDI Start etc.
void resetRecorder();


// This is a dummy function which does nothing at all, because we can't presently
// play in the background.  But it's included because if we DON'T have it, then
// Utility.playApplication() increases by 100 bytes.  :-(
void playRecorder();

void stateRecorderMenu();

void stateRecorderMenuPerformanceKeyboard();
void stateRecorderMenuPerformanceRepeat();  
void stateRecorderMenuPerformanceNext();

#endif

