////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __DRUM_SEQUENCER_H__
#define __DRUM_SEQUENCER_H__



#include "All.h"




/////// THE DRUM SEQUENCER
//
// The drum sequencer contains note data in the form of single bits, indicating that a given (drum) note has been played.
// No per-note information is stored regarding the velocity, pitch, or other auxillary features of the note.  No note-off
// information is provided (indeed the Drum Sequencer never issues note off).   Notes (as bits)
// are packed into bytes, then stored in TRACKS of a certain length L long (such as 16 beats).  Some T tracks are packed
// into a GROUP.  There are G groups.  All groups have the same number of tracks and the same length tracks as far as storage
// is concerned; but groups can be set up to shorten the length of their tracks internally, allowing different group lengths
// but wasting space.
//
// Thus our storage is basically group BY track BY notes in track.  There is also additional per-group data, some per-track
// data, and some per-track BY per-group data.  
//
// A group is basically a repeating bar or phrase.  The purpose of a group is to allow you to have one sequence pattern, then
// after it has repeated some N times, switch to another group and start playing that one.  The description of which groups
// play and when is given by an array of 20 TRANSITIONS.  Each transition stipulates a group to be played, and how long to play it.
// When this is done, we go on to the next transition.
//
//





//
// 2. The user can enter notes STEP-BY-STEP at the cursor position, or (by moving the cursor position to off the left of the screen)
//    enter notes at the PLAY POSITION.  Notes can be entered while the sequencer is playing.  He can also add RESTS and TIES.
//    A tie says "keep on playing the note in the previous slot position".
// 
// 3. Stop, start, and restart the sequencer (including affecting external MIDI Clocks), mute tracks, and clear tracks.   
// 
// 4. Toggle ECHO.  Normally when you enter a note, in step-by-step mode, it is played so you can hear what you're entering
//    [in play position mode the drum sequencer doesn't play the notes immediately but instead lets the play cursor play them].
//    NO ECHO tells the drum sequencer to not play notes when you're entering them AT ALL.  This is useful when you are playing
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
// There are 8, 16, 32, or 64 notes per track per group. There are 12 or 16 tracks per group. There are no more than 15 groups.
//
// - Each note is 1 bit, so they are packed 8 notes to the byte.
// 
// - Additionally there are 2 bytes per track:
//		5 bits MIDI channel (0 = "off", 17 = "default")
//		3 bits velocity (15, 31, 47, 63, 79, 95, 111, 127)
//		7 bits note
//		1 bit mute
//
// - Additionally there is 1 byte per group:
//		4 bits actual group length (0 = default, 1...15 or appropriate division thereof)
//		4 bits note speed (0 = default, 1 ... 15 is 1-15 of the standard note speeds)
//
// - Additionally there is 1/2 byte per group per track
//		4 bits pattern
//
// Combinations of the above data comprise the LAYOUT or FORMAT of the sequence.  There are 
// presently EIGHT possible layouts but we can have as many as SIXTEEN
//
//		8 notes, 13 groups, 16 tracks = (8/8 * 16 + 1/2 * 16 + 1) * 13 + (2 * 16) = 357
//		16 notes, 8 groups, 16 tracks = (16/8 * 16 + 1/2 * 16 + 1) * 8 + (2 * 16) = 360
//		32 notes, 4 groups, 16 tracks = (32/8 * 16 + 1/2 * 16 + 1) * 4 + (2 * 16) = 324
//		64 notes, 2 groups, 16 tracks = (64/8 * 16 + 1/2 * 16 + 1) * 2 + (2 * 16) = 306
//
//		8/19/16 = 355
//		16/10/16 = 362
//		32/5/16 = 357
//		64 -- no help
//
//		8 notes, 15 groups, 12 tracks = (8/8 * 12 + 1/2 * 12 + 1) * 17 + (2 * 12) = 347
//          ** Note that there is room for 17 groups, but we can only refer to 15 of them
//		16 notes, 11 groups, 12 tracks = (16/8 * 12 + 1/2 * 12 + 1) * 11 + (2 * 12) = 365
//		32 notes, 6 groups, 12 tracks = (32/8 * 12 + 1/2 * 12 + 1) * 6 + (2 * 12) = 354
//		64 notes, 3 groups, 12 tracks = (64/8 * 12 + 1/2 * 12 + 1) * 3 + (2 * 12) = 333
//
//		8 -- way too many
//		16/13/12 = 349
//		32 -- no help
//		64 -- no help
//
//		8 notes, 6 groups, 32 tracks = (8/8 * 32 + 1/2 * 32 + 1) * 6 + (2 * 32) = 358
//		16 notes, 3 groups, 32 tracks = (16/8 * 32 + 1/2 * 32 + 1) * 3 + (2 * 32) = 307
//		32 notes, 2 groups, 32 tracks = (32/8 * 32 + 1/2 * 32 + 1) * 2 + (2 * 32) = 354
//		64 notes, 1 group, 32 tracks = (64/8 * 32 + 1/2 * 32 + 1) * 1 + (2 * 32) = 337
//
//		8/9/32 = 361
//		16/4/32 = 324
//		32	-- no help
//		64  -- no help
//
// Some other possibilities:
//
//		8 notes, 6 groups, 32 tracks = (8/8 * 32 + 1/2 * 24 + 1) * 13 + (2 * 32) = 358
//		16 notes, 3 groups, 32 tracks = (16/8 * 32 + 1/2 * 24 + 1) * 3 + (2 * 32) = 307
//		32 notes, 2 groups, 32 tracks = (32/8 * 32 + 1/2 * 24 + 1) * 2 + (2 * 32) = 354
//		64 notes, 1 group, 32 tracks = (64/8 * 32 + 1/2 * 32 + 1) * 1 + (2 * 32) = 337
//			/// WARNING: more than 32 tracks and the access macros must be upgraded to 16-bit
//		    /// WARNING: more than 32 tracks and we have to come up with a way to draw the track range
//		16 notes, 1 group, 80 tracks = (16/8 * 80 + 1/2 * 80 + 1) * 1 + (2 * 80) = 361
//
// Thus a layout comprises AT MOST 365 BYTES.
//
// There are also 20 bytes for transitions:
//		20 global group transitions.  These are <group, repeat> pairs indicating 
//		which group and then how many times to repeat it.  Each transition is 1 byte.
//			Group is 4 bits: (0 = OTHER, 1...15)
//			Repeat is 4 bits: 
//				If Group is OTHER then:	Random: Groups 1-2 (LOOP, 1 time, 2..., 3..., 4...), Groups 1-3 (LOOP, 1, 2, 3, 4), Groups 1-4 (LOOP, 1, 2, 3, 4), END)
//				If Group is not OTHER then: LOOP, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 32, 64
//				Note that Transition 0 cannot be END.  If this happens (which it should not be able to)
// 				then this is interpreted as LOOP FOREVER Group 1.
//
// There is also 1 byte for sequence repeats:
//		4 bits for overall repeats (LOOP, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 32, 64)
//		4 bits for next sequence after repeats have concluded (0 = END, 1...10 (for 0...9))
//
// There is also one global byte:
//		4 bits: up to 16 layouts.  Currently 3 bits is used (eight layouts)
//		4 bits: extra

// GLOBALS (TEMPORARY DATA)
//
// Temporary data is stored in local.drumSequencer.
//
// OPTIONS
//
// Permanent options special to the Drum Sequencer are:
//
// options.drumSequencerNoEcho          Toggle for Echo
//
// Other permanent options affecting the Drum Sequencer include:
//
// options.noteSpeedType
// options.swing
// options.channelIn
// options.channelOut
// options.volume
// options.tempo
//
//
// DISPLAY
// 
// Drum sequences are displayed in the top 6 rows of both LEDs.  A 16-note track takes up one full row.  32-note
// tracks take up two rows.  A 64-note track takes up 4 rows.
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
//      Drum Sequencer                  STATE_DRUM_SEQUENCER: choose a slot to load or empty.  If slot is not a drum sequencer slot, format:
//              Format                          STATE_DRUM_SEQUENCER_FORMAT:    specify the layout, then STATE_DRUM_SEQUENCER_PLAY
//              [Then Play]                     STATE_DRUM_SEQUENCER_PLAY
//                      Back Button: STATE_DRUM_SEQUENCER_SURE, then STATE_DRUM_SEQUENCER
//                      Left Knob:      scroll up/down track 
//                      Right Knob:     scroll left-right in track, or far left to enter PLAY POSITION mode
//                      Middle Button [step-by-step mode]:      rest
//                      Middle Button Long Press [step-by-step mode]: tie
//                      Middle Button [play position mode]:     mute track
//                      Middle Button Long Press [play position mode]: clear track
//                      Select Button:  toggle start/stop (pause) sequence playing
//                      Select Button Long Press: Menu          STATE_DRUM_SEQUENCER_MENU
//                              MENU:
//                                      Stop:                           Stop and RESET the sequence to its initial position
//                                      Reset Track:            Clear track and reset all of its options
//                                      Length:                         Set track note length (or default)              STATE_DRUM_SEQUENCER_LENGTH
//                                      Out MIDI (Track):       Set Track MIDI out (or default, or off)         STATE_DRUM_SEQUENCER_MIDI_CHANNEL_OUT
//                                      Velocity:                       Set Track note velocity (or none, meaning use each note's individual velocity)  STATE_DRUM_SEQUENCER_VELOCITY
//                                      Fader:                          Set Track fader         STATE_DRUM_SEQUENCER_FADER
//                                      Echo:                           Toggle ECHO mode
//                                      Save:                           Save the sequence.  STATE_DRUM_SEQUENCER_SAVE, then back to STATE_DRUM_SEQUENCER_PLAY
//                                      Options:                        STATE_OPTIONS (display options menu)




// There are three edited states: the file is brand new,
// the file has been loaded and not modified yet,
// and the file has been modified
#define EDITED_STATE_NEW 0
#define EDITED_STATE_LOADED 1
#define EDITED_STATE_EDITED 2

#define PLAY_STATE_STOPPED 0
#define PLAY_STATE_WAITING 2
#define PLAY_STATE_PLAYING 1

#define DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE (0)
#define DRUM_SEQUENCER_PATTERN_RANDOM_3_4 (14)
#define DRUM_SEQUENCER_PATTERN_RANDOM_1_2 (13)
#define DRUM_SEQUENCER_PATTERN_RANDOM_1_4 (6)
#define DRUM_SEQUENCER_PATTERN_RANDOM_1_8 (9)
#define DRUM_SEQUENCER_PATTERN_ALL (15)
#define P0000 (0)			// DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE
#define P1000 (1)
#define P0100 (2)
#define P1100 (3)
#define P0010 (4)
#define P1010 (5)
#define P0110 (6)			// DRUM_SEQUENCER_PATTERN_RANDOM_1_4
#define P1110 (7)
#define P0001 (8)
#define P1001 (9)			// DRUM_SEQUENCER_PATTERN_RANDOM_1_8
#define P0101 (10)
#define P1101 (11)
#define P0011 (12)
#define P1011 (13)			// DRUM_SEQUENCER_PATTERN_RANDOM_1_2
#define P0111 (14)			// DRUM_SEQUENCER_PATTERN_RANDOM_3_4
#define P1111 (15)			// DRUM_SEQUENCER_PATTERN_ALL

#define NUM_POTS (2)
#define LEFT_POT (0)
#define RIGHT_POT (1)

#define MIDDLE_C 								(60)

#define DRUM_SEQUENCER_NOT_MUTED (0)
#define DRUM_SEQUENCER_MUTED (1)
#define DRUM_SEQUENCER_MUTE_ON_SCHEDULED (2)
#define DRUM_SEQUENCER_MUTE_OFF_SCHEDULED (3)
#define DRUM_SEQUENCER_MUTE_ON_SCHEDULED_ONCE (4)
#define DRUM_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE (5)

#define DRUM_SEQUENCER_NO_SOLO (0)
#define DRUM_SEQUENCER_SOLO (1)
#define DRUM_SEQUENCER_SOLO_ON_SCHEDULED (2)
#define DRUM_SEQUENCER_SOLO_OFF_SCHEDULED (3)

#define MAX_TRACKS 								(32)
#define MAX_GROUPS 								(15)
#define NUM_FORMATS 							(8)			// we could go as high as 16
#define NUM_TRANSITIONS 						(20)
#define DATA_LENGTH 							(387)
#define GROUP_LENGTH_DEFAULT					(0)
#define NOTE_SPEED_DEFAULT						(0)
#define NO_MIDI_OUT 							(0)
#define MIDI_OUT_DEFAULT						(16)		// for now?  I'd prefer zero, see initDrumSequencer
#define MAX_NOTE_VELOCITY						(7)			// 127
#define INITIAL_NOTE_PITCH						(60)
#define TRANSITION_GROUP_OTHER					(0)
#define TRANSITION_OTHER_END					(15)
#define TRANSITION_REPEAT_LOOP					(0)
#define NEXT_SEQUENCE_END						(0)
#define SEQUENCE_REPEAT_LOOP					(0)


struct _drumSequencerLocal
    {
	uint8_t numGroups;
    uint8_t numTracks;
    uint8_t numNotes;												// This is not the *actual* length, just the max and default length.  Use getActualGroupLength(local.drumSequencer.currentGroup())
    uint8_t currentGroup;
    uint8_t currentTrack;
    uint8_t currentTransition;
    int8_t currentEditPosition;                                     // Where is the edit cursor?  Can be -1, indicating PLAY rather than STEP BY STEP entry mode, or can be >= getActualGroupLength(local.drumSequencer.currentGroup()), indicating "right mode"
    uint8_t currentPlayPosition;                                    // Where is the play position marker?
    uint8_t format;
    uint8_t repeatSequence;
    uint8_t nextSequence;
    uint8_t transitionGroup[NUM_TRANSITIONS];
    uint8_t transitionRepeat[NUM_TRANSITIONS];
	uint8_t muted[MAX_TRACKS];										// This is wasteful, maybe we can get rid of it
	uint8_t solo;
    uint8_t playState;                                              // is the sequencer playing, paused, or stopped?
    uint8_t shouldPlay[MAX_TRACKS];									// determined when we start note 0 of the sequence, based on the current pattern.  Used throughout the sequence afterwards to determine if we should play or mute the track that time around.
    uint8_t backup;      		// used for backing up data to restore it                                                           // used to back up various values when the user cancels
    uint8_t transitionGroupBackup;
    uint16_t pots[NUM_POTS];
    int16_t currentRightPot;  
    uint8_t notePulsetransitionCountdown;      
    uint8_t shouldPlay[MAX_DRUM_SEQUENCER_TRACKS];
    uint8_t performanceMode;
    uint8_t transitionCountdown;
    uint8_t sequenceCountdown;
    uint8_t patternCountup;
    
    /*
    uint8_t markTrack;
    uint8_t markPosition;
    uint8_t solo;
    */
    };


#define CHANNEL_ADD_TO_DRUM_SEQUENCER (-1)		// The default: performance notes just get put into the drum sequencer as normal
#define CHANNEL_DEFAULT_MIDI_OUT (0)			// Performance notes are routed to MIDI_OUT
												// Values 1...16: performance notes are routed to this channel number
#define CHANNEL_TRANSPOSE (17)					// Use performance note to do transposition

struct _drumSequencer
    {
    uint8_t format;
    uint8_t repeat;
    uint8_t transition[NUM_TRANSITIONS];
    uint8_t data[DATA_LENGTH]
    };


// Turns off all notes as appropriate (rests and ties aren't cleared),
// unless clearAbsolutely is true, in which case absolutely everything gets cleared regardless
void clearNotesOnTracks(uint8_t clearEvenIfNoteNotFinished);

// Draws the sequence with the given track length, number of tracks, and skip size
void drawDrumSequencer(uint8_t tracklen, uint8_t numTracks, uint8_t skip);

// Reformats the sequence as requested by the user
void stateDrumSequencerFormat();

// Plays and records the sequence
void stateDrumSequencerPlay();

// Plays the current sequence
void playDrumSequencer();

// Gives other options
void stateDrumSequencerMenu();

void stopDrumSequencer();

void resetDrumSequencer();


// Performance Options
void stateDrumSequencerMenuPerformanceKeyboard();
void stateDrumSequencerMenuPerformanceRepeat();
void stateDrumSequencerMenuPerformanceNext();
void loadDrumSequencer(uint8_t slot);
void resetDrumSequencerTransitionCountdown();
void stateDrumSequencerMenuPattern();

/*
// Edit Options
void stateDrumSequencerMenuEditMark();
void stateDrumSequencerMenuEditCopy(uint8_t splat, uint8_t paste);
void stateDrumSequencerMenuEditDuplicate();
*/

void packDrumSequenceData(struct _drumSequencer *seq);
void unpackDrumSequenceData(struct _drumSequencer *seq);


#endif __DRUM_SEQUENCER_H__

