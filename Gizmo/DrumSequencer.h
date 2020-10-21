////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __DRUM_SEQUENCER_H__
#define __DRUM_SEQUENCER_H__



#include "All.h"




/////// THE DRUM SEQUENCER


/*
The drum sequencer contains note data in the form of single bits, indicating that a given (drum) note has been played.
No per-note information is stored regarding the velocity, pitch, or other auxillary features of the note.  No note-off
information is provided (indeed the Drum Sequencer never issues note off).   Notes (as bits)
are packed into bytes, then stored in TRACKS of a certain length L long (such as 16 beats).  Some T tracks are packed
into a GROUP.  There are G groups.  All groups have the same number of tracks and the same length tracks as far as storage
is concerned; but groups can be set up to shorten the length of their tracks internally, allowing different group lengths
but wasting space.

Thus our storage is basically group BY track BY notes in track.  There is also additional per-group data, some per-track
data, and some per-track BY per-group data.  

A group is basically a repeating bar or phrase.  The purpose of a group is to allow you to have one sequence pattern, then
after it has repeated some N times, switch to another group and start playing that one.  The description of which groups
play and when is given by an array of 20 TRANSITIONS.  Each transition stipulates a group to be played, and how long to play it.
When this is done, we go on to the next transition.

Sequence data is at data.slot.data.drumSequencer.data[].  See the DATA ACCESS MACROS AND FUNCTIONS section in DrumSequencer.cpp
for accessing these bytes.


STORAGE

The Drum Sequencer consists of some G *GROUPS*.  You can think of these as bars or measures.  The sequencer plays/edits
one group at a time.  You can set up up to 20 TRANSITIONS which specify the sequence of groups in final performance,
and how long each group is played before transitioning to the next group.

The Sequencer also has some T *TRACKS*.  Each track is a different drum sound.  Tracks are associated with a MIDI channel,
a note to play on that channel, and a velocity to play that note with.

A sequence is thus a double array of GROUP by TRACK.  Each cell in this array is a sequence of NOTES assigned to the track
within that group.  Each such sequence can also be assigned a PATTERN of muting or playing while the group is playing.

There can be no more than 15 groups.
There are 5, 8, 12, 16, or 20 tracks. 
There are 8, 16, 32, or 64 notes per track per group.

- Each note is 1 bit, so they are packed 8 notes to the byte.

- Additionally there are 2 bytes per track:
		5 bits MIDI channel (0 = "off", 17 = "default")
		3 bits velocity (15, 31, 47, 63, 79, 95, 111, 127)
		7 bits note
		1 bit mute

- Additionally there is 1 byte per group:
		4 bits actual group length (0 = FULL, 1...15 is 1/16 ... 15/16 of full group length, or 1...7 if the full length is 8)
		4 bits note speed (0 = default, 1 ... 15 is 1-15 of the standard note speeds.  This means that the fastest speed is not available)
... or maybe
		6 bits actual group length (0 = 1, 1 = 2, ..., 63 = 64)		// you can't have a group with no notes
		2 bits dunno.  Note speed?  0 = default, 1 = 2x speed, 2 = 4x speed, 3 = half speed

- Additionally there is 1/2 byte per group per track
		4 bits pattern

Combinations of the above data comprise the LAYOUT or FORMAT of the sequence.  Here are our 16 currently:

		8 notes, 10 groups, 20 tracks = (8/8 * 20 + 1/2 * 20 + 1) * 10 + (2 * 20) = 350
		16 notes, 6 groups, 20 tracks = (16/8 * 20 + 1/2 * 20 + 1) * 6 + (2 * 20) = 346
		32 notes, 3 groups, 20 tracks = (32/8 * 20 + 1/2 * 20 + 1) * 3 + (2 * 20) = 313

		8 notes, 13 groups, 16 tracks = (8/8 * 16 + 1/2 * 16 + 1) * 13 + (2 * 16) = 357
		16 notes, 8 groups, 16 tracks = (16/8 * 16 + 1/2 * 16 + 1) * 8 + (2 * 16) = 360
		32 notes, 4 groups, 16 tracks = (32/8 * 16 + 1/2 * 16 + 1) * 4 + (2 * 16) = 324
		64 notes, 2 groups, 16 tracks = (64/8 * 16 + 1/2 * 16 + 1) * 2 + (2 * 16) = 306

		8 notes, 15 groups, 12 tracks = (8/8 * 12 + 1/2 * 12 + 1) * 15 + (2 * 12) = 309
         ** Note that there is room for 17 groups, but we can only refer to 15 of them
		16 notes, 11 groups, 12 tracks = (16/8 * 12 + 1/2 * 12 + 1) * 11 + (2 * 12) = 365
		32 notes, 6 groups, 12 tracks = (32/8 * 12 + 1/2 * 12 + 1) * 6 + (2 * 12) = 354
		64 notes, 3 groups, 12 tracks = (64/8 * 12 + 1/2 * 12 + 1) * 3 + (2 * 12) = 333

		16 notes, 15 groups, 8 tracks = (16/8 * 8 + 1/2 * 8 + 1) * 15 + (2 * 8) = 331
         ** Note that there is room for 16 groups, but we can only refer to 15 of them
		32 notes, 9 groups, 8 tracks = (32/8 * 8 + 1/2 * 8 + 1) * 9 + (2 * 8) = 324
		64 notes, 5 groups, 8 tracks = (64/8 * 8 + 1/2 * 8 + 1) * 5 + (2 * 8) = 361

		32 notes, 12 groups, 6 tracks = (32/8 * 6 + 1/2 * 6 + 1) * 12 + (2 * 6) = 348
		64 notes, 10 groups, 4 tracks = (64/8 * 4 + 1/2 * 4 + 1) * 10 + (2 * 4) = 358

Notes MUST be a mulitple of 8, because we're packing them into bytes.
Tracks MUST be an even number, so we can lay out the group per track half-bytes properly.
Group can be any number, but the DrumSequencer can only refer to up to 15 groups in its transitions.

Thus a layout comprises AT MOST 365 BYTES.

Side notes:
			If we were to REMOVE the pattern from 16-track, then we could get 2 more groups out of the 8 and 16 steps
			And one more group out of the 32 step.
					8/19/16 = 355		[in reality this maxes out at 15 groups]
					16/10/16 = 362
					32/5/16 = 357
					64 -- no help

			If we were to REMOVE the pattern from 12-track, then we could get 2 more groups out of the 16 steps
					8 -- way too many
					16/13/12 = 349
					32 -- no help
					64 -- no help

			Some other possibilities:
					8 notes, 6 groups, 32 tracks = (8/8 * 32 + 1/2 * 24 + 1) * 13 + (2 * 32) = 358
					16 notes, 3 groups, 32 tracks = (16/8 * 32 + 1/2 * 24 + 1) * 3 + (2 * 32) = 307
					32 notes, 2 groups, 32 tracks = (32/8 * 32 + 1/2 * 24 + 1) * 2 + (2 * 32) = 354
					64 notes, 1 group, 32 tracks = (64/8 * 32 + 1/2 * 32 + 1) * 1 + (2 * 32) = 337
						//WARNING: more than 32 tracks and the access macros must be upgraded to 16-bit
						//WARNING: more than 32 tracks and we have to come up with a way to draw the track range
					16 notes, 1 group, 80 tracks = (16/8 * 80 + 1/2 * 80 + 1) * 1 + (2 * 80) = 361


There are also 20 bytes for transitions:
		20 global group transitions.  These are <group, repeat> pairs indicating 
		which group and then how many times to repeat it.  Each transition is 1 byte.
			Group is 4 bits: (0...14, 15 = SPECIAL)
			Repeat is 4 bits: 
				If Group is SPECIAL then:	END, Random: Groups 0-1 (LOOP 1 time, 2..., 3..., 4...), Groups 0-2 (LOOP, 1, 2, 3, 4), Groups 0-3 (LOOP, 1, 2, 3, 4)
				If Group is not SPECIAL then: LOOP, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 24, 32, 64, BIG LOOP
				Note that Transition 0 cannot be END.  If this happens (which it should not be able to)
				then this is interpreted as LOOP FOREVER Group 0.

There is also 1 byte for sequence repeats:
		4 bits for overall repeats (LOOP, 1, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128)
		4 bits for next sequence after repeats have concluded (0 = END, 1...10 (for 0...9))

There is also one global byte:
		4 bits: up to 16 layouts.
		4 bits: extra
		
		
---- Note: I think that 'transitions' is normally called a CHAIN
---- Note: I think that 'group' is normally called a PATTERN


GLOBALS (TEMPORARY DATA)

Temporary data is stored in local.drumSequencer.

Some limited sequence data (transition group, 
transition repeat, track muted) are copied to these globals mostly to be compatible with original StepSequencer code.
These might ultimately be removed.  At present the packDrumSequenceData() and unpackDrumSequenceData() functions load
and unlload these globals, plus set up some other stuff.



OPTIONS

Permanent options special to the Drum Sequencer are:

options.drumSequencerNoEcho          Toggle for Echo

Other permanent options affecting the Drum Sequencer include:

options.noteSpeedType
options.swing
options.channelIn
options.channelOut
options.volume
options.tempo


MODES

Like the StepSequencer, the DrumSequencer has three playing modes: PLAY-POSITION MODE, where when you hit keys,
data is entered at the current point that the drum sequencer is playing, STEP-BY-STEP or EDIT MODE, where the data is entered 
where the edit cursor is located, and PERFORMANCE MODE.  In performance mode, when you hit keys different
things can happen: they could be blocked, or routed to other MIDI channels.  

The mode also affects which buttons and pots do what.  
In anticipation of the need to add additional buttons, the drum sequencer has an additional mode currently called
FAR RIGHT mode.  Whereas play-position mode is entered by shifting the play cursor off-screen to the left, beyond
the beginning of the notes, far-right mode is entered by shifting the cursor off-screen to the right, beyond the
end of the notes.  At present this mode is DISABLED.

My current arrangement of buttons and pots is as follows:

	LEFT KNOB
		Play Position:	Track
		Edit:			Track
		Far right:		Change Group
		Performance:	Track
		
	RIGHT KNOB
		Position, or far left for enter in real time, or far right for groups
		Performance:	Transition to.. ???    Change tempo?  
		
	BACK BUTTON
		Play Position:	Exit
		Edit:			Exit
		Far right:		Exit
		Performance:	Leave Performance Mode
		
	SELECT BUTTON
		Play Position:	Start/Stop (within Group)
		Edit:			Start/Stop (within Group)
		Far right:		Schedule Increment Group
		Performance:	Start/Stop (transition sequence)
		
	LONG SELECT BUTTON
						Menu
	
	MIDDLE BUTTON
		Play Position:	Toggle Mute
		Edit:			Toggle Note
		Far right:		Increment Group
		Performance:	Schedule Mute
		
	LONG MIDDLE BUTTON
		Play Position:	Clear Track in Group
		Edit:			Note (Track)		// Add Group to Transitions
		Far right:		Decrement Group
		Performance:	Schedule Solo

	SELECT + MIDDLE LONG
		Play Position:	Performance Mode
		Edit:			Performance Mode
		Far right:		Performance Mode
		Performance:	Schedule Transition
	
	
Where to put: in NON-performance mode: schedule increment and maybe decrement?
		
MENU OPTIONS
		Solo	[Or in performance mode, pause]
		Reset Track?
		Change Group
		Group Length
		Group Tempo [note speed]
		Out MIDI (Track)
		Velocity (Track)
		Pattern (Track/Group)
		Edit operations -> copy to group?
		Clock Control
		Performance
			...?
		Save
		Options
		  


NOTE ENTRY

Note entry is quite different from what you'd expect give the step sequencer, since we're not entering NOTES
but rather sequences of DRUM HITS.  So we abuse the keyboard as follows:

In PLAY POSITION MODE, 
    - White keys or C# and D# SET the current note
    - Other black keys CLEAR the current note

In STEP-BY-STEP (EDIT) MODE, 
    - White keys in Edit mode, starting with Middle C, correspond to individual drumbeats.  Pressing them toggles the beat.
      Think of this like toggling the beat buttons on a classic 808 or 909 drum machine.
	- The C# and D# Black keys in Edit mode SET the *current* note, which is specified by the cursor
	- Other black keys in Edit mode CLEAR the *current* note, which is specified by the cursor

In PERFORMANCE MODE, keystrokes are routed as specified by the user via menu options.




DATA DESCRIPTIONS:

	The FORMAT (or LAYOUT) specifies T Tracks by G Groups, and each group has (the same) maximum length of N Notes
	
	Each NOTE can only be ON or OFF
	Each GROUP has:
		- A LENGTH whch is less than the maximum length.  This is defined as a fraction of the maximum length (4 bits).
		- A NOTE SPEED which may be different from the default speed.  This will cause the group to play at this speed.
		  I *think* I can get this to work?
	Each TRACK has:
		- A MIDI Channel, or off, or default
		- A note velocity (volume)
		- A note pitch (drum note)
		- A mute flag
	Each TRACK/GROUP COMBO has:
		- A pattern.  Patterns define whether the notes are muted played over a four-bar repeating sequence.  Most
		  patterns are deterministic (like PLAY PLAY MUTE PLAY), while others are random.
	
	Additionally, there is a list of up to M TRANSITIONS.  During performance mode, playing the sequence will start
	at transition 0 and gradually increase in transition index.  Transitions define which group is currently being 
	played.
	
	Each TRANSITION has:
		- A group indicating which group is presently being played, or the special group "OTHER"
		- The number of times ("repeats") the group is to be played before transitioning to the next group.
			- If the group was OTHER, then the repeats means something else:
					- Repeats can be "END", meaning that the transition is actually just a PREMATURE END OF SEQUENCE marker
					- Or it can be one of 15 "RANDOM" options to indicate that the group was selected randomly

	Finally, you can stipulate how often the sequence as a *whole* should be iterated during performance mode, 
	including all of its transitions and their repeats.  And you can stipulate what to do at the end of these iterations:
	either simply STOP, or start playing a new sequence in a different Gizmo storage slot.  This last feature is 
	borrowed from the Step Sequencer; we'll see if I can get it working.
	
	

DISPLAY

Drum sequences are displayed in the top 6 rows of both LEDs.  A 16-note track takes up one full row.  32-note
tracks take up two rows.  A 64-note track takes up 4 rows.

The sequencer also displays an EDIT CURSOR (a dot) and a PLAY CURSOR (a vertical set of dots).
The play cursor shows where the sequencer is currently playing.  The edit cursor is where new notes played, or rests or 
ties entered, will be put into the data.  This is known as STEP-BY-STEP editing mode.  You can move the edit cursor
up or down (to new tracks), or back and forth.  If you move the cursor beyond the left edge of the screen, the PLAY CURSOR
changes to indicate the current playing track.  Now if you play notes they will be entered in the current track at the
play cursor as it is playing.  This is PLAY POSITION mode.  

In step-by-step mode, the middle button enters rests or ties (with a long press).  In play position mode, the
middle button either mutes or clears the track.  The select button stops and starts the sequencer, or (long press)
brings up the sequecer's menu.

The sequence display scrolls vertically to display more tracks as necessary.  The current track is displayed in the
second row of the left LED.


INTERFACE

Root
     Drum Sequencer                  STATE_DRUM_SEQUENCER: choose a slot to load or empty.  If slot is not a drum sequencer slot, format:
             Format                          STATE_DRUM_SEQUENCER_FORMAT:    specify the layout, then STATE_DRUM_SEQUENCER_PLAY
             [Then Play]                     STATE_DRUM_SEQUENCER_PLAY
                     Select Button Long Press: Menu          STATE_DRUM_SEQUENCER_MENU
                             MENU:
                                     Solo:                   Toggle solo (outside of performance mode)
                                     Reset Track:            Reset track or group - not sure yet
                                     Length (Group):         Set group length length
                                     Speed (Group):       	 Set group speed
                                     Out MIDI (Track):         Set track midi channel
                                     Velocity (Track):       	 Set track velocity (volume)
                                     Note (Track):         	Set Track pitch (drum note)
                                     Transitions:       	Select a transition, then...	 
                                     	Group:					Select a group for the transition or ----, then ....
                                     	Repeat:					[If a group was selected, choose how many repeats, or...]
                                     	Other:					[If ---- was selected, choose the END marker or a random-group option]
                                     Pattern (Track/Group):	Set Track/Group pattern
                                     Clock Control:         Toggle cock control
                                     Echo:       	 		Toggle Echo
                                     Performance:         	MENU
                                     	Keyboard:			Route keys to sequencer, or out a MIDI channel
                                     	Repeat:				How often to repeat the entire sequence
                                     	Next:				What to do after the sequence has been repeated 
                                     	(No) Reset On Stop:	Should we reset to the beginning of the sequence when stop is pressed?
                                     Save:       	 		Save sequence




Track Utilities:
	Accent Track?
	Copy Track Info
//	Clear Track on Group
	Reset Track					// I don't like this one
	Swap with Track
	
Group utilities:
	Copy Whole Group
	Clear Group
	Change Group
	
Transition Utilities:
	Add Group to Transitions
	Swap Transitions
	Copy Transition
	Edit Transition
	
	
	//	Note (Track)

Menus
	Solo	[or pause]
	Mark
	Local
		Pattern (Track/Group)
		Copy Track
		Swap Tracks
	Track
		Note 
		Out MIDI 
		Velocity 
		Copy Whole Track
		Swap Whole Tracks
		Accent Track
		Distribute Track Info
	Group
		Length
		Tempo
		Copy Group
		Swap Groups
		Clear Group
	Transition
		Add Group to Transitions
		Edit Transition
		Go to Group
		Swap Transition Up
		Swap Transition Down
	Edit operations -> copy to group?
	Clock Control
	Performance
		...?
	Save
	Options
	
		

*/





// There are three edited states: the file is brand new,
// the file has been loaded and not modified yet,
// and the file has been modified
#define EDITED_STATE_NEW 0
#define EDITED_STATE_LOADED 1
#define EDITED_STATE_EDITED 2

#define PLAY_STATE_STOPPED 0
#define PLAY_STATE_WAITING 2
#define PLAY_STATE_PLAYING 1
#define PLAY_STATE_PAUSED 3

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

#define MAX_DRUM_SEQUENCER_TRACKS 								(20)
#define MAX_DRUM_SEQUENCER_GROUPS 								(15)
#define DRUM_SEQUENCER_DEFAULT_FORMAT 							(8)			// 16 note 11 group 12 track
#define DRUM_SEQUENCER_NUM_FORMATS 							(16)
#define DRUM_SEQUENCER_NUM_TRANSITIONS 						(20)
#define DRUM_SEQUENCER_DATA_LENGTH 							(365)		// This is the largest necessary bytes
#define DRUM_SEQUENCER_GROUP_LENGTH_DEFAULT					(0)
#define DRUM_SEQUENCER_NOTE_SPEED_DEFAULT						(0)
#define DRUM_SEQUENCER_NO_MIDI_OUT 							(0)
#define DRUM_SEQUENCER_MIDI_OUT_DEFAULT						(17)		// for now?  I'd prefer zero, see initDrumSequencer
#define DRUM_SEQUENCER_MAX_NOTE_VELOCITY						(7)			// 127
#define DRUM_SEQUENCER_INITIAL_NOTE_PITCH						(60)
#define DRUM_SEQUENCER_TRANSITION_GROUP_OTHER					(15)
#define DRUM_SEQUENCER_TRANSITION_OTHER_END						(0)
#define DRUM_SEQUENCER_TRANSITION_REPEAT_LOOP					(0)
#define DRUM_SEQUENCER_TRANSITION_REPEAT_BIG_LOOP				(15)
#define DRUM_SEQUENCER_NEXT_SEQUENCE_END						(0)
#define DRUM_SEQUENCER_SEQUENCE_REPEAT_LOOP					(0)

#define DRUM_SEQUENCER_TRANSITION_START					(255)

#define DRUM_SEQUENCER_CURRENT_RIGHT_POT_UNDEFINED			(-1)
#define CHANNEL_ADD_TO_DRUM_SEQUENCER (-1)		// The default: performance notes just get put into the drum sequencer as normal
#define CHANNEL_PICK (17)		
#define DRUM_SEQUENCER_CHANNEL_DEFAULT_MIDI_OUT (0)			// Performance notes are routed to MIDI_OUT
												// Values 1...16: performance notes are routed to this channel number

#define DRUM_SEQUENCER_NO_MARK					(255)

struct _drumSequencerLocal
    {
    uint8_t format;													// Sequence format (layout).  Since this is also in struct _drumSequencer, maybe we can get rid of it.
	uint8_t numGroups;												// Number of groups in this sequence.  
    uint8_t numTracks;												// Number of tracks in this sequence
    uint8_t numNotes;												// Maximum number of notes per track per group.  To get the *actual* length, use getActualGroupLength(local.drumSequencer.currentGroup())
    uint8_t currentGroup;											// Current group being played/edited in sequence
    uint8_t currentTrack;											// Current track being played/edited in sequence
    uint8_t currentTransition;										// Current transition played in sequence (performance mode only)
    int8_t currentEditPosition;                                     // Where is the edit cursor?  Can be -1, indicating PLAY rather than STEP BY STEP entry mode, or can be >= getActualGroupLength(local.drumSequencer.currentGroup()), indicating "right mode".  We're not using "right mode" right now.
    uint8_t currentPlayPosition;                                    // Where is the play position marker?
    uint8_t transitionGroup[DRUM_SEQUENCER_NUM_TRANSITIONS];		// The current group for each transition.  See earlier notes about how "other" and "end" etc. work.  Since this is also in struct _drumSequencer, maybe we can get rid of it.
    uint8_t transitionRepeat[DRUM_SEQUENCER_NUM_TRANSITIONS];		// The number of times to repeat for each transition.  See earlier notes about how "other" and "end" etc. work.  Since this is also in struct _drumSequencer, maybe we can get rid of it.
    uint8_t repeatSequence;											// How often to repeat the entire sequence after the transitions have been exhausted.  Since this is also in struct _drumSequencer, maybe we can get rid of it.
    uint8_t nextSequence;											// The next sequence after the sequence repeats have been exhausted. Since this is also in struct _drumSequencer, maybe we can get rid of it.
	uint8_t muted[MAX_DRUM_SEQUENCER_TRACKS];						// Whether a given track is muted, and in which mute state.
	uint8_t solo;													// Whether we're in solo mode (the given track is being soloed)
    uint8_t playState;                                              // Is the sequencer playing, paused, or stopped?
    uint8_t shouldPlay[MAX_DRUM_SEQUENCER_TRACKS];					// Should we play the given track [due to pattern]?  Determined when we start note 0 of the sequence, based on the current pattern.  Used throughout the sequence afterwards to determine if we should play or mute the track that time around.
    uint8_t performanceMode;										// Are we in performance mode?
    uint8_t transitionCountdown;									// Current countdown for repeats in the current transition
    uint8_t sequenceCountdown;										// Current countdown for repeats in whole sequence
    uint8_t patternCountup;											// Current "countdown" for the pattern
    uint8_t backup;   												// A temp variable used to backup stuff in TopLevel menus
    uint8_t transitionGroupBackup;									// A second temp variable used to backup stuff in TopLevel menus
    uint8_t transitionOperationBackup;								// A third temp variable used to backup stuff in TopLevel menus
    uint16_t pots[NUM_POTS];										// Previous pot positions.  Used in performance mode: we must exceed these positions by 32 to cause the sequencer to switch to using them as tempo etc.
    int16_t currentRightPot;  										// The previous on-screen position of the right pot.  Used to create a slop that the user must overcome to change positions (so as to prevent jumps due to noise).
    																// I think we only did it for the right pot because of lots of note positions, but with 32 tracks, maybe the left pot should do it too...
    uint8_t returnState;                                            // Used by stateDrumSequencerTransitions and stateDrumSequencerRepeat to determine where to go when cancelled
	int8_t drumRegion;												// 0...3, multiplied against the incoming note to determine which drum to toggle, or negative, indicating a track in play position mode
	uint8_t goNextTransition;										// should we go to the next transition? This value can be FALSE, TRUE (increment the transition), or X >= 2, which means to go directly to transition X - 2
	uint8_t goNextSequence;											// should we go to the next sequence?
    uint8_t scheduleStop;											// We're manually scheduled to stop at the end of this iteration 
    uint8_t markTrack;												// Mark position for the track
    uint8_t markGroup;												// Mark position for the group
    int8_t markPosition;											// Mark position for the step
    uint8_t markTransition;											// Mark for the transitions
    uint8_t invalidNoteSpeed;										// Note speed is not legitimate
    };


// SLOT_DATA_SIZE is 387 = 2 + DRUM_SEQUENCER_NUM_TRANSITIONS (20) + DRUM_SEQUENCER_DATA_LENGTH (365)

struct _drumSequencer
    {
    uint8_t format;								// just the low 4 bits used, high 4 bits unused and free
    uint8_t repeat;								// repeatSequence and nextSequence
    uint8_t transition[DRUM_SEQUENCER_NUM_TRANSITIONS];		// transitionGroup and transitionRepeat
    uint8_t data[DRUM_SEQUENCER_DATA_LENGTH];
    };


// Turns off all notes as appropriate (rests and ties aren't cleared),
// unless clearAbsolutely is true, in which case absolutely everything gets cleared regardless
void clearNotesOnTracks(uint8_t clearEvenIfNoteNotFinished);

// Draws the sequence with the given track length, number of tracks, and skip size
void drawDrumSequencer(uint8_t tracklen, uint8_t numTracks, uint8_t skip);

// Reformats the sequence as requested by the user
void stateDrumSequencerFormat();
void stateDrumSequencerFormatNote();

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


// Additional states
void stateDrumSequencerMIDIChannelOut();
void stateDrumSequencerVelocity();
void stateDrumSequencerPitch();
void stateDrumSequencerGroup();
void stateDrumSequencerGroupLength();
void stateDrumSequencerGroupSpeed();
void stateDrumSequencerTransitionEdit();
void stateDrumSequencerTransitionEditGroup();
void stateDrumSequencerTransitionEditRepeat();
void stateDrumSequencerTransitionEditSpecial();


void packDrumSequenceData();
void unpackDrumSequenceData();

void stateDrumSequencerMenuDefaultVelocity();

void clearCurrentGroup();

void stateDrumSequencerTransitionMark();
void stateDrumSequencerTransition();
void stateDrumSequencerMenuEditMark();
void stateDrumSequencerCopyGroupToNext();
void stateDrumSequencerMenuCopyGroupTrack();
void stateDrumSequencerMenuSwapGroupTracks();
void stateDrumSequencerPitch();
void stateDrumSequencerPitchBack();
void stateDrumSequencerMIDIChannelOut();
void stateDrumSequencerVelocity();
void stateDrumSequencerMenuCopyTrack();
void stateDrumSequencerMenuSwapTracks();
void stateDrumSequencerMenuDistributeTrackInfo();
void stateDrumSequencerMenuAccentTrack();
void stateDrumSequencerGroupLength();
void stateDrumSequencerGroupSpeed();
void stateDrumSequencerMenuCopyGroup();
void stateDrumSequencerMenuSwapGroup();
void stateDrumSequencerTransitionPut();
void stateDrumSequencerTransitionEdit();
void stateDrumSequencerTransitionGoGroup();
void stateDrumSequencerTransitionCopy();
//void stateDrumSequencerTransitionInsert();
void stateDrumSequencerTransitionDelete();
void stateDrumSequencerTransitionEdit();
void stateDrumSequencerTransitionEditGroup();
void stateDrumSequencerTransitionEditRepeat();
void stateDrumSequencerTransitionEditSpecial();
void stateDrumSequencerTransitionMove();


#endif __DRUM_SEQUENCER_H__

