////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



// This appears first so it can be accessed in recursive fashion by
// Recorder.h, StepSequencer.h, and Arpeggiator.h
#define SLOT_DATA_SIZE 387



#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "All.h"



////// SLOT MEMORY STORAGE COMPUTATION

//// Slots are of SLOT_SIZE (388).  The Uno can hold two of them and the Mega nine.
//// Slots consist of a one-byte SLOT TYPE, followed by 387 bytes of SLOT DATA.
//// Slots come first in the flash memory, so SLOT_OFFSET is zero.  Next comes
//// the arpeggios.  There are ten of them, each of size 18, starting at ARPEGGIATOR_OFFSET.
//// Finally comes the options struct.  This starts at OPTIONS_OFFSET.
//// The Mega has space for a 424-byte options struct.  The Uno has space for 68 bytes.

#if defined(__MEGA__)
#define NUM_SLOTS 9
#endif
#if defined(__UNO__)
#define NUM_SLOTS 2
#endif

#define SLOT_OFFSET 0
#define SLOT_SIZE ((SLOT_DATA_SIZE) + 1)
#define ARPEGGIATOR_OFFSET      ((SLOT_OFFSET) + (NUM_SLOTS) * (SLOT_SIZE))
#define NUM_ARPS 10
#define OPTIONS_OFFSET  ((ARPEGGIATOR_OFFSET) + ((NUM_ARPS) * sizeof(struct _arp)))



////// SLOTS
////// A slot has a TYPE, which is the index of a single 3x5 glyph.  This glyph
////// will be displayed when you load slots, and also serves to indicate what
////// elements are stored in a given slot.  Following this a slot contains a union
////// of 386 bytes whose layout is specified by the given application.

#define SLOT_TYPE_EMPTY (GLYPH_3x5_BLANK)
#define SLOT_TYPE_STEP_SEQUENCER (GLYPH_3x5_S)
#define SLOT_TYPE_RECORDER (GLYPH_3x5_R)
#define SLOT_TYPE_DRUM_SEQUENCER (GLYPH_3x5_D)


// Given an application state (presently STATE_STEP_SEQUENCER, STATE_DRUM_SEQUENCER or STATE_RECORDER),
// returns the appropriate slot (SLOT_TYPE_STEP_SEQUENCER, SLOT_DRUM_SEQUENCER, or SLOT_TYPE_RECORDER)
uint8_t slotTypeForApplication(uint8_t application);



struct _slot
    {
    uint8_t type;                       // Slot type, one of SLOT_TYPE_EMPTY, SLOT_TYPE_STEP_SEQUENCER, SLOT_TYPE_DRUM_SEQUENCER, or SLOT_TYPE_RECORDER
    union
        {
        struct _recorder recorder;
        struct _stepSequencer stepSequencer;
        struct _drumSequencer drumSequencer;
        // This is a union so we can restructure it any way we like
        char buffer[SLOT_DATA_SIZE];             
        } data;
    };


////// LOADED DATA
////// Depending on the application, available data is loaded from a SLOT or from
////// the ARPEGGIATION REGION.  See Arpeggiator.h for information on the arpeggiator
////// data format.

union _data
    {
    struct _slot slot;
    struct _arp arp;
    unsigned char bytes[sizeof(struct _slot)];		// for sysex to get the bytes out
    };
        
// Our loaded data
extern union _data data;
    

/// LOAD DATA
/// Loads data from Flash starting at the given position and of the given length.
void loadData(char* data, uint16_t position, uint16_t length);

/// SAVE DATA
/// Saves data to Flash starting at the given position and of the given length.
void saveData(char* data, uint16_t position, uint16_t length);

/// LOAD SLOT
/// Loads a slot of the given index.
void loadSlot(uint8_t index);

/// SAVE SLOT
/// Saves a slot of the given index.
void saveSlot(uint8_t index);

/// GET SLOT TYPE
/// Returns the type of a given slot.
uint8_t getSlotType(uint8_t index);


    
#endif

