////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License
//////
////// UTILITY
//////
////// Utility.h/.cpp define various utility functions used by a variety of applications
////// 

#ifndef __UTILITY_H__
#define __UTILITY_H__





///// DEBUGGING FACILITY
/////
///// When using MIDI you can't print out to Serial.  But you CAN print to the
///// LED matrix.  These functions will do this.

/// Print an integer (-9999 to 19999) to the LED, and pause for 1/4 sec 
void debug(int val);

/// Print a maximum TWO-character string to the LED, along with an integer from -99...127, and pause for 1/4 sec 
void debug(const char* str, int val);

/// Print two integers, each from -99...127, and pause for 1/4 sec 
void debug(uint8_t val1, uint8_t val2);



///// NOTES

/// Indicates no note (in various contexts)
#define NO_NOTE 128

// perform a click track
void doClick();




///// DRAWING SUPPORT

//// Draws a MIDI channel value of the bottom row of the left LED, using drawRange(...)
void drawMIDIChannel(uint8_t channel);

//// GLYPHS available to WRITE 3x5 GLYPHS, which writes 4 glyphs filling the whole screen
#define GLYPH_NONE 0                                    // ----
#define GLYPH_OMNI 1                                    // ALLC
#define GLYPH_DEFAULT 2                                 // DFLT
#define GLYPH_NOTE 3                                    // NOTE
#define GLYPH_SYSEX 4                                   // SYSX
#define GLYPH_SONG_POSITION 5                   // SPOS
#define GLYPH_SONG_SELECT 6                             // SSEL
#define GLYPH_TUNE_REQUEST 7                    // TREQ
#define GLYPH_START 8                                   // STRT
#define GLYPH_CONTINUE 9                                // CONT
#define GLYPH_STOP 10                                   // STOP
#define GLYPH_SYSTEM_RESET 11                   // RSET
#define GLYPH_ROOT 12                                   // ROOT
#define GLYPH_DECREMENT 13                              // DECR
#define GLYPH_INCREMENT 14                              // INCR

// Writes any of the above glyph sets to the screen
void write3x5Glyphs(uint8_t index);

// Clears the screen buffers.
void clearScreen();


///// STATE UTILITIES

///// Call this repeatedly from your SAVE state to query the user to save the current data.
///// backState          where we should go after the user has cancelled or saved.
void stateSave(uint8_t backState);


///// Call this repeatedly from your LOAD state to query the user to load the current data,
/////     or optionally to initialize the data.  You may want to customize this method to add
/////     your own initialization or unpacking code.
///// selectedState     where we should go after the user has loaded.
///// initState                 where we should go after the user has initialized new (not loaded).
///// backState                 where we should go after the user has cancelled.
///// defaultState              when we cancel and go back, what should we indicate as the default state?
void stateLoad(uint8_t selectedState, uint8_t initState, uint8_t backState, uint8_t defaultState);


///// Call this repeatedly from your SURE? state to query the user about whether he really wants
///// to do something (typically to cancel via clicking the back button)
///// selectedState             where we should go after the user has pressed the SELECT button,
/////                                   typically indicating that he DOESN'T want to go back.
///// backState                 where we should go after the user has pressed the BACK button,
/////                                   typically indicating that he WANTS to go back.
void stateSure(uint8_t selectedState, uint8_t backState);


///// Call this repeatedly from your ENTER NOTE state to query the user about what note he'd like.
///// You can provide GLYPHS, which indicates a glyph set to write to the screen via write3x5Glyphs(...) 
///// The value returned is either NO_NOTE, indicating that the user has not entered a note yet,
///// or it is a note pitch, which the user has chosen.  The velocity of the note is stored in
///// the global variable stateEnterNoteVelocity.  
///// glyphs                    glyph set to display.
///// backState                 where we should go after the user has cancelled                         
extern uint8_t stateEnterNoteVelocity;
uint8_t stateEnterNote(uint8_t glyphs, uint8_t backState);


///// Increments playing notes from an application (such as a step sequencer or arpeggiator).
///// This method is used in various states to keep playing even while wandering through a menu etc. 
void playApplication();


//// Goes to the given state, setting entry to true and no default state
void goDownState(uint8_t nextState);


//// Goes to the given state, setting entry to true and no default state,
//// and ALSO backs up options.
void goDownStateWithBackup(uint8_t _nextState);


//// Goes to the given state, setting entry to true and the default state to the current state
void goUpState(uint8_t nextState);


//// Goes to the given state, setting entry to true and the default state to the current state,
//// and ALSO backs up options.
void goUpStateWithBackup(uint8_t _nextState);



///// GENERAL UTILITIES

///// bounds n to between min and max inclusive, returning the result
uint8_t bound(uint8_t n, uint8_t min, uint8_t max);

///// increments n, then if it EQUALS or EXCEEDS max, sets it to 0, returning the result
uint8_t incrementAndWrap(uint8_t n, uint8_t max);

/// Computes the median of three values.
#define MEDIAN_OF_THREE(a,b,c) (((a) <= (b)) ? (((b) <= (c)) ? (b) : (((a) < (c)) ? (c) : (a))) : (((a) <= (c)) ? (a) : (((b) < (c)) ? (c) : (b))))

/// Computes the median of five values.  [Presently unused]
uint16_t medianOfFive(uint16_t array[]);




#endif __UTILITY_H__
