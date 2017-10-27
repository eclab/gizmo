////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


////// UTILITY
//////
////// Utility.h/.cpp define various utility functions used by a variety of applications
////// 

#ifndef __UTILITY_H__
#define __UTILITY_H__





/// MENU, NUMERICAL, AND GLYPH DISPLAYS
/// All display methods set currentDisplay to a number corresponding to the item the
/// user has chosen, or is presently choosing.
extern int16_t currentDisplay;                     // currently displayed menu item

// Divide this into the pot[LEFT_POT] to determine what the menu state should be.
// If negative, then MULTIPLY pot[LEFT_POT] by -potDivisor to determine what the state should be.
extern int16_t potDivisor;

///// DOMENUDISPLAY()
//
// This function updates a display in the form of a menu of
// scrolling text items.  The user provides up to MAX_MENU_ITEMS
// text items, and doMenuDisplay does the rest.
//
// If menu is non-NULL, then doMenuDisplay will
// set up the menu items.  Thereafter (when the menu is NULL) it 
// ignores what's passed into it and just lets the user choose and scroll them.
//
// The user can use doMenuDisplay() in two ways:
// 
// 1. To choose among different STATES.  Here in addition to the
//    menu items and their length, the user provides a BASE STATE
//    (the state corresponding to menu item 0), a DEFAULT STATE
//    (the state that should be first shown -- corresponds to
//    menu item DEFAULT STATE - BASE STATE), and a BACK STATE
//    (the state that should be transitioned to if the user presses
//    the back button -- corresponds to the menu item
//    BACK STATE - BASE STATE.  In this case the system will
//    automatically choose a state and transition to it when the
//    user either presses the select or back buttons.
//
// 2. To simply indicate what menu item was provided.  Here, the
//    user passes in STATE_NONE as baseState, and passes in a DEFAULT
//    MENU ITEM (not STATE) for defaultState.  backState is entirely
//    ignored.  As the user is scrolling through various options,
//    doMenuDisplay will return NO_MENU_SELECTED, but you can see which
//    item is being considered as currentDisplay.  If the user selects a
//    menu item, then MENU_SELECTED is set (again the item
//    is currentDisplay).  Finally if the user goes back, then
//    MENU_CANCELLED is returned.

/// These are values potentially returned by doMenuDisplay
#define NO_MENU_SELECTED 100
#define MENU_SELECTED 101
#define MENU_CANCELLED 102

#define MAX_MENU_ITEMS 17
#define MAX_MENU_ITEM_LENGTH 30

uint8_t doMenuDisplay(const char** _menu, uint8_t menuLen, uint8_t baseState, uint8_t backState, uint8_t scrollMenu, uint8_t extra = 0);



///// DONUMERICALDISPLAY()
//
// This function updates a display in the form of an on-screen number
// ranging between a MIN VALUE and a MAX VALUE, and starting at a given
// DEFAULT VALUE.   The user can choose from these values, or cancel
// the operation.
//
// To do this, you call doNumericalDisplay() multiple times until the
// user either picks a value or cancels.  The first time this function 
// is called, you should first set the global variable 'entry' to TRUE.
// This will cause doNumericalDisplay to set up its display.  
// Thereafter you should call 'entry' to FALSE until the user has chosen
// a value or has cancelled.
//
// As the user is scrolling through various options,
// doNumericalDisplay will return NO_MENU_SELECTED, but you can see which
// item is being considered in the global variable 'currentDisplay'.  
// If the user selects an item, then MENU_SELECTED is set (again the item
// is 'currentDisplay').  Finally if the user cancels, then MENU_CANCELLED
// is returned (and 'currentDisplay' is undefined).
//
// If includeOff is TRUE, then instead of displaying the MINIMUM VALUE,
// doNumericalDisplay will display the text "- - - -", suggesting "OFF"
// or "NONE" to the user.
// 
// If includeOther is set a value other than GLYPH_OTHER or GLYPH_NONE,
// then instead of displaying the MAXIMUM
// VALUE, doNumericalDisplay will display the indicated glyph. 
//
// if includeOther is instead set to GLYPH_NONE, then
// the maximum value will be displayed as normal.
//
// If includeOther is set to GLYPH_OTHER, then *all* of the values
// will be replaced with single (3x5) glyphs in the glyph font (see LEDDisplay.h)
// whose index is specified by you the global array 'glyphs'. Specifically,
// the glyph displayed for value X will be glyphs[x - minimumValue].  Be warned
// that this array is MAX_GLYPHS in length, so minimumValue - maximumValue + 1
// must be <= MAX_GLYPHS if you want to use this option.
//
// If either a glyph (due to GLYPH_OTHER) is being displayed, or a number
// is currently being displayed, you can also choose to display an additional
// 3x5 glyph at the far left side of the screen.  To do this, you put the glyph
// index in the global variable 'secondGlyph'.  You must set this global variable
// every time the function is called, as it is immediately reset to NO_GLYPH
// afterwards.

#define MAX_GLYPHS 11
extern uint8_t glyphs[MAX_GLYPHS];
extern uint8_t secondGlyph;

uint8_t doNumericalDisplay(int16_t minValue, int16_t maxValue, int16_t defaultValue, uint8_t includeOff, uint8_t includeOther);


// STATE NUMERICAL
// This function performs the basic functions for a state whose entire purpose is to compute a numerical value
// and set *value* to that value.  If updateOptions is true, then stateNumerical will recover from a cancel by
// overwriting the options with the backup options. Otherwise it will recover by overwriting value with backupValue.  

#define NO_STATE_NUMERICAL_CHANGE 255

extern uint8_t stateNumerical(uint8_t start, uint8_t end, uint8_t &value, uint8_t &backupValue,
    uint8_t updateOptions, uint8_t includeOff, uint8_t other, uint8_t backState);

///// DOGLYPHDISPLAY()
//
// This function updates a display to one of several glyphs, while 
// optionally updating the alternate display to a specific glyph.
// The glyphs are given as an array of GLYPH NUMBERS.  A glyph number
// is the glyph constant (such as GLYPH_4x5_NEGATIVE_6) 
// plus the font constant (such as FONT_4x5).  Presently FONT_5x5_ALPHABET is unsupported.
//
// If glyph array is non-NULL, then doGlyphDisplay will
// set up the menu items.  Thereafter (when the array is NULL) it 
// ignores what's passed into it and just lets the user choose and scroll them.
//
// As the user is scrolling through various options,
// doGlyphDisplay will return NO_MENU_SELECTED, but you can see which
// item is being considered as currentDisplay.  If the user selects a
// menu item, then MENU_SELECTED is set (again the item
// is currentDisplay).  Finally if the user goes back, then
// MENU_CANCELLED is returned.


// Font constants.  NO_GLYPH indicates that no glyph is to be displayed (why bother?)
#define FONT_3x5        0
#define FONT_4x5        64
#define FONT_8x5        128
#define NO_GLYPH        192

uint8_t doGlyphDisplay(const uint8_t* _glyphs, uint8_t numGlyphs, const uint8_t otherGlyph, int16_t defaultValue);







///// DEBUGGING FACILITY
/////
///// When using MIDI you can't print out to Serial.  But you CAN print to the
///// LED matrix.  These functions will do this.

/// Print an integer (-9999 to 19999) to the LED, and pause for 1/4 sec 
uint8_t debug(int16_t val);

/// Print a maximum TWO-character string to the LED, along with an integer from -99...127, and pause for 1/4 sec 
uint8_t debug(const char* str, int8_t val);

/// Print two integers, each from -99...127, and pause for 1/4 sec 
uint8_t debug(int8_t val1, int8_t val2);



///// NOTES

/// Indicates no note (in various contexts)
#define NO_NOTE 128

// perform a click track
void doClick();





///// DRAWING SUPPORT

//// Draws a MIDI channel value of the bottom row of the left LED, using drawRange(...)
void drawMIDIChannel(uint8_t channel);

//// GLYPHS available to WRITE 3x5 GLYPHS, which writes 4 glyphs filling the whole screen
#define GLYPH_OFF 0                                    	// ----
#define GLYPH_OMNI 1                                    // ALLC
#define GLYPH_DEFAULT 2                                 // DFLT
#define GLYPH_DECREMENT 3                               // DECR
#define GLYPH_INCREMENT 4                               // INCR
#define GLYPH_FREE 5                                    // FREE
#define GLYPH_NOTE 6                                    // NOTE
#define GLYPH_SYSEX 7                                   // SYSX
#define GLYPH_SONG_POSITION 8                           // SPOS
#define GLYPH_SONG_SELECT 9                             // SSEL
#define GLYPH_TUNE_REQUEST 10                           // TREQ
#define GLYPH_START 11                                  // STRT
#define GLYPH_CONTINUE 12                               // CONT
#define GLYPH_STOP 13                                   // STOP
#define GLYPH_SYSTEM_RESET 14                           // RSET

#ifdef INCLUDE_EXTENDED_GLYPH_TABLE
#define GLYPH_FADE 15									// FADE
#define GLYPH_PLAY 16									// PLAY
#define GLYPH_CHORD 17									// CHRD
#define GLYPH_HIGH 18									// HIGH
#define GLYPH_TRANSPOSE 19								// TRAN
#endif

#define GLYPH_OTHER (254)	// reserved to represent "use this glyph instead"
#define GLYPH_NONE	(255)	// reserved to represent "no glyph"

// Writes any of the above glyph sets to the screen
void write3x5Glyphs(uint8_t index);

// Clears the screen buffers.
void clearScreen();




#ifdef INCLUDE_OPTIONS_MENU_DELAY
// SET MENU DELAY
// Changes the menu delay to a desired value (between 0: no menu delay, and 11: infinite menu delay).  The default is 5
void setMenuDelay(uint8_t index);

#endif

// The index values passed into setMenuDelay correspond to the following delays (but these
// constants may NOT be passed into setMenuDelay).

#define DEFAULT_SHORT_DELAY (60 >> 3)
#define NO_MENU_DELAY  (DEFAULT_SHORT_DELAY)
#define EIGHTH_MENU_DELAY (109 >> 3)
#define QUARTER_MENU_DELAY (205 >> 3)
#define THIRD_MENU_DELAY (269 >> 3)
#define HALF_MENU_DELAY (397 >> 3)
#define DEFAULT_MENU_DELAY (781 >> 3)
#define DOUBLE_MENU_DELAY (1549 >> 3)
#define TREBLE_MENU_DELAY (2317 >> 3)
#define QUADRUPLE_MENU_DELAY (3085 >> 3)
#define EIGHT_TIMES_MENU_DELAY (6157 >> 3)
#define HIGH_MENU_DELAY  (NO_SCROLLING)







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


///// Call this repeatedly from your notional "please enter a note" state to query the user about what note he'd like.
///// The value returned is either NO_NOTE, indicating that the user has not entered a note yet,
///// or it is a note pitch, which the user has chosen.  The velocity of the note is stored in
///// the global variable stateEnterNoteVelocity.  
///// backState                 where we should go after the user has cancelled                         
extern uint8_t stateEnterNoteVelocity;
uint8_t stateEnterNote(uint8_t backState);


#ifdef INCLUDE_ENTER_CHORD
///// Call this repeatedly from your notional "please enter a chord" state to query the user about what chord he'd like.
///// You pass in an array CHORD of size MAXCHORDNOTES.
///// The value returned is either NO_NOTE, indicating that the user has not entered a (full) chord yet,
///// or it is the number of notes in the chord the user played (up to MAXCHORDNOTES).  This value will never
///// be ZERO.  The velocity of the LAST NEW note entered is stored in the global variable 
///// stateEnterNoteVelocity.  The notes are stored, sorted, in CHORD.  Note that if the user cancels his 
///// chord (that is, we go to backState), the values in the chord array will be undefined.
///// chord                     Notes are stored here.
///// backState                 where we should go after the user has cancelled                         
uint8_t stateEnterChord(uint8_t* chord, uint8_t maxChordNotes, uint8_t backState);
#endif


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

#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
uint8_t gatherByte(uint16_t pos);		// used by step sequencer also
void stripHighBits();
#endif


#endif __UTILITY_H__
