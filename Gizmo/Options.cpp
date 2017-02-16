////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


//// IMPORTANT NOTE:
//// All of the option states are hard-coded into the state machine switch
//// statement in TopLevel.cpp as a memory-saving measure.
////
//// So all you have here are utility functions for options.


#include "All.h"

// The options struct which is saved and loaded and used
GLOBAL struct _options options;

// Before modifying the options struct, you should back it up here.
// Then if you want to revert (cancel) the changes the user has made,
// You only need to set options = backupOptions
GLOBAL struct _options backupOptions;

// last timestamp for time tempo
uint32_t lastTempoTapTime;



// Loads the current options from flash.
void loadOptions() 
    { 
    loadData((char*)(&options), OPTIONS_OFFSET, sizeof(options));
    setPulseRate(options.tempo);
    setNotePulseRate(options.noteSpeedType);
    setScreenBrightness(options.screenBrightness);
#ifdef INCLUDE_OPTIONS_MENU_DELAY
    setMenuDelay(options.menuDelay);
#endif
    }


// Saves the current options to flash.
void saveOptions() 
    { 
    saveData((char*)(&options), OPTIONS_OFFSET, sizeof(options));
    backupOptions = options;
    }



// resets the current options in memory to default values(does not save them).
void resetOptions()
    {
    // zero everything
    memset(&options, 0, sizeof(struct _options));
    
    // now just set the ones that aren't zero
    options.screenBrightness = 3;  // not too dim, not mind-numbingly bright
    options.tempo = 120;
    options.noteSpeedType = NOTE_SPEED_SIXTEENTH;  // default.  This also allows swing
    options.channelIn = 1;
    options.channelOut = 1;
    options.noteLength = 100;
    options.click = NO_NOTE;
    options.clock = IGNORE_MIDI_CLOCK;

#ifdef INCLUDE_ARPEGGIATOR
    options.arpeggiatorPlayVelocity = 128;  // FREE
#endif

#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
    options.clockDivisor = 1;
#endif

#ifdef INCLUDE_OPTIONS_MENU_DELAY
    options.menuDelay = 5;  // corresponds to DEFAULT_MENU_DELAY
#endif

#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
    options.volume = 3;  // corresponds to no volume modification
#endif

#ifdef INCLUDE_EXTENDED_RECORDER
    options.recorderRepeat = true;
#endif

#ifdef INCLUDE_SPLIT
    options.splitChannel = 1;
    options.splitNote = 60;  // Middle C
    options.splitLayerNote = NO_NOTE;
#endif

#ifdef INCLUDE_MEASURE
    options.measureBeatsPerBar = 4;
    options.measureBarsPerPhrase = 4;
#endif

#if defined(HEADLESS_RESET)
    options.channelControl = 16;
#endif 
    }


