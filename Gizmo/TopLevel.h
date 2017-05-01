////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#ifndef __TOPLEVEL_H__
#define __TOPLEVEL_H__

#include "All.h"



//// THE STATE MACHINE

// These are the states in the overall state machine

typedef enum _State: uint8_t
	{
	STATE_NONE = 255,
	STATE_ROOT = 0,

#ifdef INCLUDE_ARPEGGIATOR
	STATE_ARPEGGIATOR,
#endif

#ifdef INCLUDE_STEP_SEQUENCER
	STATE_STEP_SEQUENCER,
#endif

#ifdef INCLUDE_RECORDER
	STATE_RECORDER,
#endif

#ifdef INCLUDE_GAUGE
	STATE_GAUGE,
#endif

#ifdef INCLUDE_CONTROLLER
	STATE_CONTROLLER,
#endif

#ifdef INCLUDE_SPLIT
	STATE_SPLIT,
#endif

#ifdef INCLUDE_THRU
	STATE_THRU,
#endif

#ifdef INCLUDE_SYNTH
	STATE_SYNTH,
#endif

#ifdef INCLUDE_MEASURE
	STATE_MEASURE,
#endif

	STATE_OPTIONS,
	STATE_UNDEFINED_1,	// leave all these alone, they're buffer space for 12 apps
	STATE_UNDEFINED_2,
	STATE_UNDEFINED_3,
	STATE_UNDEFINED_4,			   
	STATE_UNDEFINED_5,	           
	STATE_UNDEFINED_6,		       
	STATE_UNDEFINED_7,
	STATE_UNDEFINED_8,
	STATE_UNDEFINED_9,
	STATE_UNDEFINED_10,			   
	STATE_UNDEFINED_11,	           
	STATE_UNDEFINED_12,		 

#ifdef INCLUDE_ARPEGGIATOR
	STATE_ARPEGGIATOR_PLAY,			
	STATE_ARPEGGIATOR_PLAY_OCTAVES,
	STATE_ARPEGGIATOR_PLAY_VELOCITY,
	STATE_ARPEGGIATOR_MENU,
	STATE_ARPEGGIATOR_CREATE,
	STATE_ARPEGGIATOR_CREATE_EDIT,
	STATE_ARPEGGIATOR_CREATE_SAVE,
	STATE_ARPEGGIATOR_CREATE_SURE,
#endif

#ifdef INCLUDE_STEP_SEQUENCER
	STATE_STEP_SEQUENCER_FORMAT,
	STATE_STEP_SEQUENCER_PLAY,
	STATE_STEP_SEQUENCER_MENU,
	STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT,
	STATE_STEP_SEQUENCER_VELOCITY,
	STATE_STEP_SEQUENCER_FADER,
	STATE_STEP_SEQUENCER_LENGTH,
	STATE_STEP_SEQUENCER_SURE,
	STATE_STEP_SEQUENCER_SAVE,
#endif

#ifdef 	INCLUDE_EXTENDED_STEP_SEQUENCER
	STATE_STEP_SEQUENCER_MENU_TYPE,
	STATE_STEP_SEQUENCER_MENU_TYPE_PARAMETER,
#endif


#ifdef INCLUDE_RECORDER
	STATE_RECORDER_FORMAT,
	STATE_RECORDER_PLAY,
	STATE_RECORDER_SAVE,
	STATE_RECORDER_SURE,
#endif

#ifdef INCLUDE_EXTENDED_RECORDER
	STATE_RECORDER_MENU,
#endif


#ifdef INCLUDE_CONTROLLER
	STATE_CONTROLLER_PLAY,
	STATE_CONTROLLER_SET_LEFT_KNOB_TYPE,
	STATE_CONTROLLER_SET_RIGHT_KNOB_TYPE,
	STATE_CONTROLLER_SET_MIDDLE_BUTTON_TYPE,
	STATE_CONTROLLER_SET_SELECT_BUTTON_TYPE,
#endif

#ifdef INCLUDE_EXTENDED_CONTROLLER
	STATE_CONTROLLER_MODULATION,
	STATE_CONTROLLER_PLAY_WAVE_ENVELOPE,
	STATE_CONTROLLER_SET_WAVE_TYPE,
	STATE_CONTROLLER_SET_WAVE_ENVELOPE,
	STATE_CONTROLLER_MODULATION_SET_MODE,
	STATE_CONTROLLER_MODULATION_SET_CLOCK,
	STATE_CONTROLLER_SET_WAVE_ENVELOPE_VALUE,
	STATE_CONTROLLER_PLAY_WAVE_NUMBER,
#endif

#ifdef INCLUDE_CONTROLLER
	STATE_CONTROLLER_SET_LEFT_KNOB_NUMBER,
	STATE_CONTROLLER_SET_RIGHT_KNOB_NUMBER,
	STATE_CONTROLLER_SET_MIDDLE_BUTTON_NUMBER,
	STATE_CONTROLLER_SET_SELECT_BUTTON_NUMBER,
	STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_ON,
	STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON,
	STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF,
	STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF,
#endif

	STATE_OPTIONS_TEMPO,
	STATE_OPTIONS_NOTE_SPEED,
	STATE_OPTIONS_SWING,

#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
	STATE_OPTIONS_TRANSPOSE,
	STATE_OPTIONS_VOLUME,
#endif

	STATE_OPTIONS_PLAY_LENGTH,
	STATE_OPTIONS_MIDI_CHANNEL_IN,
	STATE_OPTIONS_MIDI_CHANNEL_OUT,
	STATE_OPTIONS_MIDI_CHANNEL_CONTROL,
	STATE_OPTIONS_MIDI_CLOCK,
	
#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
	STATE_OPTIONS_MIDI_CLOCK_DIVIDE,
#endif

	STATE_OPTIONS_CLICK,
	STATE_OPTIONS_SCREEN_BRIGHTNESS,

#ifdef INCLUDE_OPTIONS_MENU_DELAY
	STATE_OPTIONS_MENU_DELAY,
#endif

#ifdef INCLUDE_VOLTAGE
	STATE_OPTIONS_VOLTAGE,
#endif

	STATE_OPTIONS_ABOUT,

#ifdef INCLUDE_SPLIT
	STATE_SPLIT_CHANNEL,
	STATE_SPLIT_NOTE,
	STATE_SPLIT_LAYER_NOTE,
#endif

#ifdef INCLUDE_THRU
	STATE_THRU_PLAY,
	STATE_THRU_EXTRA_NOTES,
	STATE_THRU_DISTRIBUTE_NOTES,
	STATE_THRU_CHORD_MEMORY,
	STATE_THRU_DEBOUNCE,
	STATE_THRU_MERGE_CHANNEL_IN,
	STATE_THRU_CC_NRPN,
	STATE_THRU_BLOCK_OTHER_CHANNELS,
#endif

#ifdef INCLUDE_MEASURE
	STATE_MEASURE_MENU,
	STATE_MEASURE_BEATS_PER_BAR,
	STATE_MEASURE_BARS_PER_PHRASE,
#endif

#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD
	STATE_SYNTH_WALDORF_BLOFELD,
#endif
#ifdef INCLUDE_SYNTH_KAWAI_K4
	STATE_SYNTH_KAWAI_K4,
#endif
#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
	STATE_SYNTH_OBERHEIM_MATRIX_1000,
#endif
#ifdef INCLUDE_SYNTH_KORG_MICROSAMPLER
	STATE_SYNTH_KORG_MICROSAMPLER,
#endif
#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z
	STATE_SYNTH_YAMAHA_TX81Z,
#endif INCLUDE_SYNTH_YAMAHA_TX81Z
	} State;



#define FIRST_APPLICATION (STATE_ROOT + 1)
#define MAX_APPLICATIONS        12        // How many applications do we have slots for?
extern uint8_t state;                     // The current state
extern uint8_t application;               // The top-level non-root state (the application, so to speak)
extern uint8_t optionsReturnState;        // If we're in STATE_OPTIONS and the user presses BACK, where should we go?
extern uint8_t defaultState;              // If we have just BACKed up into a menu state, what state should be the first one displayed?  This can be STATE_NONE
extern uint8_t entry;                     // Are we just entering a state?
#ifdef INCLUDE_IMMEDIATE_RETURN
extern uint8_t immediateReturn;
#endif

// top-level state machine
void go();






//// BYPASSING

extern uint8_t bypass;                                  // are we bypassing MIDI right now?




// some shorthand so we can save a bit of program space.  Of course
// this increases our stack by 14 bytes
extern const char* nrpn_p;// = PSTR("NRPN");
extern const char* rpn_p;// = PSTR("RPN");
extern const char* cc_p;// = PSTR("CC");
extern const char* v_p;// = PSTR("IS");
extern const char* up_p;// = PSTR("UP");
extern const char* down_p;// = PSTR("DOWN");
extern const char* options_p;




///// LOCAL

union _local
    {
    struct _stepSequencerLocal stepSequencer;
    struct _arpLocal arp;
    struct _recorderLocal recorder;
    struct _gaugeLocal gauge;
    struct _thruLocal thru;
    struct _controlLocal control;
    struct _measureLocal measure;
    struct _splitLocal split;
    struct _synthLocal synth;
    };
        
extern _local local;



//// CV OPTIONS

#define NO_VOLTAGE  0
#define VOLTAGE_WITH_VELOCITY	1
#define VOLTAGE_WITH_AFTERTOUCH 2




//// BUTTONS AND POTS

#define BUTTON_PRESSED_COUNTDOWN_DEBOUNCE 313     // about 1/10 of a second must have transpired before we consider another button press  (313 / 3125 == 1/10)
#define BUTTON_PRESSED_COUNTDOWN_MAX 1612         // 1/2 second before a button is considered RELEASED LONG  (1612 / 3125
#define MINIMUM_POT_DEVIATION 8                   // A pot just be turned more than this value before we consider it changed

/// Button and Pot numbers
#define BACK_BUTTON 0
#define MIDDLE_BUTTON 1
#define SELECT_BUTTON 2
#define LEFT_POT 0
#define RIGHT_POT 1

extern uint8_t button[3];                                       // Is button #i presently being held down?
extern uint16_t pot[2];                                         // What is the current value of pot #i?

// Update states
#define NO_CHANGE 0                             		// No change in the button or pot yet
#define CHANGED 1                                       // The value pot has been changed
#define PRESSED 1                                       // The button has been pressed
#define PRESSED_AND_RELEASED 2          				// The button was pressed and released before isUpdated was called to check
#define RELEASED 3                                      // The button has been released
#define RELEASED_LONG 4                         // the button was released after being pressed for 1/2 second or more

extern uint8_t potUpdated[2];       // has the left pot been updated?  CHANGED or NO_CHANGE
extern uint8_t buttonUpdated[3];    // has the back button been updated?  PRESSED, RELEASED, PRESSED_AND_RELEASED, RELEASED_LONG, or NO_CHANGE
extern uint8_t lockoutPots;                     // Set to TRUE when we we want to ignore changes to the pots (perhaps we're changing with NRPN messages)


// SETUP POTS
// initializes the pots
void setupPots();

void clearReleased();

void updateButtons(uint8_t buttonPressed[]);

// Returns if the given button is in the given value state (PRESSED, RELEASED, PRESSED_AND_RELEASED, RELEASED_LONG)
// ALSO: 
uint8_t isUpdated(uint8_t button, uint8_t val);


// FULLRESET()  
// This resets all the parameters to their default values in the EEPROM,
// then reboots the board.

void fullReset();
void semiReset();

//// BYPASS TOGGLE
void toggleBypass(uint8_t channel);

///// THE DISPLAY

// We're going to try to handle both 8-column and 16-column displays.  The 8-column
// display buffer is stored in led.  The "second" display (if using 16 columns) is
// stored in led2.  Note that the second display is to the LEFT of the first display.
//
// We don't update the display every tick because it is costly.  Instead we update
// it every 4 ticks.
//
// The MIDI board also has two small LEDs.  

extern uint8_t updateDisplay;                                   // indicates whether we should update the display
extern unsigned char led[LED_WIDTH];
extern unsigned char led2[LED_WIDTH];






/// MENU, NUMERICAL, AND GLYPH DISPLAYS
/// All display methods set currentDisplay to a number corresponding to the item the
/// user has chosen, or is presently choosing.
extern int16_t currentDisplay;                     // currently displayed menu item


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
// DEFAULT VALUE. 
//
// If the global variable 'entry' is true, then doNumericalDisplay will
// set up the display.  Thereafter (when 'entry' is false) it 
// ignores what's passed into it and just lets the user choose a number.
//
// As the user is scrolling through various options,
// doNumericalDisplay will return NO_MENU_SELECTED, but you can see which
// item is being considered as currentDisplay.  If the user selects a
// menu item, then MENU_SELECTED is set (again the item
// is currentDisplay).  Finally if the user goes back, then
// MENU_CANCELLED is returned.
//
// If _includeOff == 1, then currentDisplay can also be equal to minValue - 1,
// meaning "off", and the displayed value will be "--".
//
// includeOther can be set to OTHER_NONE, OTHER_OMNI, OTHER_DEFAULT, OTHER_FREE, or 
// OTHER_GLYPH, and if anything but OTHER_NONE, it will correspond to the maximum value.
// Furthermore if OTHER_GLYPH is used, then the glyphs array is scanned to determine 
// a glyph to display on the far left side of the screen for each possible numerical
// value.  At present this array is only MAX_GLYPHS in length, so if your max value is 
// larger than this, you're going to have a problem.  

#define OTHER_NONE 0
#define OTHER_OMNI 1
#define OTHER_DEFAULT 2
#define OTHER_DECREMENT 3
#define OTHER_INCREMENT 4
#define OTHER_FREE 5
#define OTHER_GLYPH 255

#define MAX_GLYPHS 11
extern uint8_t glyphs[MAX_GLYPHS];
extern uint8_t secondGlyph;

uint8_t doNumericalDisplay(int16_t minValue, int16_t maxValue, int16_t defaultValue, uint8_t includeOff, uint8_t includeOther);



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



// Set this to schedule the screen brightness to be revised (because this is
// costly and shouldn't be done constantly in response to, say, turning a pot).
extern uint8_t scheduleScreenBrightnessUpdate;



#endif

