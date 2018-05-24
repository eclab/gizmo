////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __THRU_H__
#define __THRU_H__



/////// THE THRU FACILITY
//
// The thru facility can do any of the following (including in combination):
//
// 1. Distribute notes coming into options.channelIn and send them, one by one round-robin, to N different MIDI channels
//    starting at options.channelOut.
//
// 2. In response to a note coming into options.channelIn, send the same note out an appropriate MIDI channel
//    some N times in rapid sucession.
//
// OPTIONS
//
// Permanent options special to the Key Splitter are:
//
// options.thruExtraNotes               How many *additional* notes should I send out?
// options.thruNumDistributionChannels  Over how many *additional* channels should I distribute notes?
//
// GLOBALS (TEMPORARY DATA)
//
// Temporary data is stored in local.thru.
//
// Other permanent options affecting the Key Splitter include:
//
// options.channelIn
// options.channelOut
// options.channelControl
// options.transpose                            // only affects notes coming into channelIn
// options.volume                                       // only affects notes coming into channelIn
//
// DISPLAY
// 
// When running the Thru facility, you will see the word PLAY.
//
// INTERFACE
//
// Root
//      Thru                    STATE_THRU
//              [special controls:]
//                                              Back Button:    STATE_ROOT 
//                              Go                                              STATE_THRU_PLAY
//                              Extra Notes:                    STATE_THRU_EXTRA_NOTES
//                              Distribute Notes:               STATE_THRU_DISTRIBUTE_NOTES

#define MAX_CHORD_MEMORY_NOTES (8)

#define DEBOUNCE_STATE_OFF 0						// No note is being played
#define DEBOUNCE_STATE_FIRST_NOTE_DOWN 1			// A note is being played
#define DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED 2		// A note was let up too soon, we're still playing it for a bit

struct _thruLocal
    {
    uint8_t chordMemory[MAX_CHORD_MEMORY_NOTES];
    uint8_t distributionNotes[16];
    uint8_t currentDistributionChannelIndex;
    uint8_t debounceState;
    uint32_t debounceTime;
    uint8_t debounceNote;
    };


void stateThruPlay();
void playThru();


#endif // __THRU_H__
