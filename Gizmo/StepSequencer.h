////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __STEP_SEQUENCER_H__
#define __STEP_SEQUENCER_H__



#include "All.h"




/////// THE STEP SEQUENCER
//
// The step sequencer can do the following:
//
// 1. 12-track 16-note, 8-track 24-note, or 6-track 32-note step sequences.
//
// 2. The user can enter notes STEP-BY-STEP at the cursor position, or (by moving the cursor position to off the left of the screen)
//    enter notes at the PLAY POSITION.  Notes can be entered while the sequencer is playing.  He can also add RESTS and TIES.
//    A tie says "keep on playing the note in the previous slot position".
// 
// 3. Stop, start, and restart the sequencer (including affecting external MIDI Clocks), mute tracks, and clear tracks.   
// 
// 4. Toggle ECHO.  Normally when you enter a note, in step-by-step mode, it is played so you can hear what you're entering
//    [in play position mode the step sequencer doesn't play the notes immediately but instead lets the play cursor play them].
//    NO ECHO tells the step sequencer to not play notes when you're entering them AT ALL.  This is useful when you are playing
//    and entering notes using the same device (which is playing as you enter them).
//
// 5. Mute tracks, clear tracks, specify the MIDI OUT on a per-track basis (or use the default), specify the note velocity 
//    on a per-track basis (or use the velocity entered for each note), specify the note length on a per-track basis
//    (or use the default), change the volume of the tracks (a 7-bit fader), or save sequences.
//
// 6. Sequences are affected by SWING, by TEMPO, by NOTE SPEED, and by NOTE LENGTH.
//
//
// STORAGE
//
// Notice that the number of tracks, times the number of notes is 12x16 = 8x24 = 6x32 = 4x48 = 3x64 = 192.  The step sequencer stores
// the PITCH and the VELOCITY of each note, which are 7 bits each, in two bytes, for a total of 192 x 2 = 384 bytes.
// Additionally a REST is defined as having a pitch of 0 and a velocity of 0.  A TIE has a pitch of 1 and a velocity of 0.
//
// If we're storing CONTROL data rather than NOTE data, then 14-bits represent MSB + LSB, or (the value 2^14 - 1) "Nothing".
// Yes, this means you can't enter 2^14-1 as a sequence value, oh well.
//
// A step sequence consists of a FORMAT byte, a REPEAT byte, an UNUSED byte, and a 384 byte BUFFER holding the notes.  
//
// The FORMAT byte has two parts.  The low 3 bits indicate the maximum track length (one of 16, 18, 20, 24, 32, 36, 48, and 64).
// The high 5 bits indicate the actual track length, (one of MAX, 1, 2, ..., 31).
// Thus a track length of 64 cannot be reasonably shortened.
//
// Embedded in each track is some per-track data.  This data is stored in the single high unused bit in each byte in the buffer
// [recall that MIDI pitch and velocity are only 7 bits each].  Tracks can have as little as 32 bytes (16 notes x 2 bytes per note),
// so we have 32 bits to pack stuff into.  The data is (in order):
//
//// 1 bit NOTE vs CONTROL
//// If NOTE:
////     1 bit mute
////     5 bits MIDI out channel (including "use default", which is 17, and "no MIDI out", which is 0)
////     7 bits note length (0...100 as a percentage, or PLAY_LENGTH_USE_DEFAULT)
////     7 bits note velocity (0 = "use per-note velocity", or 1...127)
////     1 bit transposable
////     5 bits fader
////	 4 bits pattern

//// If CONTROL:
////     3 bits: CC MSB, NRPN MSB, RPN MSB, PC, BEND MSB, AFTERTOUCH, INTERNAL
////     14 bits Parameter
////     5 bits MIDI out channel
////	 4 bits pattern
////     5 bits unused

//
// This extra data is packed and unpacked in Utilities.stateSave and Utilities.stateLoad, using the private functions
// distributeByte and gatherByte (and stripHighBits).
//
//
// GLOBALS (TEMPORARY DATA)
//
// Temporary data is stored in local.stepSequencer.
//
// OPTIONS
//
// Permanent options special to the Step Sequencer are:
//
// options.stepSequencerNoEcho          Toggle for Echo
//
// Other permanent options affecting the Step Sequencer include:
//
// options.noteSpeedType
// options.swing
// options.channelIn
// options.channelOut
// options.transpose
// options.volume
// options.tempo
// options.noteLength
//
//
// DISPLAY
// 
// Step sequences are displayed in the top 6 rows of both LEDs.  A 16-note track takes up one full row.  24-note and 32-note
// tracks take up two rows.  A 48-note track takes up 3 rows.  A 64-note track takes up 4 rows.
// The sequencer also displays an EDIT CURSOR (a dot) and a PLAY CURSOR (a vertical set of dots).
// The play cursor shows where the sequencer is currently playing.  The edit cursor is where new notes played, or rests or 
// ties entered, will be put into the data.  This is known as STEP-BY-STEP editing mode.  You can move the edit cursor
// up or down (to new tracks), or back and forth.  If you move the cursor beyond the left edge of the screen, the PLAY CURSOR
// changes to indicate the current playing track.  Now if you play notes they will be entered in the current track at the
// play cursor as it is playing.  This is known as PLAY POSITION mode.  
//
// In step-by-step mode, the middle button enters rests or ties (with a long press).  In play position mode, the
// middle button either mutes or clears the track.  The select button stops and starts the sequencer, or (long press)
// brings up the sequecer's menu.
//
// The sequence display scrolls vertically to display more tracks as necessary.  The current track is displayed in the
// second row of the left LED.
//
//
// INTERFACE
//
// Root
//      Step Sequencer                  STATE_STEP_SEQUENCER: choose a slot to load or empty.  If slot is not a step sequencer slot, format:
//              Format                          STATE_STEP_SEQUENCER_FORMAT:    16, 24, or 32 notes, then STATE_STEP_SEQUENCER_PLAY
//              [Then Play]                     STATE_STEP_SEQUENCER_PLAY
//                      Back Button: STATE_STEP_SEQUENCER_SURE, then STATE_STEP_SEQUENCER
//                      Left Knob:      scroll up/down track 
//                      Right Knob:     scroll left-right in track, or far left to enter PLAY POSITION mode
//                      Middle Button [step-by-step mode]:      rest
//                      Middle Button Long Press [step-by-step mode]: tie
//                      Middle Button [play position mode]:     mute track
//                      Middle Button Long Press [play position mode]: clear track
//                      Select Button:  toggle start/stop (pause) sequence playing
//                      Select Button Long Press: Menu          STATE_STEP_SEQUENCER_MENU
//                              MENU:
//                                      Stop:                           Stop and RESET the sequence to its initial position
//                                      Reset Track:            Clear track and reset all of its options
//                                      Length:                         Set track note length (or default)              STATE_STEP_SEQUENCER_LENGTH
//                                      Out MIDI (Track):       Set Track MIDI out (or default, or off)         STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT
//                                      Velocity:                       Set Track note velocity (or none, meaning use each note's individual velocity)  STATE_STEP_SEQUENCER_VELOCITY
//                                      Fader:                          Set Track fader         STATE_STEP_SEQUENCER_FADER
//                                      Echo:                           Toggle ECHO mode
//                                      Save:                           Save the sequence.  STATE_STEP_SEQUENCER_SAVE, then back to STATE_STEP_SEQUENCER_PLAY
//                                      Options:                        STATE_OPTIONS (display options menu)





// Sequences may have no more than 12 tracks, but can have fewer depending on format
#define MAX_STEP_SEQUENCER_TRACKS 12

// local.stepSequencer.velocity[track] is set to this if it's not overriding the individual note velocities
#define STEP_SEQUENCER_NO_OVERRIDE_VELOCITY (0)

// local.stepSequencer.noteLength[track] is set to this if it's not overriding the default play length in options.noteLength
#define PLAY_LENGTH_USE_DEFAULT 101

// local.outMidi[track] is set to this if it's not overriding the default MIDI out in options.channelOut
#define MIDI_OUT_DEFAULT 17

// No MIDI out channel at all
#define NO_MIDI_OUT 0

// There are three edited states: the file is brand new,
// the file has been loaded and not modified yet,
// and the file has been modified
#define EDITED_STATE_NEW 0
#define EDITED_STATE_LOADED 1
#define EDITED_STATE_EDITED 2


#define PLAY_STATE_STOPPED 0
#define PLAY_STATE_WAITING 2
#define PLAY_STATE_PLAYING 1

// RESTS are NOTE 0 VEL 0, and TIES are NOTE 1 VEL 0

/// LOCAL

#define STEP_SEQUENCER_DATA_NOTE	0
#define STEP_SEQUENCER_DATA_CC		1		// raw CC, all 127 parameter numbers
#define STEP_SEQUENCER_DATA_14_BIT_CC	2	// cooked 14-bit, only 31 parameter numbers
#define STEP_SEQUENCER_DATA_NRPN	3
#define STEP_SEQUENCER_DATA_RPN	4
#define STEP_SEQUENCER_DATA_PC	5
#define STEP_SEQUENCER_DATA_BEND	6
#define STEP_SEQUENCER_DATA_AFTERTOUCH	7

#define CONTROL_VALUE_EMPTY (16383)
#define MAX_CONTROL_VALUE (16382)

#define COUNTDOWN_INFINITE (255)

#define STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE (0)
#define STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL (14)
#define STEP_SEQUENCER_PATTERN_RANDOM_3_4 (14)
#define STEP_SEQUENCER_PATTERN_RANDOM_1_2 (13)
#define STEP_SEQUENCER_PATTERN_RANDOM_1_4 (6)
#define STEP_SEQUENCER_PATTERN_RANDOM_1_8 (9)
#define STEP_SEQUENCER_PATTERN_ALL (15)
#define P0000 (0)			// STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE
#define P1000 (1)
#define P0100 (2)
#define P1100 (3)
#define P0010 (4)
#define P1010 (5)
#define P0110 (6)			// STEP_SEQUENCER_PATTERN_RANDOM_1_4
#define P1110 (7)
#define P0001 (8)
#define P1001 (9)			// STEP_SEQUENCER_PATTERN_RANDOM_1_8
#define P0101 (10)
#define P1101 (11)
#define P0011 (12)
#define P1011 (13)			// STEP_SEQUENCER_PATTERN_RANDOM_1_2
#define P0111 (14)			// STEP_SEQUENCER_PATTERN_RANDOM_3_4
#define P1111 (15)			// STEP_SEQUENCER_PATTERN_ALL



#define STEP_SEQUENCER_NOT_MUTED (0)
#define STEP_SEQUENCER_MUTED (1)
#define STEP_SEQUENCER_MUTE_ON_SCHEDULED (2)
#define STEP_SEQUENCER_MUTE_OFF_SCHEDULED (3)
#define STEP_SEQUENCER_MUTE_ON_SCHEDULED_ONCE (4)
#define STEP_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE (5)

#define STEP_SEQUENCER_NO_SOLO (0)
#define STEP_SEQUENCER_SOLO (1)
#define STEP_SEQUENCER_SOLO_ON_SCHEDULED (2)
#define STEP_SEQUENCER_SOLO_OFF_SCHEDULED (3)

#define STEP_SEQUENCER_REST_NOTE (255)


#define NO_TRACK (255)
#define NO_SEQUENCER_NOTE (255)
#define NO_SEQUENCER_POS (255)

struct _stepSequencerLocal
    {
    uint8_t playState;                                              // is the sequencer playing, paused, or stopped?
    int8_t currentEditPosition;                                     // Where is the edit cursor?  Can be -1, indicating PLAY rather than STEP BY STEP entry mode
    uint8_t currentPlayPosition;                                    // Where is the play position marker?
        
    // You'd think that the right way to do this would be to make a struct with each of these variables
    // and then just have an array of structs, one per track.  But it adds 400 bytes to the total code size.  :-(
        
    uint8_t data[MAX_STEP_SEQUENCER_TRACKS];						// What kind of data is this track? STEP_SEQUENCER_DATA_NOTE, STEP_SEQUENCER_DATA_CC, etc.
    uint8_t outMIDI[MAX_STEP_SEQUENCER_TRACKS];             		// Per-track MIDI out.  Can also be CHANNEL_DEFAULT
    uint8_t noteLength[MAX_STEP_SEQUENCER_TRACKS];  				// Per-track note length, from 0...100, or PLAY_LENGTH_USE_DEFAULT
    uint8_t muted[MAX_STEP_SEQUENCER_TRACKS];               		// Per-track mute toggle
    uint8_t velocity[MAX_STEP_SEQUENCER_TRACKS];    				// Per track note velocity, or STEP_SEQUENCER_NO_OVERRIDE_VELOCITY
    uint8_t fader[MAX_STEP_SEQUENCER_TRACKS];               		// Per-track fader, values from 1...16
    uint32_t offTime[MAX_STEP_SEQUENCER_TRACKS];    				// When do we turn off? 
    uint8_t noteOff[MAX_STEP_SEQUENCER_TRACKS];						// What note should be turned off?
    uint8_t shouldPlay[MAX_STEP_SEQUENCER_TRACKS];					// Should the track be played this time around (due to the pattern)?
    uint8_t transposable[MAX_STEP_SEQUENCER_TRACKS];				// Can this track be transposed in performance mode?
    uint8_t pattern[MAX_STEP_SEQUENCER_TRACKS];						// Track pattern
    uint8_t dontPlay[MAX_STEP_SEQUENCER_TRACKS];					// Don't play the note on this track this step because it was played manually while being entered
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
	uint16_t controlParameter[MAX_STEP_SEQUENCER_TRACKS];			// If the data is a control data type, what is its parameter?
    uint16_t lastControlValue[MAX_STEP_SEQUENCER_TRACKS];			// If the data is a control data type, what was the last control value it held?
#endif
    uint8_t newData;				// A temporary variable.  comes in from STATE_STEP_SEQUENCER_MENU_TYPE, used in STATE_STEP_SEQUENCER_MENU_TYPE_PARAMETER
	int8_t transpose;				// Current transposition due to performance mode
    uint8_t performanceMode;		// We are in performane mode
    uint8_t goNextSequence;			// We're manually scheduled to go to the next sequence at the end of this iteration 
    uint8_t scheduleStop;			// We're manually scheduled to stop at the end of this iteration 
    uint8_t countdown;				// Number of iterations left before we terminate or go to the next sequence automatically
    uint8_t countup;				// Position in the pattern
    uint16_t pots[2];				// Pot values (left and right)
    uint8_t markTrack;				// Track where our mark is located 
    uint8_t markPosition;			// Position in the track where our mark is located 
    uint8_t solo;					// Solo is on
    uint8_t currentTrack;           // which track are we editing?
    uint8_t backup;      			// used for backing up data to restore it                                                           // used to back up various values when the user cancels
    int16_t currentRightPot;		// Current X position of cursor
    uint8_t lastNote;				// The most recent note value that was entered, or NO_SEQUENCER_NOTE
    uint8_t lastNotePos;			// The position at which the last note was entered, or NO_SEQUENCER_POS
	uint8_t lastExclusiveTrack;     // The last track chosen for exclusive random
    };


#define FADER_IDENTITY_VALUE 16

/// DATA

// There are four step sequencer formats available
#define STEP_SEQUENCER_FORMAT_16x12_ 0
#define STEP_SEQUENCER_FORMAT_24x8_ 1
#define STEP_SEQUENCER_FORMAT_32x6_ 2
#define STEP_SEQUENCER_FORMAT_48x4_ 3
#define STEP_SEQUENCER_FORMAT_64x3_ 4
#define STEP_SEQUENCER_FORMAT_96x2_ 5

#define CHANNEL_ADD_TO_STEP_SEQUENCER (-1)		// The default: performance notes just get put into the step sequencer as normal
#define CHANNEL_DEFAULT_MIDI_OUT (0)			// Performance notes are routed to MIDI_OUT
												// Values 1...16: performance notes are routed to this channel number
#define CHANNEL_TRANSPOSE (17)					// Use performance note to do transposition

#define STEP_SEQUENCER_BUFFER_SIZE		(SLOT_DATA_SIZE - 3)

struct _stepSequencer
    {
    uint8_t format;                                 // step sequencer format in question
    uint8_t repeat;									// how much should we repeat and where should we continue?  This is forever, 1 time, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 times (Low 4 bits) | STOP, 1, ..., 9 (High 4 bits)
	uint8_t unused;
    uint8_t buffer[STEP_SEQUENCER_BUFFER_SIZE];
    };


// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
extern uint8_t _trackLength[6];
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
extern uint8_t _numTracks[6];


/// TRACK FORMAT MACROS
/// (These are here rather than in StepSequencer.cpp so Utility.cpp can access them)
/// The track format is stored in data.slot.data.stepSequencer.format
/// and it consists of 5 high bytes defining the "custom length", namely values 0...31,
/// plus 3 low bytes defining the basic step sequencer format, namely values 0...7,
/// which signify 16, 24, 32, 48, 64, 96, UNUSED, UNUSED.
/// The maximum length is at present 96.  The length of a track is defined as either
/// the basic format length (if the custom length is 0) or the basic format length 
/// minus 32 plus the custom length.  For example, a basic format of 64 can either be
/// the full 64 (if the custom length is 0) or it can be 33 ... 63 (if the custom length
/// is 1 ... 31).  Similarly a basic format of 96 can either be the full 96 (custom length 0)
/// or 65 ... 95 (custom length 1 ... 31).

// The largest track size
#define MAXIMUM_TRACK_LENGTH (96)
// The custom length value which indicates that the track has no custom length
#define TRACK_LENGTH_FULL (0)
// Returns the custom length, including TRACK_LENGTH_FULL
#define GET_TRACK_CUSTOM_LENGTH() 	((data.slot.data.stepSequencer.format >> 3) & 31)
// Returns the basic format
#define GET_TRACK_FORMAT() 	(data.slot.data.stepSequencer.format & 7)
// Returns the full track length irrespective of the custom length
#define GET_TRACK_FULL_LENGTH() 	(_trackLength[GET_TRACK_FORMAT()])
// Returns the number of tracks in the current format
#define GET_NUM_TRACKS() (_numTracks[GET_TRACK_FORMAT()])
// Returns the minimum possible custom length
#define GET_MINIMUM_CUSTOM_LENGTH() \
	(GET_TRACK_FORMAT() == STEP_SEQUENCER_FORMAT_48x4_ ? 17 : \
	(GET_TRACK_FORMAT() == STEP_SEQUENCER_FORMAT_64x3_ ? 33 : \
	(GET_TRACK_FORMAT() == STEP_SEQUENCER_FORMAT_96x2_ ? 65 : \
	1)))
// Returns the actual track length in the current format
#define GET_TRACK_LENGTH() \
	((GET_TRACK_CUSTOM_LENGTH() == TRACK_LENGTH_FULL) ? GET_TRACK_FULL_LENGTH() : \
	(GET_TRACK_FORMAT() == STEP_SEQUENCER_FORMAT_48x4_ ? GET_TRACK_CUSTOM_LENGTH() + 16 : \
	(GET_TRACK_FORMAT() == STEP_SEQUENCER_FORMAT_64x3_ ? GET_TRACK_CUSTOM_LENGTH() + 32 : \
	(GET_TRACK_FORMAT() == STEP_SEQUENCER_FORMAT_96x2_ ? GET_TRACK_CUSTOM_LENGTH() + 64 : \
	GET_TRACK_CUSTOM_LENGTH()))))




// Turns off all notes as appropriate (rests and ties aren't cleared),
// unless clearAbsolutely is true, in which case absolutely everything gets cleared regardless
void clearNotesOnTracks(uint8_t clearEvenIfNoteNotFinished);

// If lastNote is not NO_SEQUENCER_NOTE and lastNotePos is not NO_SEQUENCER_POS,
// creates a series of ties from lastNotePos to pos, and removes successive ties from pos.
void tieNote(uint8_t note, uint8_t pos);

// Draws the sequence with the given track length, number of tracks, and skip size
//void drawStepSequencer(uint8_t tracklen, uint8_t numTracks, uint8_t skip);

// Reformats the sequence as requested by the user
void stateStepSequencerFormat();

// Plays and records the sequence
void stateStepSequencerPlay();

// Plays the current sequence
void playStepSequencer();

// Gives other options
void stateStepSequencerMenu();

#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
void stateStepSequencerMenuRest();
void stateStepSequencerMenuType();
void stateStepSequencerMenuTypeParameter();
#endif

void stopStepSequencer();

void resetStepSequencer();

void stateStepSequencerMenuLength();

// Performance Options
void stateStepSequencerMenuPerformanceKeyboard();
void stateStepSequencerMenuPerformanceRepeat();
void stateStepSequencerMenuPerformanceNext();
//void loadStepSequencer(uint8_t slot);
void resetStepSequencerCountdown();
void stateStepSequencerMenuPattern();
void stateStepSequencerMenuPerformanceStop();

// Edit Options
void stateStepSequencerMenuEditMark();
void stateStepSequencerMenuEditCopy(uint8_t splat, uint8_t paste);
void stateStepSequencerMenuEditDuplicate();

#endif

