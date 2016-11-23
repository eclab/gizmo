#ifndef __TOPLEVEL_H__
#define __TOPLEVEL_H__

#include "All.h"



//// MIDI CHANNELS
#define CHANNEL_OFF 0
#define CHANNEL_OMNI 17                         // For INPUT
#define CHANNEL_DEFAULT 16                      // For OUTPUT, such as when the Step Sequencer wants to say that per-track should follow the default



//// MIDI CLOCK OPTIONS SETTINGS
// We have five options for handling the MIDI clock (or producing our own)
// These are the possible settings for options.clock
#define USE_MIDI_CLOCK 0         // Use external MIDI clock, and pass it through
#define DIVIDE_MIDI_CLOCK 1      // Use external MIDI clock, and pass it through, but slowed down to the given NOTE VALUE
#define CONSUME_MIDI_CLOCK 2     // Use external MIDI clock, but don't pass it through
#define IGNORE_MIDI_CLOCK 3      // Use our own internal clock, but pass any MIDI clock through
#define GENERATE_MIDI_CLOCK 4    // Use our own internal clock and produce an outgoing MIDI clock from it, rather than passing through any MIDI clock
#define BLOCK_MIDI_CLOCK 5       // Use our own internal clock.  Don't pass through or generate any MIDI clock.


// I don't want to do MIDI Namespace stuff.  
// So I have defined the following #defines, copies
// of enumerations in midi_defs.h

#define    MIDIInvalidType           ((midi::MidiType) 0x00)    ///< For notifying errors
#define    MIDINoteOff               ((midi::MidiType) 0x80)    ///< Note Off
#define    MIDINoteOn                ((midi::MidiType) 0x90)    ///< Note On
#define    MIDIAfterTouchPoly        ((midi::MidiType) 0xA0)    ///< Polyphonic AfterTouch
#define    MIDIChannelControl        ((midi::MidiType) 0xB0)    ///< Control Change / Channel Mode
#define    MIDIProgramChange         ((midi::MidiType) 0xC0)    ///< Program Change
#define    MIDIAfterTouchChannel     ((midi::MidiType) 0xD0)    ///< Channel (monophonic) AfterTouch
#define    MIDIPitchBend             ((midi::MidiType) 0xE0)    ///< Pitch Bend
#define    MIDISystemExclusive       ((midi::MidiType) 0xF0)    ///< System Exclusive
#define    MIDITimeCodeQuarterFrame  ((midi::MidiType) 0xF1)    ///< System Common - MIDI Time Code Quarter Frame
#define    MIDISongPosition          ((midi::MidiType) 0xF2)    ///< System Common - Song Position Pointer
#define    MIDISongSelect            ((midi::MidiType) 0xF3)    ///< System Common - Song Select
#define    MIDITuneRequest           ((midi::MidiType) 0xF6)    ///< System Common - Tune Request
#define    MIDIClock                 ((midi::MidiType) 0xF8)    ///< System Real Time - Timing Clock
#define    MIDIStart                 ((midi::MidiType) 0xFA)    ///< System Real Time - Start
#define    MIDIContinue              ((midi::MidiType) 0xFB)    ///< System Real Time - Continue
#define    MIDIStop                  ((midi::MidiType) 0xFC)    ///< System Real Time - Stop
#define    MIDIActiveSensing         ((midi::MidiType) 0xFE)    ///< System Real Time - Active Sensing
#define    MIDISystemReset           ((midi::MidiType) 0xFF)    ///< System Real Time - System Reset






// STATES
// These are the states in the overall state machine

#if defined(__AVR_ATmega2560__)
#define STATE_NONE 255
#define STATE_ROOT 0
#define STATE_ARPEGGIATOR 1
#define STATE_STEP_SEQUENCER 2
#define STATE_RECORDER 3
#define STATE_GAUGE 4
#define STATE_CONTROLLER 5
#define STATE_SPLIT 6
#define STATE_OPTIONS 7
#define STATE_UNDEFINED_2 8      // Future applications
#define STATE_UNDEFINED_3 9      // Future applications
#define STATE_UNDEFINED_4 10      // Future applications
#define STATE_UNDEFINED_5 11      // Future applications
#define STATE_UNDEFINED_6 12      // Future applications
#define STATE_ARPEGGIATOR_PLAY 13
#define STATE_ARPEGGIATOR_PLAY_OCTAVES 14
#define STATE_ARPEGGIATOR_CREATE 15
#define STATE_ARPEGGIATOR_CREATE_EDIT 16
#define STATE_ARPEGGIATOR_CREATE_SAVE 17
#define STATE_ARPEGGIATOR_CREATE_SURE 18
#define STATE_STEP_SEQUENCER_FORMAT 19
#define STATE_STEP_SEQUENCER_PLAY 20
#define STATE_STEP_SEQUENCER_MENU 21
#define STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT 22
#define STATE_STEP_SEQUENCER_VELOCITY 23
#define STATE_STEP_SEQUENCER_FADER 24
#define STATE_STEP_SEQUENCER_LENGTH 25
#define STATE_STEP_SEQUENCER_SURE 26
#define STATE_STEP_SEQUENCER_SAVE 27
#define STATE_RECORDER_FORMAT 28
#define STATE_RECORDER_PLAY 29
#define STATE_RECORDER_SAVE 30
#define STATE_RECORDER_SURE 31
#define STATE_RECORDER_MENU 32
#define STATE_CONTROLLER_PLAY 33
#define STATE_CONTROLLER_SET_LEFT_KNOB_TYPE 34
#define STATE_CONTROLLER_SET_RIGHT_KNOB_TYPE 35
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_TYPE 36
#define STATE_CONTROLLER_SET_SELECT_BUTTON_TYPE 37
#define STATE_CONTROLLER_SET_LEFT_KNOB_NUMBER 38
#define STATE_CONTROLLER_SET_RIGHT_KNOB_NUMBER 39
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_NUMBER 40
#define STATE_CONTROLLER_SET_SELECT_BUTTON_NUMBER 41
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_ON 42
#define STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON 43
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF 44
#define STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF 45
#define STATE_OPTIONS_TEMPO 46
#define STATE_OPTIONS_NOTE_SPEED 47
#define STATE_OPTIONS_SWING 48
#define STATE_OPTIONS_TRANSPOSE 49
#define STATE_OPTIONS_VOLUME 50
#define STATE_OPTIONS_PLAY_LENGTH 51
#define STATE_OPTIONS_MIDI_CHANNEL_IN 52
#define STATE_OPTIONS_MIDI_CHANNEL_OUT 53
#define STATE_OPTIONS_MIDI_CHANNEL_CONTROL 54
#define STATE_OPTIONS_MIDI_CLOCK 55
#define STATE_OPTIONS_CLICK 56
#define STATE_OPTIONS_SCREEN_BRIGHTNESS 57
#define STATE_OPTIONS_MENU_DELAY 58
#define STATE_OPTIONS_VOLTAGE 59
#define STATE_OPTIONS_ABOUT 60
#define STATE_SPLIT_CHANNEL 61
#define STATE_SPLIT_NOTE 62

#else 
#define STATE_NONE 255
#define STATE_ROOT 0
#define STATE_ARPEGGIATOR 1
#define STATE_STEP_SEQUENCER 2
#define STATE_RECORDER 3
#define STATE_GAUGE 4
#define STATE_CONTROLLER 5
#define STATE_OPTIONS 6
#define STATE_UNDEFINED_1 7
#define STATE_UNDEFINED_2 8      // Future applications
#define STATE_UNDEFINED_3 9      // Future applications
#define STATE_UNDEFINED_4 10      // Future applications
#define STATE_UNDEFINED_5 11      // Future applications
#define STATE_UNDEFINED_6 12      // Future applications
#define STATE_ARPEGGIATOR_PLAY 13
#define STATE_ARPEGGIATOR_PLAY_OCTAVES 14
#define STATE_ARPEGGIATOR_CREATE 15
#define STATE_ARPEGGIATOR_CREATE_EDIT 16
#define STATE_ARPEGGIATOR_CREATE_SAVE 17
#define STATE_ARPEGGIATOR_CREATE_SURE 18
#define STATE_STEP_SEQUENCER_FORMAT 19
#define STATE_STEP_SEQUENCER_PLAY 20
#define STATE_STEP_SEQUENCER_MENU 21
#define STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT 22
#define STATE_STEP_SEQUENCER_VELOCITY 23
#define STATE_STEP_SEQUENCER_FADER 24
#define STATE_STEP_SEQUENCER_LENGTH 25
#define STATE_STEP_SEQUENCER_SURE 26
#define STATE_STEP_SEQUENCER_SAVE 27
#define STATE_RECORDER_FORMAT 28
#define STATE_RECORDER_PLAY 29
#define STATE_RECORDER_SAVE 30
#define STATE_RECORDER_SURE 31
#define STATE_RECORDER_MENU 32
#define STATE_CONTROLLER_PLAY 33
#define STATE_CONTROLLER_SET_LEFT_KNOB_TYPE 34
#define STATE_CONTROLLER_SET_RIGHT_KNOB_TYPE 35
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_TYPE 36
#define STATE_CONTROLLER_SET_SELECT_BUTTON_TYPE 37
#define STATE_CONTROLLER_SET_LEFT_KNOB_NUMBER 38
#define STATE_CONTROLLER_SET_RIGHT_KNOB_NUMBER 39
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_NUMBER 40
#define STATE_CONTROLLER_SET_SELECT_BUTTON_NUMBER 41
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_ON 42
#define STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON 43
#define STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF 44
#define STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF 45
#define STATE_OPTIONS_TEMPO 46
#define STATE_OPTIONS_NOTE_SPEED 47
#define STATE_OPTIONS_SWING 48
#define STATE_OPTIONS_TRANSPOSE 49
#define STATE_OPTIONS_VOLUME 50
#define STATE_OPTIONS_PLAY_LENGTH 51
#define STATE_OPTIONS_MIDI_CHANNEL_IN 52
#define STATE_OPTIONS_MIDI_CHANNEL_OUT 53
#define STATE_OPTIONS_MIDI_CHANNEL_CONTROL 54
#define STATE_OPTIONS_MIDI_CLOCK 55
#define STATE_OPTIONS_CLICK 56
#define STATE_OPTIONS_SCREEN_BRIGHTNESS 57
#define STATE_OPTIONS_VOLTAGE 58
#define STATE_OPTIONS_ABOUT 59
#endif // defined(__AVR_ATmega2560__)




/// THE STATE MACHINE
#define MAX_APPLICATIONS        12                // How many applications do we have slots for?
extern uint8_t state;                 // The current state
extern uint8_t application;           // The top-level non-root state (the application, so to speak)
extern uint8_t entry;                     // Are we just entering a state?
extern uint8_t optionsReturnState;        // If we're in STATE_OPTIONS and the user presses BACK, where should we go?
extern uint8_t defaultState;              // If we have just BACKed up into a menu state, what state should be the first one displayed?  This can be STATE_NONE

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
#if defined(__AVR_ATmega2560__)
extern const char* voltage_p;// = PSTR("VOLTAGE");
#endif
extern const char* length_p;
extern const char* options_p;




///// LOCAL

union _local
    {
    struct _stepSequencerLocal stepSequencer;
    struct _arpLocal arp;
    struct _recorderLocal recorder;
    struct _gaugeLocal gauge;
    };
        
extern _local local;






//// BUTTONS AND POTS

#define BUTTON_PRESSED_COUNTDOWN_MAX 1612                       // 1/2 second before a button is considered RELEASED LONG
#define MINIMUM_POT_DEVIATION 4                                         // A pot just be turned more than this value before we consider it changed

/// Button and Pot numbers
#define BACK_BUTTON 0
#define MIDDLE_BUTTON 1
#define SELECT_BUTTON 2
#define LEFT_POT 0
#define RIGHT_POT 1

extern uint8_t button[3];                                       // Is button #i presently being held down?
extern uint16_t pot[2];                                         // What is the current value of pot #i?

// Update states
#define NO_CHANGE 0                                     // No change in the button or pot yet
#define CHANGED 1                                       // The value pot has been changed
#define PRESSED 1                                       // The button has been pressed
#define PRESSED_AND_RELEASED 2          // The button was pressed and released before isUpdated was called to check
#define RELEASED 3                                      // The button has been released
#define RELEASED_LONG 4                         // the button was released after being pressed for 1/2 second or more

extern uint8_t potUpdated[2];       // has the left pot been updated?  CHANGED or NO_CHANGE
extern uint8_t buttonUpdated[3];    // has the back button been updated?  PRESSED, RELEASED, PRESSED_AND_RELEASED, RELEASED_LONG, or NO_CHANGE
extern uint8_t lockoutPots;                     // Set to TRUE when we we want to ignore changes to the pots (perhaps we're changing with NRPN messages)


// SETUP POTS
// initializes the pots
void setupPots();

// Returns if the given button is in the given value state (PRESSED, RELEASED, PRESSED_AND_RELEASED, RELEASED_LONG)
// ALSO: 
uint8_t isUpdated(uint8_t button, uint8_t val);



/// ALL POSSIBLE itemTypes that we might receive
#define MIDI_CLOCK                      0
#define MIDI_TIME_CODE          1
#define MIDI_ACTIVE_SENSING     2
#define MIDI_NOTE_ON   3
#define MIDI_NOTE_OFF    4
#define MIDI_AFTERTOUCH 5
#define MIDI_AFTERTOUCH_POLY 6
#define MIDI_PROGRAM_CHANGE     7
#define MIDI_PITCH_BEND 8
#define MIDI_SYSTEM_EXCLUSIVE   9
#define MIDI_SONG_POSITION      10
#define MIDI_SONG_SELECT        11
#define MIDI_TUNE_REQUEST       12
#define MIDI_START      13
#define MIDI_CONTINUE   14
#define MIDI_STOP       15
#define MIDI_SYSTEM_RESET       16
#define MIDI_CC_7_BIT    17                // simple CC messages
#define MIDI_CC_14_BIT    18               // all other CC messages other than NRPN, RPN
#define MIDI_NRPN_14_BIT    19                                 // NRPN DATA messages
#define MIDI_NRPN_INCREMENT    20
#define MIDI_NRPN_DECREMENT    21
#define MIDI_RPN_14_BIT    22                               // RPN DATA messages
#define MIDI_RPN_INCREMENT    23
#define MIDI_RPN_DECREMENT    24


// Bits to raise for newRapidMidiItem
#define MIDI_CLOCK_BIT 0
#define MIDI_TIME_CODE_BIT 1
#define ACTIVE_SENSING_BIT 2

// newItem can be any of the following
#define NO_NEW_ITEM 0
#define NEW_ITEM 1
#define WAIT_FOR_A_SEC 2        // indicates that LSB data might be coming through any minute now.  Or not.


// NEW INCOMING MIDI DATA TRIGGERS
extern uint8_t  newItem;                // newItem can be 0, 1, or WAIT_FOR_A_SEC
extern uint8_t  newRapidMidiItem;       // Bit 0 is MIDI_CLOCK_BIT, Bit 1 is MIDI_TIME_CODE_BIT, Bit 2 is ACTIVE_SENSING_BIT.  This could be merged with newItem
extern uint8_t itemType;
extern uint16_t itemNumber;             // Note on/off/poly aftertouch use this for NOTE PITCH
extern uint16_t itemValue;                      // Note on/off/poly aftertouch use this for NOTE VELOCITY / AFTERTOUCH


// FULLRESET()  
// This resets all the parameters to their default values in the EEPROM,
// then reboots the board.

void fullReset();



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



#if defined(__AVR_ATmega2560__)
// SET MENU DELAY
// Changes the menu delay to a desired value (between 0: no menu delay, and 11: infinite menu delay).  The default is 5
void setMenuDelay(uint8_t index);

// The index values passed into setMenuDelay correspond to the following delays (but these
// constants may NOT be passed into setMenuDelay).

#endif

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
#define OTHER_INCREMENT 3
#define OTHER_DECREMENT 4
#define OTHER_GLYPH 5

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


/// MIDI HANDLERS
/// Called by the Midi system.

void handleStart();
void handleStop();
void handleContinue();
void handleClock();
void handleNoteOff(byte channel, byte note, byte velocity);
void handleNoteOn(byte channel, byte note, byte velocity);
void handleAfterTouchPoly(byte channel, byte note, byte pressure);
void handleGeneralControlChange(byte channel, byte number, byte value);
void handleProgramChange(byte channel, byte number);
void handleAfterTouchChannel(byte channel, byte pressure);
void handlePitchBend(byte channel, int bend);
void handleSystemExclusive(byte* array, unsigned size);
void handleTimeCodeQuarterFrame(byte data);
void handleSongPosition(unsigned beats);
void handleSongSelect(byte songnumber);
void handleTuneRequest();
void handleActiveSensing();
void handleSystemReset();


/// MIDI OUTPUT

// We call this instead of MIDI.sendNoteOn so we can do stuff
// like transposition and volume control.
void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
void sendAllNotesOff();


//// REMOTE CONTROL VIA NRPN

// The following NRPN parameters will control Gizmo.
// Buttons (including BYPASS and UNLOCK) are 1 when pressed, 0 when unpressed.
// Pots should be sent 14-bit, but only range from 0...1023.

#define NRPN_BACK_BUTTON_PARAMETER              0
#define NRPN_MIDDLE_BUTTON_PARAMETER            1
#define NRPN_SELECT_BUTTON_PARAMETER            2
#define NRPN_BACK_BUTTON_PARAMETER              0
#define NRPN_MIDDLE_BUTTON_PARAMETER            1
#define NRPN_SELECT_BUTTON_PARAMETER            2
#define NRPN_LEFT_POT_PARAMETER                 3
#define NRPN_RIGHT_POT_PARAMETER                4
#define NRPN_BYPASS_PARAMETER                   5
#define NRPN_UNLOCK_PARAMETER                   6
#define NRPN_START_PARAMETER                    7
#define NRPN_STOP_PARAMETER                     8       
#define NRPN_CONTINUE_PARAMETER                 9



#endif

