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



// Loads the current options from flash.
void loadOptions() 
    { 
    loadData((char*)(&options), OPTIONS_OFFSET, sizeof(options));
    setPulseRate(options.tempo);
    setNotePulseRate(options.noteSpeedType);
    setScreenBrightness(options.screenBrightness);
#if defined(__AVR_ATmega2560__)
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
    options.volume = 3;  // corresponds to no volume modification
    options.click = NO_NOTE;
#if defined(__AVR_ATmega2560__)
    options.menuDelay = 6;  // corresponds to DEFAULT_MENU_DELAY
    options.splitChannel = 1;
	options.splitNote = 60;  // Middle C
#endif
    }


