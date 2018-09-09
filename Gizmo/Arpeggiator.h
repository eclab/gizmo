////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License





#ifndef __ARPEGGIATOR_H__
#define __ARPEGGIATOR_H__

#include "All.h"

/////// THE ARPEGGIATOR
//
// The arpeggiator is a basic arpeggiator which can do any of the following:
//
// 1. UP, DOWN, UP-DOWN, RANDOM, ASSIGN, and CHORD arpeggios.  A CHORD arpeggio just plays the chord rapidly.
//    The UP, DOWN, UP-DOWN, and RANDOM arpeggios can be affected by the number of OCTAVES (options.arpeggiatorPlayOctaves)
//    When octaves > 1, the arpeggios will play not only the given chord notes but transposed notes from some number of 
//    octaves above the chord.  The length of the arpeggio is a function of the number of keys being held down.
//
// 2. Ten different user-defined arpeggios.  Each arpeggio can be up to 32 notes or rests long, and can consist of up to
//    15 unique notes ("note" 15 represents rests).  These arpeggios ignore the OCTAVES setting.  An arpeggio is simply
//    a list of these 16 numbers, such as 0, 0, 1, 2, 1, 3, 2, 0, 5, 15 [rest], 4, 8, ... up to 32 in all.
//    User-defined arpeggios also define one of the numbers 0...14 to be the ROOT NOTE: it's assigned to the lowest note
//    in the chord the user is presently holding down.  If for example number 4 is the root note, then numbers BELOW the
//    root note will be played by transposing the chord below the root.
//
// 3. Creation of the user-defined arpeggios.
//
// 4. Arpeggios are affected by SWING, by TEMPO, by NOTE SPEED, and by NOTE LENGTH.
//
// 5. Latching.  If the user engages the latch, then the arpeggiator will continue to play a chord even after the user has
//    let go of the keys.  Only when the user lets go of ALL of they keys and then PLAYS a new key will the arpeggio stop
//    playing (and a new one will be played in its place with the new played keys).
//
// STORAGE
//
// User-defined arpeggios are stored in arpeggio slots 0...9, starting at position ARPEGGIATOR_OFFSET.
// Each arpeggio is 18 bytes long: an arpeggio LENGTH (up to 32), a ROOT NOTE (0...14), and the 32 notes.
// Notes are packed 2 to a byte, and are values 0...15 where 15 is a rest.  When loaded, an arpeggio is put into
// data.arp (a struct _arp).
//
// GLOBALS (TEMPORARY DATA)
//
// Temporary data is stored in local.arp.
//
// OPTIONS
//
// Permanent options special to the Arpeggiator are:
//
// options.arpeggiatorPlayOctaves               How many octaves to play
// options.arpeggiatorLatch                             Is Latch Mode enables
//
// Other permanent options affecting the Arpeggiator include:
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
// DISPLAY
// 
// Arpeggios are displayed in a 7-row by 8 column space on the left LED.  If an arpeggio has <= 7 unique notes,
// each is assigned to its own row, like a piano player roll.  If an arpeggio has > 7 notes, then they are displayed
// in "compressed" form: Note 0 is an LED on the first row, Note 1 is LEDs on the first AND second rows, Note 2
// is an LED on the second row, Note 3 is LEDS on the second AND third rows, and so on.
//
// INTERFACE
//
// Root
//      Arpeggiator                     STATE_ARPEGGIATOR
//              [special controls:]
//                      Back Button: STATE_ARPEGGIATOR_SURE, then STATE_ROOT 
//              Up                              STATE_ARPEGGIATOR_PLAY, local.arp.number = 0
//              Down                    STATE_ARPEGIATOR_PLAY, local.arp.number = 1
//              Up-Down                 ...
//              Random                  ...
//              Chord Repeat    ...
//              0                               STATE_ARPEGIATOR_PLAY, local.arp.number = 5, arpeggio displayed
//              1                               STATE_ARPEGIATOR_PLAY, local.arp.number = 6, arpeggio displayed
//              2                               ...
//              3                               ...
//              4                               ...
//              5                               ...
//              6                               ...
//              7                               ...
//              8                               ...
//              9                               ...
//                      [controls for Up...9:]
//                              Middle Button Pressed: toggle latch mode
//                              Left Knob: scroll through MENU
//                              Select Button Pressed: select MENU option
//                              MENU:
//                                      Octaves:        STATE_ARPEGGIATOR_PLAY_OCTAVES, choose 0...ARPEGGIATOR_MAX_OCTAVES
//                                      Options:        STATE_OPTIONS (display options menu)
//              Create                          STATE_ARPEGGIATOR_CREATE, asks the user to enter the ROOT note, then...
//                      Edit screen             STATE_ARPEGGIATOR_CREATE_EDIT
//                              [controls:]
//                                      Note played: add a note
//                                      Right Knob: scroll cursor back and forth through arpeggiator, new notes start at cursor point
//                                      Back Button: STATE_ARPEGGIATOR_CREATE_SURE, then STATE_ARPEGGIATOR
//                                      Select Button: STATE_ARPEGIATOR_CREATE_SAVE (user chooses a slot to save in), then STATE_ARPEGGIATOR if successful, else STATE_ARPEGGIATOR_CREATE_EDIT
//                                      Middle Button: Add a rest
//                                      Left Knob: scroll through MENU
//                                      Select Button Pressed: select MENU option
//                                      MENU:
//                                              Octaves:        STATE_ARPEGGIATOR_PLAY_OCTAVES, choose 0...ARPEGGIATOR_MAX_OCTAVES
//                                              Options:        STATE_OPTIONS (display options menu)



//// ARPEGGIO CONSTANTS

/// How many notes may be held down at one time?  This should also be the maximum number
/// of unique notes that can be entered during editing.
#define MAX_ARP_CHORD_NOTES 15

// Initial value for local.arp.currentPosition, indicates that we're not
// playing any note right now
#define ARP_POSITION_START (-1)

// How many octaves can we put in local.arp.numOctaves?
#define ARPEGGIATOR_MAX_OCTAVES 6


// Menu selections we have made which indicate what we're doing at a given time.
// Other menu selections can include 4...13 which represent the doing the arpeggios 0...9
#define ARPEGGIATOR_NUMBER_UP 0
#define ARPEGGIATOR_NUMBER_DOWN 1
#define ARPEGGIATOR_NUMBER_UP_DOWN 2
#define ARPEGGIATOR_NUMBER_RANDOM 3
#define ARPEGGIATOR_NUMBER_ASSIGN 4
#define ARPEGGIATOR_NUMBER_CHORD_REPEAT 5
#define ARPEGGIATOR_NUMBER_CREATE 16

// Constants for including a blinky cursor when drawing the arpeggio.
// EDIT_CURSOR_POS puts the cursor just beyond the current arpeggio edit position.
// EDIT_CURSOR_START puts it at 0.  NO_EDIT_CURSOR doesn't draw it at all.
#define EDIT_CURSOR_POS 253
#define EDIT_CURSOR_START 254
#define NO_EDIT_CURSOR 255

#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
#define ARP_POT_SLOP (32)
#define ARPEGGIATOR_PERFORMANCE_MODE_TRANSPOSE (17)
#endif


//// LOCAL 

struct _arpLocal
    {
    uint32_t offTime;                                                   // When should we send the next noteOff?
    uint8_t noteOff;                                                    // What note should be given a noteOff?
    uint8_t steadyNoteOff;                                              // doesn't get erased by a NOTE OFF
    uint8_t number;                                                     // The arpeggio number.  0...4 are ARPEGGIATOR_NUMBER_UP...ARPEGGIATOR_NUMBER_RANDOM, 
    // then we have arpeggios 0..9, then we have ARPEGGIATOR_NUMBER_CREATE
    int8_t currentPosition;                                             // Which note in the arpeggio is being played or edited?  Note that this is signed.  
    // ARP_POSITION_START (-1) indicates "at beginning".
    uint8_t velocity;                                                   // Velocity of the arpeggio playing
    uint8_t chordNotes[MAX_ARP_CHORD_NOTES];    // Notes in the chord being played.  Reused to store notes as they are entered during editing
    uint8_t numChordNotes;                                              // num notes in chordNotes
    uint8_t goingDown;                                                  // Are we descending in the up/down arpeggio style?
    uint8_t playing;                                                    // Am I in a state where adding/removing notes is reasonable?
    uint8_t lastVelocity;                                               // Stores the most recent velocity with which a note was entered during editing, so when we scroll back we have a reasonable velocity to play
    uint8_t currentRightPot;                                        	// What is the most recent right pot value for editing (-1 ... 32 or so).  
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
    uint8_t backup;      		// used for backing up data to restore it                                                           // used to back up various values when the user cancels
	uint8_t performanceMode;
	int8_t transpose;
	uint8_t transposeRoot;
	uint16_t oldLeftPot;
	uint16_t oldRightPot;
#endif
    // We have to jump by at least 2 to start scrolling -- this is an anti-noise measure
    };
        
//// DATA

// Maximum length of an arpeggio
#define MAX_ARP_NOTES 32

// There are 14 unique notes in an arpeggio.  Rests are note 15, and ties are note 14.
#define ARP_REST        15
#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
#define ARP_TIE        	14
#endif
	
// Notes are packed 2 to a byte.  This gets the first one
#define ARP_NOTE0(note)   ((note) & 15)
// This gets the second one
#define ARP_NOTE1(note)   ((note) >> 4)
// This gets a note at index idx in the array
#define ARP_NOTEX(idx)    (((idx) & 1) ? ARP_NOTE1(data.arp.notes[(idx) >> 1]) : ARP_NOTE0(data.arp.notes[(idx) >> 1]))
// This packs two notes together into a byte
#define PACK_ARP_NOTES(note0, note1)   (((note0) & 15) | ((note1) << 4))
// This revises a note at index idx in the array
#define SET_ARP_NOTEX(idx, val)  (data.arp.notes[(idx) >> 1] = (((idx) & 1) ? PACK_ARP_NOTES(ARP_NOTE0(data.arp.notes[(idx) >> 1]), (val)) : PACK_ARP_NOTES((val), ARP_NOTE1(data.arp.notes[(idx) >> 1]))))



struct _arp
    {
    uint8_t length;                                                                     // How long is the arpeggio?  (up to MAX_ARP_NOTES)
    uint8_t root;                                                                       // What note (0...14) in the arpeggio corresponds to the root of the played chord?
    uint8_t notes[MAX_ARP_NOTES >> 1];              // half of MAX_ARP_NOTES of course
    };






// Starting at position pos, draws up to next SEVEN notes of the arpeggio.
// We leave a one-column space so as not to interfere with the right LED matrix.
// The edit cursor is also drawn, if it is EDIT_CURSOR_POS or EDIT_CURSOR_START
void drawArpeggio(uint8_t* mat, uint8_t pos, uint8_t editCursor, uint8_t len = 7);

// Continue to play the arpeggio
void playArpeggio();

// Load an arpeggio
#define LOAD_ARPEGGIO(index) (loadData((char*)(&(data.arp)), sizeof(struct _arp) * (index)  + (ARPEGGIATOR_OFFSET), sizeof(struct _arp)))

// Save an arpeggio
#define SAVE_ARPEGGIO(index) (saveData((char*)(&(data.arp)), sizeof(struct _arp) * (index)  + (ARPEGGIATOR_OFFSET), sizeof(struct _arp)))

// Is the arpeggio slot empty?  To do this we read the length to see if it's nonzero.  Length is the first byte in _arp
#define ARPEGGIO_IS_NONEMPTY(index) (EEPROM.read((ARPEGGIATOR_OFFSET) +  (index) * sizeof(struct _arp)))

// Remove a note from chordNotes, or mark it if latch mode is on.  O(n) :-(
void arpeggiatorRemoveNote(uint8_t note);

// Add a note to chordNotes
void arpeggiatorAddNote(uint8_t note, uint8_t velocity);

// Choose an arpeggiation, or to create one
void stateArpeggiator();

// Handle the menu structure for playing an arpeggio
void stateArpeggiatorPlay();

// Handle the screen for creating an arpeggio.  This first chooses the root.
// Inlined at TopLevel to save space
//void stateArpeggiatorCreate();

// Handle the screen for editing an arpeggio.
void stateArpeggiatorCreateEdit();

// Handle the screen for saving an arpeggio.  
void stateArpeggiatorCreateSave();

void stateArpeggiatorMenu();

#endif

