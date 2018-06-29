////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"






//// SETTING UP MIDI

MIDI_CREATE_DEFAULT_INSTANCE();

///// COMMON PROGMEM STRINGS

// some shorthand so we can save a bit of program space.  Of course
// this uses up some of our working memory.  These are PSTR strings
// used in more than one location in the program.  They're set in the Gizmo.ino file
GLOBAL const char* nrpn_p;// = PSTR("NRPN");
GLOBAL const char* rpn_p;// = PSTR("RPN");
GLOBAL const char* cc_p;// = PSTR("CC");
GLOBAL const char* v_p;// = PSTR("IS");
GLOBAL const char* options_p;  // = PSTR("OPTIONS");






/// BYPASSING

///    If 'bypass' is 1, then all signals are currently being sent through no matter what, and
///    furthermore, the system cannot emit any signals.  The user can turn on bypass
///    either by choosing it in the options, or by long-pressing BACK+SELECT.
///    Don't set bypass -- instead call toggleBypass

#define BYPASS_ON (1)
#define BYPASS_OFF (0)
#define BYPASS_FIRST_ON (2)

#ifdef TOPLEVEL_BYPASS
GLOBAL uint8_t bypass = BYPASS_FIRST_ON;                       // This is set when we are doing bypassing
#else
GLOBAL uint8_t bypass = BYPASS_OFF;                       // This is set when we are doing bypassing
#endif

GLOBAL uint8_t bypassOut = BYPASS_OFF;                    // This is set when we are preventing Gizmo's applications from writing out
GLOBAL uint8_t dontBypassOut = 0;				 // An application can set this to allow writing out, but he must afterwards say 

void toggleBypass(uint8_t channel)
	{
#ifdef TOPLEVEL_BYPASS
	// the first time through bypass is BYPASS_FIRST_ON.
	// only after it's been turned off (or back on again) do we start doing all-sounds-off.
	if (bypass !=  BYPASS_FIRST_ON)
		sendAllSoundsOffDisregardBypass(channel);
#else
    sendAllSoundsOffDisregardBypass(channel);
#endif

#ifndef HEADLESS
    if (!bypass) 
        {
	    // clear the LEDs
        *port_LED_GREEN |= LED_GREEN_mask;
        *port_LED_RED |= LED_RED_mask;
        }
#endif
    bypass = !bypass;
    if (bypass) { MIDI.turnThruOn(); }
    else MIDI.turnThruOff();

    bypassOut = (bypass && !dontBypassOut);
	}
	





///// READING SENSORS

// Some rules for how reading is done.
//
// The three buttons are called the BACK, MIDDLE, and SELECT buttons.
//
// Pressing the select button will usually choose an option, except in certain applications
// where it might do a special function along with pressing the middle button.
//
// Pressing the middle button doesn't do anything except in certain applications where it
// does some special function. 
//
// Pressing the back button will always go back (up the tree).
//
// Pressing and HOLDING a button is called a LONG_PRESS.  
//
// You can test for the state of the buttons via the backButton, middleButton, and selectButton
// variables.  These are 0/1. 
//
// When a button changes state, then buttonUpdated[BACK_BUTTON], buttonUpdated[MIDDLE_BUTTON], or buttonUpdated[SELECT_BUTTON]
// is set to PRESSED, RELEASED, or RELEASED_LONG to indicate the change.  If no change occurs then 
// the button is set to NO_CHANGE.
//
// POTS.  The current value of a pot can be found in pot[LEFT_POT] or pot[RIGHT_POT]. When a pot has been
// twisted more than MINIMUM_POT_DEVIATION, it is considered to have been updated.  At this point the
// potUpdated[LEFT_POT] or potUpdated[RIGHT_POT] variables are revised to say CHANGED (rather than NO_CHANGE)
// to indicate that the pot has been just set to something new.
//
// UPDATING.  We don't update the buttons or pots every tick, because it is expensive.  Instead
// we update them every 4 ticks (the pots are updated a different tick than the buttons).

#define NUM_BUTTONS (3)

#define BACK_BUTTON 0
#define MIDDLE_BUTTON 1
#define SELECT_BUTTON 2

GLOBAL uint8_t button[NUM_BUTTONS];
GLOBAL uint8_t buttonUpdated[NUM_BUTTONS] = { NO_CHANGE, NO_CHANGE, NO_CHANGE };
GLOBAL static uint8_t ignoreNextButtonRelease[NUM_BUTTONS] = { false, false, false };


#ifdef INCLUDE_EXTENDED_CONTROLLER
#define NUM_POTS (4)
#else
#define NUM_POTS (2)
#endif

#define LEFT_POT 0
#define RIGHT_POT 1
#define A2_POT 2
#define A3_POT 3

GLOBAL uint16_t pot[NUM_POTS];        // The current pot value OR MIDI controlled value
GLOBAL uint8_t potUpdated[NUM_POTS];       // has the pot been updated?  CHANGED or NO_CHANGE
GLOBAL static uint16_t potCurrent[NUM_POTS][3];     // The filtered current pot value
GLOBAL static uint16_t potCurrentFinal[NUM_POTS];    //  
GLOBAL static uint16_t potLast[NUM_POTS];     // The last pot value submitted 


// SETUP POTS
// initializes the pots
void setupPots()
    {
    memset(potCurrent, 0, NUM_POTS * 3);
    memset(potUpdated, NO_CHANGE, NUM_POTS);
    memset(pot, 0, NUM_POTS);
    memset(potCurrentFinal, 0, NUM_POTS);
    memset(potLast, 0, NUM_POTS);
    }

//// Clears the 'released' and 'released long' flag on all buttons.
void clearReleased()
    {
    for(uint8_t i = 0; i < NUM_BUTTONS; i++)
        for(uint8_t j = PRESSED; j <= RELEASED_LONG; j++)
            isUpdated(i, j);
    } 

// Returns 1 if the given button is in the given value state (PRESSED, RELEASED, PRESSED_AND_RELEASED, RELEASED_LONG)
// If the button is PRESSED_AND_RELEASED then if the user queries EITHER PRESSED or RELEASED, then 1 is returned.
//
// Additionally there are some side effects if the query returns 1:
// 1. Sets the button state to NO_CHANGE so later queries won't see anything
// 2. If the button query was for PRESSED, and the button is indeed PRESSED, then when the button is
//    later RELEASED or RELEASED_LONG, the query will ignore this.
// then 
uint8_t isUpdated(uint8_t button, uint8_t type)
    {
    if (type == buttonUpdated[button] || ((type == PRESSED || type == RELEASED) && buttonUpdated[button] == PRESSED_AND_RELEASED))
        {
        if (buttonUpdated[button] == PRESSED)  // user asked about PRESSED, and the button is indeed PRESSED
            {
            ignoreNextButtonRelease[button] = true;
            }
        buttonUpdated[button] = NO_CHANGE;
        return 1;
        }
    else return 0;
    }


/// BUTTON COUNTDOWNS
//  I use the button countdown in two ways:
//  1. If the button is presently RELEASED, then I ignore presses until the button countdown counts through BUTTON_PRESSED_COUNTDOWN_DEBOUNCE ticks
//  2. If the button is presently PRESSED, and continues to be pressed for up to BUTTON_PRESSED_COUNTDOWN_MAX, then it's considered a LONG PRESS

GLOBAL int16_t buttonPressedCountdown[3] = { BUTTON_PRESSED_COUNTDOWN_DEBOUNCE, BUTTON_PRESSED_COUNTDOWN_DEBOUNCE, BUTTON_PRESSED_COUNTDOWN_DEBOUNCE };

void updateButtons(uint8_t buttonPressed[])
    {
    for(uint8_t i = 0; i < 3; i++)
        {
        if (!button[i])         // button used to be released
            {
            if (buttonPressed[i] && buttonPressedCountdown[i] == 0)        // button was pressed and there's been enough time
                {
                buttonUpdated[i] = PRESSED;
                buttonPressedCountdown[i] = BUTTON_PRESSED_COUNTDOWN_MAX;
                button[i] = buttonPressed[i];
                }
            }
        else                            			// button used to be pressed
            {
            if (!buttonPressed[i])                  // button was released or released long
                {
                if (ignoreNextButtonRelease[i])
                    {
                    ignoreNextButtonRelease[i] = 0;  // ignore it this time around, but not later releases
                    }
                else if (buttonPressedCountdown[i] == 0)
                    buttonUpdated[i] = RELEASED_LONG;
                else if (buttonUpdated[i] == PRESSED)  // looks like it wasn't checked for
                    buttonUpdated[i] = PRESSED_AND_RELEASED;
                else
                    buttonUpdated[i] = RELEASED;
                buttonPressedCountdown[i] = BUTTON_PRESSED_COUNTDOWN_DEBOUNCE;
                button[i] = buttonPressed[i];
                }
            }
        }
    }


/// Gets the new value of a pot and sets it into pot.  Returns CHANGED or NO_CHANGE.
/// potCurrent is an array of the most recent three calls to analogRead for the pot.  It's
/// used to do a median filter.
//  VARIABLES:
//  pot                                 Where to store the resulting value
//  potCurrent                  An array of three numbers which will store the most recent three calls to analogRead()
//  potCurrentFinal             A value which will store the most recent smoothed estimate produced by potCurrent
//  potLast                             A value which will hold the PREVIOUS smoothed estimate produced by potCurrent
//  analog                              The analog pin to read via analogRead(analog)
//
//  RETURNED: CHANGED or NO CHANGED, depending if potCurrentFinal and potLast are sufficiently different from
//            one another to assume that the pot is being changed by the user

uint8_t updatePot(uint16_t &pot, uint16_t* potCurrent, uint16_t &potCurrentFinal, uint16_t& potLast, uint8_t analog)
    {
    
    // we clean up the pots as follows:
    // 1. Run it through a median of three filter
    // 2. potCurrentFinal <- 1/2 potCurrentFinal + 1/2 result from #1
    // 3. If potCurrentFinal differs from its old value by at least MINIMUM_POT_DEVIATION (8), then we have a new pot value.
    
    // This implies that the pots have a realistic resolution of 1024 / 8 = 128.
    // The biggest range that we need to dial in is 2^14 = 16384.
    // This means that the RIGHT POT must have a resolution of at least 128, since 128 * 128 = 16384.
    
    // I'd like MINIMUM_POT_DEVIATION to be 4, but it's just too noisy.  :-(
 
    potCurrent[0] = potCurrent[1];
    potCurrent[1] = potCurrent[2];
    potCurrent[2] = analogRead(analog);
    uint16_t middle = MEDIAN_OF_THREE(potCurrent[0], potCurrent[1], potCurrent[2]);
    if (middle == 1023)         // we handle this exceptional condition because otherwise potCurrentFinal would never be >= 1022, due to the division.
        potCurrentFinal = middle;
    else potCurrentFinal = (potCurrentFinal + middle) / 2;
    // test to see if we're really turning the knob
    if (potLast != potCurrentFinal && 
            (potLast > potCurrentFinal && potLast - potCurrentFinal >= MINIMUM_POT_DEVIATION ||
            potCurrentFinal > potLast && potCurrentFinal - potLast >= MINIMUM_POT_DEVIATION ||
            potCurrentFinal == 1023 || //potCurrentFinal == 1023 - MINIMUM_POT_DEVIATION ||             // handle boundary condition    -- NOTE: too noisy.  I've removed it and mentioned it in the manual.
            potCurrentFinal == 0 )) //potCurrentFinal <= MINIMUM_POT_DEVIATION))             // handle boundary condition
        { potLast = potCurrentFinal; pot = potCurrentFinal; return CHANGED; }
    else return NO_CHANGE;
    }












///// THE DISPLAY

// We're going to try to handle both 8-column and 16-column displays.  The 8-column
// display buffer is stored in led.  The "second" display (if using 16 columns) is
// stored in led2.  Note that the second display is to the LEFT of the first display.
//
// We don't update the display every tick because it is costly.  Instead we update
// it every 4 ticks.
//
// The MIDI board also has two small LEDs.  

GLOBAL uint8_t updateDisplay = 0;
GLOBAL unsigned char led[LED_WIDTH];
GLOBAL unsigned char led2[LED_WIDTH];











// UPDATE()
// This function is called every time tick inside go(), and every tick 
// it does one of four things:
//
// 1. Reads and updates the buttons and returning 0
// 2. Reads and updates the left pot and returning 0
// 3. Reads and updates the right pot and returning 0
// 4. Signals that applications should redraw the screen, by returning 1

/// When this is set, turning the pots won't affect anything.
/// Pressing a button unlocks the pots.
GLOBAL uint8_t lockoutPots = 0;
GLOBAL uint8_t scheduleScreenBrightnessUpdate = 0;

uint8_t update()
    {
    switch (tickCount & 0x3) // That is, binary 11
        {
        case 0:
            {
#ifndef HEADLESS
            uint8_t buttonPressed[3];
            
            // Note that when we're pressed, the value is ZERO, so we do !pressed
            buttonPressed[BACK_BUTTON] = !(*port_BACK_BUTTON & BACK_BUTTON_mask) ;
            buttonPressed[MIDDLE_BUTTON] = !(*port_MIDDLE_BUTTON & MIDDLE_BUTTON_mask) ;
            buttonPressed[SELECT_BUTTON] = !(*port_SELECT_BUTTON & SELECT_BUTTON_mask) ;
            
            // continue to lock out pots only if no button is being pressed
            lockoutPots = lockoutPots && (!buttonPressed[BACK_BUTTON]) && (!buttonPressed[MIDDLE_BUTTON]) && (!buttonPressed[SELECT_BUTTON]);
                        
            // This next if-statement is pretty critical.  When you press a button remotely via CC, it immediately locks out the
            // pots, and then changes the buttonPressed[] array appropriately.  This line prevents update() from erasing that
            // freshly-minted buttonPressed[] array, unless the user is actually pressing a physical button on Gizmo.
            if (!lockoutPots)
            	{
            	updateButtons(buttonPressed);
            	}
#endif // HEADLESS
            return 0;  // don't update the display
            }
        break;
        case 1:
            {
#ifndef HEADLESS
            if (!lockoutPots)
                potUpdated[LEFT_POT] = updatePot(pot[LEFT_POT], potCurrent[LEFT_POT], potCurrentFinal[LEFT_POT], potLast[LEFT_POT], A0);
#endif // HEADLESS
#ifdef INCLUDE_EXTENDED_CONTROLLER
                potUpdated[A2_POT] = updatePot(pot[A2_POT], potCurrent[A2_POT], potCurrentFinal[A2_POT], potLast[A2_POT], A14);
#endif
            return 0;  // don't update the display
            }
        break;
        case 2:
            {
#ifndef HEADLESS
            if (!lockoutPots)
                potUpdated[RIGHT_POT] = updatePot(pot[RIGHT_POT], potCurrent[RIGHT_POT], potCurrentFinal[RIGHT_POT], potLast[RIGHT_POT], A1);
#endif // HEADLESS
#ifdef INCLUDE_EXTENDED_CONTROLLER
                potUpdated[A3_POT] = updatePot(pot[A3_POT], potCurrent[A3_POT], potCurrentFinal[A3_POT], potLast[A3_POT], A15);
#endif
            return 0;  // don't update the display
            }
        break;  
        case 3:
            {
            // we'll only update the screen every 32 times.  This is
            // a refresh rate of about 100 times a sec.  It's also enough
            // to approximately display 999 beats per second. Perhaps we might
            // do other I2C stuff as well for the other times
            if ((tickCount & 31) == 31)
                return 1;  // update the display
            // off-sync with the screen update, we will update the screen brightness
            // if it's been requested.  This is because it's an I2C operation
            // and can be costly if called many times in sequence. 
            // At present screen brightness change is NONblocking
            else if (scheduleScreenBrightnessUpdate && ((tickCount & 31) == 15))
                {
                setScreenBrightness(options.screenBrightness);
                scheduleScreenBrightnessUpdate = 0;
                }
            return 0;
            }
        break;
        }
    }
  

// SEMIRESET()  
// This resets and resaves the options, then reboots the board

void semiReset()
    {
    resetOptions();
    saveOptions();
    soft_restart();
    }


// FULLRESET()  
// This resets all the parameters to their default values in the EEPROM,
// then reboots the board.

void fullReset()
    {
    for(uint8_t i = 0; i < NUM_SLOTS; i++)
        {
        loadSlot(i);
        data.slot.type = SLOT_TYPE_EMPTY;
        saveSlot(i);
        }
  
    for(uint8_t i = 0; i < NUM_ARPS; i++)
        {
        LOAD_ARPEGGIO(i);
        data.arp.length = 0;
        SAVE_ARPEGGIO(i);
        }

    semiReset();
    }




//// writeFooter(led, led2)
//
// This function writes the "footer" (the last row of the rightmost matrix).
// The last row indicates the application (first four dots), 
// the beat (the far right dot), and whether or not we're
// bypassed (the far right down, rapidly blinking).
//
// presently led2 is ignored
//
// Note that this code assumes that you've cleared the screen.  If you don't
// always do updates on updateDisplay, then you should clear the footer at least

uint8_t drawBeatToggle = 0;
uint8_t drawNotePulseToggle = 0;

void writeFooterAndSend()
    {
    drawRange(led, 1, 0, MAX_APPLICATIONS, application - FIRST_APPLICATION);
    if (bypass)
        {
        blinkPoint(led, 7, 0);
        }
    else if (getClockState() == CLOCK_RUNNING)
        {
        if (drawBeatToggle)
            {
            setPoint(led, 7, 0);
            }
                
        if (drawNotePulseToggle)
            {
            setPoint(led, 0, 0);
            }
        }
    sendMatrix(led, led2);
    }










/// GAUGE HELPER FUNCTIONS
/// Most of the Gauge app is inlined in the state machine below.
/// These three helper functions reduce the memory footprint.
/// However we can't move these to a file like Gauge.cpp because
/// the linker wastes memory in doing so.  So they're staying here.

/// Adds a number to the buffer.  The number may be buffered with spaces
/// at its beginning: these are not removed.  This is used to write the FIRST
/// number in a scrolling message for CC, RPN, or NRPN.
void addGaugeNumberNoTrim(uint16_t val)
    {
    char b[6];
    numberToString(b, val);
    addToBuffer(b);
    }

/// Adds a number to the buffer, removing initial spaces.  This is used to write
/// subsequent numbers in a scrolling message for CC, RPN, or NRPN.
void addGaugeNumber(uint16_t val)
    {
    char b[6];
    char* a;
    numberToString(b, val);
    a = b;
    while(a[0] == ' ') a++;  // trim out initial spaces
    addToBuffer(a);
    }

/// Writes a note pitch and velocity (or pressure value) to the screen.
void writeGaugeNote()
    {
    writeNotePitch(led2, (uint8_t) itemNumber);                           // Note
    writeShortNumber(led, (uint8_t) itemValue, false);            // Velocity or Pressure
    }




//// MIDI SIGNALS
////
//// These are occasional (not constant) MIDI signals
//// that we can reasonably display on our gauge.  This is used for STATE_GAUGE_ANY
//// Omitted are: MIDI Clock, Active Sensing, both AfterTouch forms, Time Code Quarter Frame



GLOBAL uint8_t  newItem = NO_NEW_ITEM;                        // newItem can be 0, 1, or WAIT_FOR_A_SEC
GLOBAL uint8_t itemType;                                                // See Item types in TopLevel.cpp
GLOBAL uint16_t itemNumber;                             // Note on/off/poly aftertouch use this for NOTE PITCH
GLOBAL uint16_t itemValue;                      // Note on/off/poly aftertouch use this for NOTE VELOCITY / AFTERTOUCH
GLOBAL uint8_t itemChannel = CHANNEL_OFF;




//// LOCAL APPLICATION DATA
///  [hehe, global local]
GLOBAL _local local;
        

void write3x5GlyphPair(uint8_t glyph1, uint8_t glyph2)
    {
    write3x5Glyph(led2, glyph1, 0);
    write3x5Glyph(led2, glyph2, 4);
    }

//// TOP LEVEL STATE VARIABLES

GLOBAL uint8_t state = STATE_ROOT;                     // The current state
GLOBAL uint8_t application = FIRST_APPLICATION;           // The top state (the application, so to speak): we display a dot indicating this.
GLOBAL uint8_t entry = 1;  
GLOBAL uint8_t optionsReturnState;
GLOBAL uint8_t defaultState = STATE_NONE;
#ifdef INCLUDE_EXTENDED_MENU_DEFAULTS
// Explanation.  While doNumericalDisplay uses defaultState for its defaults, doMenuDisplay cannot because
// defaultState is already used to describe a *state*.  So if you have a non-stateful menu which jumps to
// a new state and then you jump back with the defaultState set, you don't want the non-stateful menu
// to interpret this as a default *value*.  So this hack allows us to specify a default value for a menu
// that's not stateful.  Yuck.
GLOBAL uint8_t defaultMenuValue = 0;
#endif
#ifdef INCLUDE_IMMEDIATE_RETURN
GLOBAL uint8_t immediateReturn = false;
#endif

////// GO()
//
// This is the top-level state machine.
//
//
// Note that a number of states (such as the gauge, options, and a few others)
// are hard-inlined in here rather than broken out into their own functions.  The
// reason for this is simple: space.  The compiler makes much tighter code when 
// there's lots of stuff in the go() function, so I'm including what I can before
// it turns into a giant hairball in order to stay within the memory space of an
// Atmel 328. 


void go()
    {
    MIDI.read();
    
    for(uint8_t i = 0; i < 3; i++)
        if (buttonPressedCountdown[i] > 0) 
            buttonPressedCountdown[i]--;

    updateTimers();

    // update the screen, read from the sensors, or update the board LEDs
    updateDisplay = update();
    
    if (isUpdated(BACK_BUTTON, RELEASED_LONG))
        {
        toggleBypass(CHANNEL_OMNI);
        }

    // Now do your state-specific thing
    switch(state)
        {
        case STATE_ROOT:
            {
            if (entry)
                {
                optionsReturnState = STATE_ROOT;
                }
#if defined(__MEGA__)
#if defined(INCLUDE_SYSEX)
            const char* menuItems[11] = { PSTR("ARPEGGIATOR"), PSTR("STEP SEQUENCER"), PSTR("RECORDER"), PSTR("GAUGE"), PSTR("CONTROLLER"), PSTR("SPLIT"), PSTR("THRU"), PSTR("SYNTH"), PSTR("MEASURE"), PSTR("SYSEX"), options_p };
            if (doMenuDisplay(menuItems, 11, FIRST_APPLICATION, STATE_ROOT, 1) == MENU_SELECTED)
#else
            const char* menuItems[10] = { PSTR("ARPEGGIATOR"), PSTR("STEP SEQUENCER"), PSTR("RECORDER"), PSTR("GAUGE"), PSTR("CONTROLLER"), PSTR("SPLIT"), PSTR("THRU"), PSTR("SYNTH"), PSTR("MEASURE"), options_p };
            if (doMenuDisplay(menuItems, 10, FIRST_APPLICATION, STATE_ROOT, 1) == MENU_SELECTED)
#endif
            	{
#if defined(TOPLEVEL_BYPASS)
				if (bypass == BYPASS_FIRST_ON)
					toggleBypass(0); // the channel doesn't matter, it'll get ignored
#endif
            	}
#endif
#if defined(__UNO__)
            const char* menuItems[6] = { PSTR("ARPEGGIATOR"), PSTR("STEP SEQUENCER"), PSTR("RECORDER"), PSTR("GAUGE"), PSTR("CONTROLLER"), options_p };
            if (doMenuDisplay(menuItems, 6, FIRST_APPLICATION, STATE_ROOT, 1) == MENU_SELECTED)
            	{
#if defined(TOPLEVEL_BYPASS)
				if (bypass == BYPASS_FIRST_ON)
					toggleBypass(0); // the channel doesn't matter, it'll get ignored
#endif
            	}
#endif
            }
        break;  
#ifdef INCLUDE_ARPEGGIATOR
        case STATE_ARPEGGIATOR:
            {
            stateArpeggiator();
            }
        break;
#endif
#ifdef INCLUDE_STEP_SEQUENCER
        case STATE_STEP_SEQUENCER:
            {
            if (entry)              // we do this because this state is entered just before we exit the entire step sequencer
                {
#ifdef INCLUDE_PROVIDE_RAW_CC
                setParseRawCC(false);
#endif
                }
            stateLoad(STATE_STEP_SEQUENCER_PLAY, STATE_STEP_SEQUENCER_FORMAT, STATE_ROOT, STATE_STEP_SEQUENCER);
            }
        break;
#endif
#ifdef INCLUDE_RECORDER
        case STATE_RECORDER:
            {
            stateLoad(STATE_RECORDER_PLAY, STATE_RECORDER_PLAY, STATE_ROOT, STATE_RECORDER);
            }
        break;
#endif
#ifdef INCLUDE_GAUGE
        case STATE_GAUGE:
            {   
            if (entry) 
                {
                backupOptions = options;
                clearScreen();
                clearBuffer();
                memset(local.gauge.fastMidi, 0, 3);
#ifdef INCLUDE_PROVIDE_RAW_CC
                setParseRawCC(options.gaugeMidiInProvideRawCC);
#endif
                entry = false; 
                }
            else
                {
#ifdef INCLUDE_EXTENDED_GAUGE
                uint8_t dontShowValue = 0;
#endif

                // at present I'm saying (newItem) rather than (newItem == NEW_ITEM)
                // so even the WAIT_FOR_A_SEC stuff gets sent through.  Note sure
                // if testing for (newItem=1) will result in display starvation
                // when every time the display comes up we have a new incomplete
                // CC, NRPN, or RPN.
        
                if (getBufferLength() > 0 && updateDisplay)  // we've got a scrollbuffer loaded.  Won't happen on first update.
                    {
                    clearScreen();
                    scrollBuffer(led, led2);
                    }
                                        
#ifdef INCLUDE_EXTENDED_GAUGE
                if (isUpdated(SELECT_BUTTON, RELEASED))
                    {
                    setParseRawCC(options.gaugeMidiInProvideRawCC = !options.gaugeMidiInProvideRawCC);
                    saveOptions();
                    }
#endif
                if (newItem)
                    {
                    if ((itemType >= MIDI_NOTE_ON))   // It's not fast midi
                        {
                        const char* str = NULL;
                                                    
                        clearScreen();
                        if (itemType < MIDI_CC_7_BIT) // it's not a CC, RPN, or NRPN, and it's not displayable FAST MIDI
                            {
                            clearBuffer(); // so we stop scrolling
                            }

                        switch(itemType)
                            {
                            case MIDI_NOTE_ON:
                                {
                                // Note we can't arrange this and NOTE OFF as a FALL THRU
                                // because writeGaugeNote() overwrites the points that we set
                                // immediately afterwards, so it can't be after them!
                                writeGaugeNote();
                                for(uint8_t i = 0; i < 5; i++)
                                    setPoint(led, i, 1);
                                }
                            break;
                            case MIDI_NOTE_OFF:
                                {
                                writeGaugeNote();
                                }
                            break;
                            case MIDI_AFTERTOUCH:
                                {
                                write3x5GlyphPair(GLYPH_3x5_A, GLYPH_3x5_T);
                                writeShortNumber(led, (uint8_t) itemValue, false);
                                }
                            break;
                            case MIDI_AFTERTOUCH_POLY:
                                {
                                writeGaugeNote();
                                for(uint8_t i = 0; i < 5; i+=2)
                                    setPoint(led, i, 1);
                                }
                            break;
                            case MIDI_PROGRAM_CHANGE:
                                {
                                write3x5GlyphPair(GLYPH_3x5_P, GLYPH_3x5_C);
                                writeShortNumber(led, (uint8_t) itemNumber, false);
                                }
                            break;
                            case MIDI_CC_7_BIT:
                                {
#ifdef INCLUDE_EXTENDED_GAUGE
                                if (options.gaugeMidiInProvideRawCC)
                                    {
                                    clearBuffer(); // so we stop scrolling
                                    writeShortNumber(led2, (uint8_t) itemNumber, true);
                                    writeShortNumber(led, (uint8_t) itemValue, false);
                                    break;
                                    }
                                else if (itemNumber >= 120)             // Channel Mode
                                    {
                                    dontShowValue = true;
                                    switch(itemNumber)
                                        {
                                        case 120:
                                            {
                                            str = PSTR("ALL SOUND OFF");
                                            break;
                                            }
                                        case 121:
                                            {
                                            str = PSTR("RESET ALL CONTROLLERS");
                                            break;
                                            }
                                        case 122:
                                            {
                                            if (itemValue)
                                                str = PSTR("LOCAL ON");
                                            else
                                                str = PSTR("LOCAL OFF");
                                            break;
                                            }
                                        case 123:
                                            {
                                            str = PSTR("ALL NOTES OFF");
                                            break;
                                            }
                                        case 124:
                                            {
                                            str = PSTR("OMNI OFF");
                                            break;
                                            }
                                        case 125:
                                            {
                                            str = PSTR("OMNI ON");
                                            break;
                                            }
                                        case 126:
                                            {
                                            dontShowValue = false;  // we want to see the value (it's the channel)
                                            str = PSTR("MONO ON");
                                            break;
                                            }
                                        case 127:
                                            {
                                            str = PSTR("POLY ON");
                                            break;
                                            }
                                        }
                                    break;
                                    }
#else
                                if (itemNumber >= 120)          // channel mode
                                    {
                                    str = PSTR("CHANNEL MODE");
                                    break;
                                    }
#endif
                                // else we fall thru
                                }
                            // FALL THRU
                            case MIDI_CC_14_BIT:
                                {
                                str = cc_p;
                                }
                            break;
                            case MIDI_NRPN_14_BIT:
                                // FALL THRU
                            case MIDI_NRPN_INCREMENT:
                                // FALL THRU
                            case MIDI_NRPN_DECREMENT:
                                {
                                str = nrpn_p;
                                }
                            break;
                            case MIDI_RPN_14_BIT:
                                // FALL THRU
                            case MIDI_RPN_INCREMENT:
                                // FALL THRU
                            case MIDI_RPN_DECREMENT:
                                {
                                str = rpn_p;
                                }
                            break;
                            case MIDI_PITCH_BEND:
                                {
                                writeNumber(led, led2, ((int16_t) itemValue) - 8192);           // pitch bend is actually signed
                                }
                            break;
                            case MIDI_SYSTEM_EXCLUSIVE: 
                            case MIDI_SONG_POSITION:
                            case MIDI_SONG_SELECT: 
                            case MIDI_TUNE_REQUEST:
                            case MIDI_START: 
                            case MIDI_CONTINUE:
                            case MIDI_STOP:
                            case MIDI_SYSTEM_RESET: 
                                {
                                write3x5Glyphs(itemType - MIDI_SYSTEM_EXCLUSIVE + GLYPH_SYSTEM_RESET);
                                }
                            break;
                            }
                                
                        if (str != NULL)
                            {           
                            char b[5];
                                                                                        
                            clearBuffer();
                                                                
                            // If we're incrementing/decrementing, add UP or DOWN
                            if ((itemType >= MIDI_NRPN_INCREMENT))
                                {
                                addToBuffer("   ");
                                if (itemType >= MIDI_NRPN_DECREMENT)
                                    {
                                    strcpy_P(b, PSTR("-"));
                                    }
                                else
                                    {
                                    strcpy_P(b, PSTR("+"));
                                    }
                                addToBuffer(b);
                                }
                                                                
                            // else if we're 7-bit CC, just add the value
                            else if (itemType == MIDI_CC_7_BIT)
                                {
#ifdef INCLUDE_EXTENDED_GAUGE
                                if (!dontShowValue)
#endif                                                          
                                    addGaugeNumberNoTrim(itemValue);
                                }
                                                                
                            // else add the MSB
                            else
                                {
                                addGaugeNumberNoTrim(itemValue >> 7);
                                }
                                                                
                            // Next load the name
#ifdef INCLUDE_EXTENDED_GAUGE
                            if (!dontShowValue)
#endif                                                          
                                addToBuffer(" "); 
                            strcpy_P(b, str);
                            addToBuffer(b);
                                                                
#ifdef INCLUDE_EXTENDED_GAUGE
                            if (itemType != MIDI_CC_7_BIT || itemNumber < 120)  // we don't want channel mode drawing here
                                {       
#endif                                                          
                                // Next the number
                                addToBuffer(" ");
                                addGaugeNumber(itemNumber);
                                                                                                                                        
                                if (itemType != MIDI_CC_7_BIT)          // either we indicate how much we increment/decrement, or show the full 14-bit number
                                    {
                                    addToBuffer(" (");
                                    addGaugeNumber(itemValue);                                      
                                    addToBuffer(")");
                                    }
#ifdef INCLUDE_EXTENDED_GAUGE
                                }
#endif                                                          
                            }
                        }
                    else                    // Fast MIDI
                        {
                        local.gauge.fastMidi[itemType] = !local.gauge.fastMidi[itemType];
                        }

                    if (newItem == WAIT_FOR_A_SEC)
                        newItem = NEW_ITEM;
                    else
                        newItem = NO_NEW_ITEM;
                    }
                }

            if (updateDisplay)
                {
                // blink the fast MIDI stuff
                for(uint8_t i = 0; i < 3; i++)
                    {
                    // slightly inefficient but it gets us under the byte limit
                    clearPoint(led, i + 5, 1);
                    if (local.gauge.fastMidi[i])
                        setPoint(led, i + 5, 1);
                    }
                drawMIDIChannel(itemChannel);

#ifdef INCLUDE_EXTENDED_GAUGE
                if (options.gaugeMidiInProvideRawCC)
                    setPoint(led, 5, 1);
                else
                    clearPoint(led, 5, 1);
#endif

                // At any rate...                
                // Clear the bypass/beat so it can draw itself again,
                // because we don't update ourselves every single time 
                for(uint8_t i = 0; i < 8; i++)
                    clearPoint(led, i, 0);          
                }
     

            if (isUpdated(BACK_BUTTON, RELEASED))
                {
#ifdef INCLUDE_PROVIDE_RAW_CC
                setParseRawCC(false);
#endif
                goUpStateWithBackup(STATE_ROOT);
                }
            }
        break;
#endif        
#ifdef INCLUDE_CONTROLLER
        case STATE_CONTROLLER:
            {
            if (entry)
                MIDI.sendRealTime(MIDIClock);
#ifdef INCLUDE_EXTENDED_CONTROLLER:
            const char* menuItems[9] = { PSTR("GO"), PSTR("L KNOB"), PSTR("R KNOB"), PSTR("M BUTTON"), PSTR("R BUTTON"), PSTR("WAVE"), PSTR("RANDOM"), PSTR("A2"), PSTR("A3"),  };
            doMenuDisplay(menuItems, 9, STATE_CONTROLLER_PLAY, STATE_ROOT, 1);
#else
            const char* menuItems[5] = { PSTR("GO"), PSTR("L KNOB"), PSTR("R KNOB"), PSTR("M BUTTON"), PSTR("R BUTTON") };
            doMenuDisplay(menuItems, 5, STATE_CONTROLLER_PLAY, STATE_ROOT, 1);
#endif
            }
        break;
#endif
#ifdef INCLUDE_SPLIT
        case STATE_SPLIT:
            {
            stateSplit();
            }
        break;
#endif
#ifdef INCLUDE_THRU
        case STATE_THRU:
            {
            const char* menuItems[7] = { PSTR("GO"), PSTR("EXTRA NOTES"), PSTR("DISTRIBUTE NOTES"), options.thruChordMemorySize == 0 ? PSTR("CHORD MEMORY") : PSTR("NO CHORD MEMORY"), PSTR("DEBOUNCE"), PSTR("MERGE CHANNEL"), /* options.thruCCToNRPN ? PSTR("NO CC-NRPN") : PSTR("CC-NRPN") ,*/ options.thruBlockOtherChannels ? PSTR("UNBLOCK OTHERS") :  PSTR("BLOCK OTHERS") };
            doMenuDisplay(menuItems, 7, STATE_THRU_PLAY, STATE_ROOT, 1);
            }
        break;
#endif
#ifdef INCLUDE_SYNTH
        case STATE_SYNTH:
            {
            if (entry)
            	{
				for(uint8_t i = 0; i < 25; i++)
					local.synth.passMIDIData[i] = true;
            	entry = false;
            	}
            
            local.synth.countDown = 0;
            local.synth.parameterDisplay = DISPLAY_NOTHING;
            
            const char* menuItems[6] = { 
#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD
                PSTR("WALDORF BLOFELD"),
#endif INCLUDE_SYNTH_WALDORF_BLOFELD
#ifdef INCLUDE_SYNTH_KAWAI_K4
                PSTR("KAWAI K4"),
#endif INCLUDE_SYNTH_KAWAI_K4
#ifdef INCLUDE_SYNTH_KAWAI_K5
                PSTR("KAWAI K5"),
#endif INCLUDE_SYNTH_KAWAI_K5
#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
                PSTR("OBERHEIM MATRIX 1000"),
#endif INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
#ifdef INCLUDE_SYNTH_KORG_MICROSAMPLER
                PSTR("KORG MICROSAMPLER"),
#endif INCLUDE_SYNTH_KORG_MICROSAMPLER
#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z
                PSTR("YAMAHA TX81Z"),
#endif INCLUDE_SYNTH_YAMAHA_TX81Z
                };
            doMenuDisplay(menuItems, 4, STATE_SYNTH_WALDORF_BLOFELD, STATE_ROOT, 1);
            }
        break;
#endif
#ifdef INCLUDE_MEASURE
        case STATE_MEASURE:
            {
            stateMeasure();
            }
        break;
#endif

#ifdef INCLUDE_SYSEX
        case STATE_SYSEX:
            {
            // make sure that we're reset properly
            local.sysex.slot = NO_SYSEX_SLOT;
            
            const char* menuItems[2] = { PSTR("SLOT"), PSTR("ARPEGGIO") };
            doMenuDisplay(menuItems, 2, STATE_SYSEX_SLOT, STATE_ROOT, 1);
            }
        break;
        
        case STATE_SYSEX_SLOT:
            {
            local.sysex.type = SYSEX_TYPE_SLOT;
            
            // make sure that we're reset properly
            local.sysex.received = RECEIVED_NONE;
            uint8_t result = doNumericalDisplay(0, NUM_SLOTS - 1, 1, false, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    break;
                case MENU_SELECTED:
                	local.sysex.slot = currentDisplay;
                	goDownState(STATE_SYSEX_GO);
                	break;
                case MENU_CANCELLED:
                	goUpState(STATE_SYSEX);
                	break;
                }
            }
        break;
        
        case STATE_SYSEX_ARP:
            {
            local.sysex.type = SYSEX_TYPE_ARP;

            // make sure that we're reset properly
            local.sysex.received = RECEIVED_NONE;
            uint8_t result = doNumericalDisplay(0, NUM_ARPS - 1, 1, false, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    break;
                case MENU_SELECTED:
                	local.sysex.slot = currentDisplay;
                	goDownState(STATE_SYSEX_GO);
                	break;
                case MENU_CANCELLED:
                	goUpState(STATE_SYSEX);
                	break;
                }
            }
        break;

        case STATE_SYSEX_GO:
            {
            // display
            if (local.sysex.received == RECEIVED_NONE)
            	{
				clearScreen();  // is this necessary?
            	write3x5Glyphs(GLYPH_OFF);
            	}
            else if (local.sysex.received == RECEIVED_WRONG)
            	{
				clearScreen();  // is this necessary?
            	write3x5Glyphs(GLYPH_SYSEX);
            	}
            else if (local.sysex.received == RECEIVED_BAD)
            	{
				clearScreen();  // is this necessary?
            	write3x5Glyphs(GLYPH_FAIL);
            	}
            else
            	{
				clearScreen();
				writeShortNumber(led, ((uint8_t)local.sysex.received), false);
            	}
            	
            // handle buttons
            if (isUpdated(BACK_BUTTON, RELEASED))
            	{
            	goUpState(local.sysex.type == SYSEX_TYPE_SLOT ? STATE_SYSEX_SLOT : STATE_SYSEX_ARP);
            	}
            else if (isUpdated(SELECT_BUTTON, PRESSED))
            	{
            	if (local.sysex.type == SYSEX_TYPE_SLOT)
            		{
            		sendSlotSysex();
            		}
            	else
            		{
            		sendArpSysex();
            		}
				local.sysex.received++;
				if (local.sysex.received <= 0)  // previous was BAD or WRONG, or we wrapped around
					local.sysex.received = 1;
            	}
            }
        break;
#endif


        case STATE_OPTIONS:
            {
#ifdef INCLUDE_CLOCK_IN_OPTIONS
            if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
                {
                if (getClockState() == CLOCK_RUNNING)
                    {
                    stopClock(true);
                    }
                else
                    {
                    startClock(true);
                    }
                }
                
            else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
                {
                if (getClockState() == CLOCK_RUNNING)
                    {
                    stopClock(true);
                    }
                else
                    {
                    continueClock(true);
                    }
                }
#endif
                        
#if defined(__MEGA__)
            const char* menuItems[15] = { PSTR("TEMPO"), PSTR("NOTE SPEED"), PSTR("SWING"), PSTR("TRANSPOSE"), 
                                          PSTR("VOLUME"), PSTR("LENGTH"), PSTR("IN MIDI"), PSTR("OUT MIDI"), PSTR("CONTROL MIDI"), PSTR("CLOCK"), PSTR("DIVIDE"),
                                          ((options.click == NO_NOTE) ? PSTR("CLICK") : PSTR("NO CLICK")),
                                          PSTR("BRIGHTNESS"), 
                                          PSTR("MENU DELAY"),
                                          PSTR("GIZMO V6 (C) 2018 SEAN LUKE") };
            doMenuDisplay(menuItems, 15, STATE_OPTIONS_TEMPO, optionsReturnState, 1);
#endif
#if defined(__UNO__)
            const char* menuItems[11] = { PSTR("TEMPO"), PSTR("NOTE SPEED"), PSTR("SWING"), 
                                          PSTR("LENGTH"), PSTR("IN MIDI"), PSTR("OUT MIDI"), PSTR("CONTROL MIDI"), PSTR("CLOCK"), 
                                          ((options.click == NO_NOTE) ? PSTR("CLICK") : PSTR("NO CLICK")),
                                          PSTR("BRIGHTNESS"),
                                          PSTR("GIZMO V6 (C) 2018 SEAN LUKE") };
            doMenuDisplay(menuItems, 11, STATE_OPTIONS_TEMPO, optionsReturnState, 1);
#endif

            playApplication(); 
            }
        break;

        
        case STATE_UNDEFINED_1:
            // FALL THRU
        case STATE_UNDEFINED_2:
            // FALL THRU
        case STATE_UNDEFINED_3:
            // FALL THRU
        case STATE_UNDEFINED_4:
            // FALL THRU
        case STATE_UNDEFINED_5:
            // FALL THRU
        case STATE_UNDEFINED_6:
            // FALL THRU
        case STATE_UNDEFINED_7:
            // FALL THRU
        case STATE_UNDEFINED_8:
            // FALL THRU
        case STATE_UNDEFINED_9:
            // FALL THRU
        case STATE_UNDEFINED_10:
            // FALL THRU
        case STATE_UNDEFINED_11:
            // FALL THRU
        case STATE_UNDEFINED_12:
            {
            goUpState(STATE_ROOT);
            }
        break;


#ifdef INCLUDE_ARPEGGIATOR
        case STATE_ARPEGGIATOR_PLAY:
            {
            stateArpeggiatorPlay();
            }
        break;
        case STATE_ARPEGGIATOR_PLAY_OCTAVES:
            {
            stateNumerical(0, ARPEGGIATOR_MAX_OCTAVES, options.arpeggiatorPlayOctaves, backupOptions.arpeggiatorPlayOctaves, true, false, GLYPH_NONE,  STATE_ARPEGGIATOR_PLAY);
            playArpeggio();
            }
        break;
        case STATE_ARPEGGIATOR_PLAY_VELOCITY:
            {
            // 128 represents FREE key velocity
            stateNumerical(0, 128, options.arpeggiatorPlayVelocity, backupOptions.arpeggiatorPlayVelocity, false, false, GLYPH_FREE, STATE_ARPEGGIATOR_PLAY);
            playArpeggio();
            }
        break;
        case STATE_ARPEGGIATOR_MENU:
            {
            stateArpeggiatorMenu();
            }
        break;
        case STATE_ARPEGGIATOR_CREATE:
            {
			uint8_t note = stateEnterNote(STATE_ARPEGGIATOR);
			if (note != NO_NOTE)  // it's a real note
				{
				data.arp.root = note;
				state = STATE_ARPEGGIATOR_CREATE_EDIT;
				entry = true;
				}
            }
        break;
        case STATE_ARPEGGIATOR_CREATE_EDIT:
            {
            stateArpeggiatorCreateEdit();
            }
        break;
        case STATE_ARPEGGIATOR_CREATE_SAVE:
            {
            stateArpeggiatorCreateSave();
            }
        break;
        case STATE_ARPEGGIATOR_CREATE_SURE:
            {
            stateSure(STATE_ARPEGGIATOR_CREATE_EDIT, STATE_ARPEGGIATOR);
            }
        break;
#endif

#ifdef INCLUDE_EXTENDED_ARPEGGIATOR
        case STATE_ARPEGGIATOR_PLAY_ALONG:
            {
            // 0 is DEFAULT
            stateNumerical(0, 16, options.arpeggiatorPlayAlongChannel, backupOptions.arpeggiatorPlayAlongChannel, true, false, GLYPH_NONE,  STATE_ARPEGGIATOR_MENU);
            playArpeggio();
            }
        break;
#endif


#ifdef INCLUDE_STEP_SEQUENCER
        case STATE_STEP_SEQUENCER_FORMAT:
            {
            stateStepSequencerFormat();
            }
        break;
        case STATE_STEP_SEQUENCER_PLAY:
            {
            stateStepSequencerPlay();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU:
            {
            stateStepSequencerMenu();
            }
        break;
        case STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT:
            {
            if (entry)
                clearNotesOnTracks(true);
                
#ifdef INCLUDE_IMMEDIATE_RETURN
            // 17 represents DEFAULT channel
            uint8_t val = stateNumerical(0, 17, local.stepSequencer.outMIDI[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, true, GLYPH_DEFAULT, 
            	immediateReturn ? STATE_STEP_SEQUENCER_PLAY : STATE_STEP_SEQUENCER_MENU);
#else
            uint8_t val = stateNumerical(0, 17, local.stepSequencer.outMIDI[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, true, GLYPH_DEFAULT, STATE_STEP_SEQUENCER_MENU);
#endif
            if (val != NO_STATE_NUMERICAL_CHANGE)
                sendAllSoundsOff();
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_VELOCITY:
            {
#ifdef INCLUDE_IMMEDIATE_RETURN
            // 0 represents FREE velocity
            stateNumerical(0, 127, local.stepSequencer.velocity[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, true, GLYPH_NONE,
            	immediateReturn ? STATE_STEP_SEQUENCER_PLAY : STATE_STEP_SEQUENCER_MENU);
#else
            stateNumerical(0, 127, local.stepSequencer.velocity[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, true, GLYPH_NONE, STATE_STEP_SEQUENCER_MENU);
#endif
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_FADER:
            {
#ifdef INCLUDE_IMMEDIATE_RETURN
            stateNumerical(0, 31, local.stepSequencer.fader[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_NONE, 
            	immediateReturn ? STATE_STEP_SEQUENCER_PLAY : STATE_STEP_SEQUENCER_MENU);
#else
            stateNumerical(0, 31, local.stepSequencer.fader[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_NONE, 
            	STATE_STEP_SEQUENCER_MENU);
#endif
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_LENGTH:
            {
            // 101 represents DEFAULT length
#ifdef INCLUDE_IMMEDIATE_RETURN
            stateNumerical(0, 101, local.stepSequencer.noteLength[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_DEFAULT,
            	immediateReturn ? STATE_STEP_SEQUENCER_PLAY : STATE_STEP_SEQUENCER_MENU);
#else
            stateNumerical(0, 101, local.stepSequencer.noteLength[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_DEFAULT, STATE_STEP_SEQUENCER_MENU);
#endif
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_SURE:
            {
            stateSure(STATE_STEP_SEQUENCER_PLAY, STATE_STEP_SEQUENCER);
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_SAVE:
            {
            stateSave(STATE_STEP_SEQUENCER_PLAY);
            playStepSequencer();
            }
        break;
#endif

#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
        case STATE_STEP_SEQUENCER_MENU_TYPE:
            {
            stateStepSequencerMenuType();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_TYPE_PARAMETER:
            {
            stateStepSequencerMenuTypeParameter();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_PATTERN:
            {
            stateStepSequencerMenuPattern();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_EDIT:
            {
            const char* menuItems[5] = { PSTR("MARK"), PSTR("COPY"), PSTR("SPLAT"), PSTR("MOVE"), PSTR("DUPLICATE") };
            doMenuDisplay(menuItems, 5, STATE_STEP_SEQUENCER_MENU_EDIT_MARK, STATE_STEP_SEQUENCER_MENU, 1);
		    playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_EDIT_MARK:
            {
            stateStepSequencerMenuEditMark();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_EDIT_COPY:
            {
            stateStepSequencerMenuEditCopy(false, false);
			playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_EDIT_SPLAT:
            {
            stateStepSequencerMenuEditCopy(true, false);
			playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_EDIT_MOVE:
            {
            stateStepSequencerMenuEditCopy(false, true);
			playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_EDIT_DUPLICATE:
        	{
            stateStepSequencerMenuEditDuplicate();
			playStepSequencer();
        	}
        case STATE_STEP_SEQUENCER_MENU_PERFORMANCE:
            {
            const char* menuItems[3] = { PSTR("KEYBOARD"), PSTR("REPEAT SEQUENCE"), PSTR("NEXT SEQUENCE") };
            doMenuDisplay(menuItems, 3, STATE_STEP_SEQUENCER_MENU_PERFORMANCE_KEYBOARD, STATE_STEP_SEQUENCER_MENU, 1);
		    playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_PERFORMANCE_KEYBOARD:
            {
			stateStepSequencerMenuPerformanceKeyboard();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_PERFORMANCE_REPEAT:
            {
			stateStepSequencerMenuPerformanceRepeat();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_PERFORMANCE_NEXT:
            {
			stateStepSequencerMenuPerformanceNext();
            }
        break;
        case STATE_STEP_SEQUENCER_MENU_NO:
            {
            const char* menuItems[1] = { PSTR("NO") };
            doMenuDisplay(menuItems, 1, immediateReturn ? STATE_STEP_SEQUENCER_PLAY : STATE_STEP_SEQUENCER_MENU, immediateReturn ? STATE_STEP_SEQUENCER_PLAY : STATE_STEP_SEQUENCER_MENU, 1);
		    playStepSequencer();
            }
        break;
#endif
       
#ifdef INCLUDE_RECORDER
        case STATE_RECORDER_FORMAT:
            {
            data.slot.data.recorder.length = 0;
            }
        // Fall Thru!
        case STATE_RECORDER_PLAY:
            {
            stateRecorderPlay();
            }
        break;
        case STATE_RECORDER_SAVE:
            {
            stateSave(STATE_RECORDER_PLAY);
            }
        break;
        case STATE_RECORDER_SURE:
            {
            stateSure(STATE_RECORDER_PLAY, STATE_RECORDER);
            }
        break;
#endif

#ifdef INCLUDE_EXTENDED_RECORDER
        case STATE_RECORDER_MENU:
            {
            stateRecorderMenu();
            }
        break;
#endif

#ifdef INCLUDE_CONTROLLER
        case STATE_CONTROLLER_PLAY:
            {
            if (entry)
                {
                local.control.middleButtonToggle = 0;
                local.control.selectButtonToggle = 0;
                local.control.displayValue = -1;
                local.control.displayType = CONTROL_TYPE_OFF;
#ifdef INCLUDE_EXTENDED_CONTROLLER
				local.control.potWaiting[0] = 0;
				local.control.potWaiting[1] = 0;
				local.control.potWaiting[2] = 0;
				local.control.potWaiting[3] = 0;
#endif
                entry = false;
                dontBypassOut = true;
    			// update bypassOut on entry
    			bypassOut = (bypass && !dontBypassOut);
                }

            if (isUpdated(BACK_BUTTON, RELEASED))
                {
                dontBypassOut = false;
    			// update bypassOut on exit
    			bypassOut = (bypass && !dontBypassOut);
                goUpState(STATE_CONTROLLER);
                }
            else
                {
                // this region is redundant but simplifying to a common function call makes the code bigger 
        
                if (isUpdated(MIDDLE_BUTTON, PRESSED))
                    {
                    local.control.middleButtonToggle = !local.control.middleButtonToggle;
                    if (options.middleButtonControlType != CONTROL_TYPE_OFF)
                        {
                        local.control.displayValue = ((local.control.middleButtonToggle ? options.middleButtonControlOn : options.middleButtonControlOff));
                        
#ifdef INCLUDE_EXTENDED_CONTROLLER
                        if (options.middleButtonControlType == CONTROL_TYPE_PITCH_BEND)
                            {
                            if (local.control.displayValue != 0) // if it's not "off"
                                {
                                local.control.displayValue--;
                                local.control.displayType = options.middleButtonControlType;
                                }
                            else
                                {
                                local.control.displayType = CONTROL_TYPE_OFF;
                                }
                            }
                        else
#endif
                            {
                            // at this point local.control.displayValue is 0...129, where 0 is off and 129 is "INCREMENT", and 1...128 is 0...127
                            if (local.control.displayValue != 0) // if it's not "off"
                                {
                                local.control.displayValue--;  // map to 0...128, where 128 is "INCREMENT"

                                // now convert INCREMENT to DECREMENT
                                if (local.control.displayValue == CONTROL_VALUE_INCREMENT)
                                    local.control.displayValue++;
                                                                
                                // Now move to MSB+LSB
                                local.control.displayValue = local.control.displayValue << 7;
                                                                
                                sendControllerCommand( local.control.displayType = options.middleButtonControlType, options.middleButtonControlNumber, local.control.displayValue, options.channelOut);
                                                        
                                }
                            else
                                local.control.displayType = CONTROL_TYPE_OFF;
                            }
                        }
                    }

                if (isUpdated(SELECT_BUTTON, PRESSED))
                    {
                    local.control.selectButtonToggle = !local.control.selectButtonToggle;
                    if (options.selectButtonControlType != CONTROL_TYPE_OFF)
                        {
                        local.control.displayValue = ((local.control.selectButtonToggle ?  options.selectButtonControlOn :  options.selectButtonControlOff));
                            
#ifdef INCLUDE_EXTENDED_CONTROLLER

                        if (options.selectButtonControlType == CONTROL_TYPE_PITCH_BEND)
                            {
                            if (local.control.displayValue != 0) // if it's not "off"
                                {
                                local.control.displayValue--;
                                local.control.displayType = options.selectButtonControlType;
                                }
                            else
                                {
                                local.control.displayType = CONTROL_TYPE_OFF;
                                }
                            }
                        else
#endif
                            {
                            // at this point local.control.displayValue is 0...129, where 0 is off and 129 is "INCREMENT", and 1...128 is 0...127
                            if (local.control.displayValue != 0)    // if we're not OFF
                                {
                                local.control.displayValue--;  // map to 0...128, where 128 is "INCREMENT"
                                                                
                                // Now move to MSB+LSB
                                local.control.displayValue = local.control.displayValue << 7;
                                                                
                                sendControllerCommand( local.control.displayType = options.selectButtonControlType, options.selectButtonControlNumber, local.control.displayValue, options.channelOut); 
                                }
                            else
                                local.control.displayType = CONTROL_TYPE_OFF;
                            }
                        }
                    }
        

#ifdef INCLUDE_EXTENDED_CONTROLLER
                if (potUpdated[LEFT_POT] && (options.leftKnobControlType != CONTROL_TYPE_OFF))
                    {
                    local.control.displayValue = pot[LEFT_POT];
                    // at this point local.control.displayValue is 0...1023
            
                    // Now move to MSB+LSB
                    local.control.displayValue = local.control.displayValue << 4;
                    
                    if (local.control.potUpdateValue[LEFT_POT] != local.control.displayValue)
                    	{
                    	local.control.potUpdateValue[LEFT_POT] = local.control.displayValue;
	                    local.control.potWaiting[LEFT_POT] = 1;
	                    }
                    }
          
                if (potUpdated[RIGHT_POT] && (options.rightKnobControlType != CONTROL_TYPE_OFF))
                    {
                    local.control.displayValue = pot[RIGHT_POT];            
                    // at this point local.control.displayValue is 0...1023

                    // Now move to MSB+LSB
                    local.control.displayValue = local.control.displayValue << 4;
                    
                    if (local.control.potUpdateValue[RIGHT_POT] != local.control.displayValue)
                    	{
   	                 	local.control.potUpdateValue[RIGHT_POT] = local.control.displayValue;
   	                 	local.control.potWaiting[RIGHT_POT] = 1;
						}
                    }
	
                 if (potUpdated[A2_POT] && (options.a2ControlType != CONTROL_TYPE_OFF))
                    {
                    local.control.displayValue = pot[A2_POT];            
                    // at this point local.control.displayValue is 0...1023

                    // Now move to MSB+LSB
                    local.control.displayValue = local.control.displayValue << 4;
                    
                    if (local.control.potUpdateValue[A2_POT] != local.control.displayValue)
                    	{
                    	local.control.potUpdateValue[A2_POT] = local.control.displayValue;
                    	local.control.potWaiting[A2_POT] = 1;
                    	}
                    }

                if (potUpdated[A3_POT] && (options.a3ControlType != CONTROL_TYPE_OFF))
                    {
                    local.control.displayValue = pot[A3_POT];            
                    // at this point local.control.displayValue is 0...1023

                    // Now move to MSB+LSB
                    local.control.displayValue = local.control.displayValue << 4;

                    if (local.control.potUpdateValue[A3_POT] != local.control.displayValue)
                    	{
                    	local.control.potUpdateValue[A3_POT] = local.control.displayValue;
                    	local.control.potWaiting[A3_POT] = 1;
                    	}
                    }

				// figure out who has been waiting the longest, if any.  The goal here is to only allow one out at a time and yet prevent starvation

				int8_t winner = -1;
				uint32_t winnerTime = 0;
				if (local.control.potWaiting[LEFT_POT] && (currentTime - local.control.potUpdateTime[LEFT_POT] >= MINIMUM_CONTROLLER_POT_DELAY))
					{
					if (local.control.potUpdateTime[LEFT_POT] - currentTime > winnerTime) { winner = LEFT_POT; winnerTime = currentTime - local.control.potUpdateTime[LEFT_POT]; }
					}

                if (local.control.potWaiting[RIGHT_POT] && (currentTime - local.control.potUpdateTime[RIGHT_POT] >= MINIMUM_CONTROLLER_POT_DELAY))
					{
					if (local.control.potUpdateTime[RIGHT_POT] - currentTime > winnerTime) { winner = RIGHT_POT; winnerTime = currentTime - local.control.potUpdateTime[RIGHT_POT]; }
					}

                if (local.control.potWaiting[A2_POT] && (currentTime - local.control.potUpdateTime[A2_POT] >= MINIMUM_CONTROLLER_POT_DELAY))
					{
					if (local.control.potUpdateTime[A2_POT] - currentTime > winnerTime) { winner = A2_POT; winnerTime = currentTime - local.control.potUpdateTime[A2_POT]; }
					}

                if (local.control.potWaiting[A3_POT] && (currentTime - local.control.potUpdateTime[A3_POT] >= MINIMUM_CONTROLLER_POT_DELAY))
					{
					if (local.control.potUpdateTime[A3_POT] - currentTime > winnerTime) { winner = A3_POT; winnerTime = currentTime - local.control.potUpdateTime[A3_POT]; }
					}
					
				// here we go
                if (winner == LEFT_POT)
                    	{
                    	sendControllerCommand( local.control.displayType = options.leftKnobControlType, options.leftKnobControlNumber, local.control.potUpdateValue[LEFT_POT], options.channelOut);
                    	local.control.potUpdateTime[LEFT_POT] = currentTime;
                    	local.control.potWaiting[LEFT_POT] = 0;
						}
                else if (winner == RIGHT_POT)
                    	{
                    	sendControllerCommand( local.control.displayType = options.rightKnobControlType, options.rightKnobControlNumber, local.control.potUpdateValue[RIGHT_POT], options.channelOut);
                    	local.control.potUpdateTime[RIGHT_POT] = currentTime;
                    	local.control.potWaiting[RIGHT_POT] = 0;
						}
                else if (winner == A2_POT)
                    	{
                    	sendControllerCommand( local.control.displayType = options.a2ControlType, options.a2ControlNumber, local.control.potUpdateValue[A2_POT], options.channelOut);
                    	local.control.potUpdateTime[A2_POT] = currentTime;
                    	local.control.potWaiting[A2_POT] = 0;
						}
                else if (winner == A3_POT)
                    	{
                    	sendControllerCommand( local.control.displayType = options.a3ControlType, options.a3ControlNumber, local.control.potUpdateValue[A3_POT], options.channelOut);
                    	local.control.potUpdateTime[A3_POT] = currentTime;
                    	local.control.potWaiting[A3_POT] = 0;
						}
                
#else
                if (potUpdated[LEFT_POT] && (options.leftKnobControlType != CONTROL_TYPE_OFF))
                    {
                    local.control.displayValue = pot[LEFT_POT];
                    // at this point local.control.displayValue is 0...1023
            
                    // Now move to MSB+LSB
                    local.control.displayValue = local.control.displayValue << 4;
                    
                    sendControllerCommand( local.control.displayType = options.leftKnobControlType, options.leftKnobControlNumber, local.control.displayValue, options.channelOut);
                    }
          
                if (potUpdated[RIGHT_POT] && (options.rightKnobControlType != CONTROL_TYPE_OFF))
                    {
                    local.control.displayValue = pot[RIGHT_POT];            
                    // at this point local.control.displayValue is 0...1023

                    // Now move to MSB+LSB
                    local.control.displayValue = local.control.displayValue << 4;
                    
                    sendControllerCommand( local.control.displayType = options.rightKnobControlType, options.rightKnobControlNumber, local.control.displayValue, options.channelOut); 
                    }
#endif
                }
   
            if (updateDisplay)
                {
                clearScreen();
                
                // local.control.displayValue is now -1, meaning "OFF",
                // or it is a value in the range of MSB + LSB

                if (local.control.displayType != CONTROL_TYPE_OFF)  // isn't "off"
                    {
                    uint8_t msb = (uint8_t)(local.control.displayValue >> 7);
                                                                                        
                    // if we needed a little bit more space, we could change this to something like
                    // write3x5Glyphs(msb - CONTROL_VALUE_INCREMENT + GLYPH_INCREMENT);
                    // except that GLYPH_INCREMENT comes SECOND.  We'd need to fix all that to make it
                    // consistent.  It'd save us about 20 bytes maybe?
                    if (msb == CONTROL_VALUE_INCREMENT)
                        {
                        write3x5Glyphs(GLYPH_INCREMENT);
                        }
                    else if (msb == CONTROL_VALUE_DECREMENT)
                        {
                        write3x5Glyphs(GLYPH_DECREMENT);
                        }
                    else
                        {
#ifdef INCLUDE_EXTENDED_CONTROLLER
                        if (local.control.displayType == CONTROL_TYPE_PITCH_BEND)
                            {
                            writeNumber(led, led2, ((int16_t)(local.control.displayValue)) + (int16_t)(MIDI_PITCHBEND_MIN));
                            }
                        else
#endif
                            writeShortNumber(led, msb, false);
                        }
                    }
                else
                	{
                	write3x5Glyphs(GLYPH_OFF);
                	}
                }
            }
        break;
        case STATE_CONTROLLER_SET_LEFT_KNOB_TYPE:
            {
            setControllerType(options.leftKnobControlType, STATE_CONTROLLER_SET_LEFT_KNOB_NUMBER, STATE_NONE);
            }
        break;
        case STATE_CONTROLLER_SET_RIGHT_KNOB_TYPE:
            {
            setControllerType(options.rightKnobControlType, STATE_CONTROLLER_SET_RIGHT_KNOB_NUMBER, STATE_NONE);
            }
        break;
        case STATE_CONTROLLER_SET_MIDDLE_BUTTON_TYPE:
            {
            setControllerType(options.middleButtonControlType, STATE_CONTROLLER_SET_MIDDLE_BUTTON_NUMBER, STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_ON);
            }
        break;
        case STATE_CONTROLLER_SET_SELECT_BUTTON_TYPE:
            {
            setControllerType(options.selectButtonControlType, STATE_CONTROLLER_SET_SELECT_BUTTON_NUMBER, STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON);
            }
        break;
#ifdef INCLUDE_EXTENDED_CONTROLLER
        case STATE_CONTROLLER_WAVE_ENVELOPE:
            {
            const char* menuItems[5] = { PSTR("GO"), PSTR("CONTROL"), PSTR("ENVELOPE"), PSTR("MODE"), options.controlModulationClocked ? PSTR("UNSYNC") : PSTR("SYNC") };
            doMenuDisplay(menuItems, 5, STATE_CONTROLLER_PLAY_WAVE_ENVELOPE, STATE_CONTROLLER, 1);
            }
        break;
        case STATE_CONTROLLER_RANDOM:
            {
            const char* menuItems[7] = { PSTR("GO"), PSTR("CONTROL"), PSTR("MODE"), PSTR("RANGE"), PSTR("INITIAL VALUE"), PSTR("LENGTH"), options.controlRandomClocked ? PSTR("UNSYNC") : PSTR("SYNC") };
            doMenuDisplay(menuItems, 7, STATE_CONTROLLER_PLAY_RANDOM, STATE_CONTROLLER, 1);
            }
        break;
        case STATE_CONTROLLER_SET_A2_TYPE:
            {
            setControllerType(options.a2ControlType, STATE_CONTROLLER_SET_A2_NUMBER, STATE_NONE);
            }
        break;
        case STATE_CONTROLLER_SET_A3_TYPE:
            {
            setControllerType(options.a3ControlType, STATE_CONTROLLER_SET_A3_NUMBER, STATE_NONE);
            }
        break;
        case STATE_CONTROLLER_PLAY_WAVE_ENVELOPE:
            {
            stateControllerPlayWaveEnvelope();
            }
        break;
        case STATE_CONTROLLER_SET_WAVE_ENVELOPE_TYPE:
            {
            setControllerType(options.waveControlType, STATE_CONTROLLER_SET_WAVE_ENVELOPE_NUMBER, STATE_NONE);
            }
        break;
        case STATE_CONTROLLER_SET_WAVE_ENVELOPE:
            {
            stateControllerSetWaveEnvelope();
            }
        break;
        case STATE_CONTROLLER_WAVE_ENVELOPE_SET_MODE:
            {
            stateControllerModulationSetMode();
            }
        break;
        case STATE_CONTROLLER_WAVE_ENVELOPE_SET_CLOCK:
            {
            options.controlModulationClocked = !options.controlModulationClocked;
            saveOptions();
            goUpState(STATE_CONTROLLER_WAVE_ENVELOPE);
            }
        break;
        case STATE_CONTROLLER_SET_WAVE_ENVELOPE_VALUE:
            {
            stateControllerSetWaveEnvelopeValue();
            }
        break;
        case STATE_CONTROLLER_RESET_WAVE_ENVELOPE_VALUES:
            {
            stateSure(STATE_CONTROLLER_RESET_WAVE_ENVELOPE_VALUES_GO, STATE_CONTROLLER_SET_WAVE_ENVELOPE_VALUE);
            }
        break;
        case STATE_CONTROLLER_RESET_WAVE_ENVELOPE_VALUES_GO:
            {
            stateControllerResetWaveEnvelopeValuesGo();
            }
        break;
        case STATE_CONTROLLER_SET_WAVE_ENVELOPE_NUMBER:
            {
            setControllerNumber(options.waveControlType, options.waveControlNumber, backupOptions.waveControlType, backupOptions.waveControlNumber, STATE_CONTROLLER_WAVE_ENVELOPE);
            }
        break;
        case STATE_CONTROLLER_PLAY_RANDOM:
            {
            stateControllerPlayRandom();
            }
        break;
        case STATE_CONTROLLER_RANDOM_SET_TYPE:
            {
            setControllerType(options.randomControlType, STATE_CONTROLLER_SET_RANDOM_NUMBER, STATE_NONE);
            }
        break;
        case STATE_CONTROLLER_RANDOM_SET_MODE:
            {
            stateControllerRandomSetMode();
            }
        break;
        case STATE_CONTROLLER_RANDOM_SET_RANGE:
            {
			stateNumerical(1, 127, options.randomRange, backupOptions.randomRange, true, false, GLYPH_NONE, STATE_CONTROLLER_RANDOM);
            }
        break;
        case STATE_CONTROLLER_RANDOM_SET_INITIAL_VALUE:
            {
			stateNumerical(0, 127, options.randomInitialValue, backupOptions.randomInitialValue, true, false, GLYPH_NONE, STATE_CONTROLLER_RANDOM);
            }
        break;
        case STATE_CONTROLLER_RANDOM_SET_LENGTH:
            {
			stateNumerical(0, 255, options.randomLength, backupOptions.randomLength, true, false, GLYPH_HIGH, STATE_CONTROLLER_RANDOM);
            }
        break;
        case STATE_CONTROLLER_RANDOM_SET_CLOCK:
            {
            options.controlRandomClocked = !options.controlRandomClocked;
            saveOptions();
            goUpState(STATE_CONTROLLER_RANDOM);
            }
        break;
        case STATE_CONTROLLER_SET_RANDOM_NUMBER:
            {
            setControllerNumber(options.randomControlType, options.randomControlNumber, backupOptions.randomControlType, backupOptions.randomControlNumber, STATE_CONTROLLER_RANDOM);
            }
        break;
        case STATE_CONTROLLER_SET_A2_NUMBER:
            {
            setControllerNumber(options.a2ControlType, options.a2ControlNumber, backupOptions.a2ControlType, backupOptions.a2ControlNumber, STATE_CONTROLLER);
            }
        break;
        case STATE_CONTROLLER_SET_A3_NUMBER:
            {
            setControllerNumber(options.a3ControlType, options.a3ControlNumber, backupOptions.a3ControlType, backupOptions.a3ControlNumber, STATE_CONTROLLER);
            }
        break;
#endif
        case STATE_CONTROLLER_SET_LEFT_KNOB_NUMBER:
            {
            setControllerNumber(options.leftKnobControlType, options.leftKnobControlNumber, backupOptions.leftKnobControlType, backupOptions.leftKnobControlNumber, STATE_CONTROLLER);
            }
        break;
        case STATE_CONTROLLER_SET_RIGHT_KNOB_NUMBER:
            {
            setControllerNumber(options.rightKnobControlType, options.rightKnobControlNumber, backupOptions.rightKnobControlType, backupOptions.rightKnobControlNumber, STATE_CONTROLLER);
            }
        break;
        case STATE_CONTROLLER_SET_MIDDLE_BUTTON_NUMBER:
            {
            setControllerNumber(options.middleButtonControlType, options.middleButtonControlNumber, backupOptions.middleButtonControlType, backupOptions.middleButtonControlNumber, STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_ON);
            }
        break;
        case STATE_CONTROLLER_SET_SELECT_BUTTON_NUMBER:
            {
            setControllerNumber(options.selectButtonControlType, options.selectButtonControlNumber, backupOptions.selectButtonControlType, backupOptions.selectButtonControlNumber, STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON);
            }
        break;
        case STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_ON:
            {
            setControllerButtonOnOff(options.middleButtonControlOn, STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF);
            }
        break;
        case STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON:
            {
            setControllerButtonOnOff(options.selectButtonControlOn, STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF);
            }
        break;
        case STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF:
            {
            setControllerButtonOnOff(options.middleButtonControlOff, STATE_CONTROLLER);
            }
        break;
        case STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF:
            {
            setControllerButtonOnOff(options.selectButtonControlOff, STATE_CONTROLLER);
            }
        break;
#endif

        case STATE_OPTIONS_TEMPO:
            {
            if (entry)
                {
                backupOptions = options; 
                lastTempoTapTime = 0;
                }

            if (isUpdated(MIDDLE_BUTTON, PRESSED))
                {
                if (lastTempoTapTime != 0)
                    {
                    // BPM = 1/(min/beat).  min/beat = micros/beat *  sec / 1000000 micros * min / 60 sec
                    // So BPM = 60000000 / micros 
                    uint16_t newTempo = (uint16_t)(60000000L / (currentTime - lastTempoTapTime));

                    // fold into options.tempo as a smoothing effort. 
                    // Note that we increase newTempo by one
                    // so that if options.tempo = newTempo - 1, averaging the two won't
                    // just truncate back to options.tempo.  We don't do this if newTempo
                    // <= options.tempo because we'd truncate DOWN to newTempo in this case.
                    if (options.tempo < newTempo)
                        newTempo = newTempo + 1;
                    options.tempo = max(min(((options.tempo + newTempo) >> 1), 999), 1);  // saves a tiny bit of code space!

					setPulseRate(options.tempo);
                    entry = true;
                    }
                lastTempoTapTime = currentTime;
                }

            // at this point, MIDDLE_BUTTON shouldn't have any effect on doNumericalDisplay (incrementing it)
            // because it's been consumed.
            
            uint8_t result = doNumericalDisplay(1, MAXIMUM_BPM, options.tempo, 0, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    if (options.tempo != currentDisplay)
                        {
                        options.tempo = currentDisplay; 
                        setPulseRate(options.tempo);          // don't constantly drive this function
                        }
                    break;
                case MENU_SELECTED:
                    if (backupOptions.tempo != options.tempo)
                        saveOptions();
                    // FALL THRU
                case MENU_CANCELLED:
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpStateWithBackup(optionsReturnState);
                    else
                        goUpStateWithBackup(STATE_OPTIONS);
#else
                    goUpStateWithBackup(STATE_OPTIONS);
#endif
                    setPulseRate(options.tempo);
                    break;
                }
            playApplication();       
            }
        break;
        case STATE_OPTIONS_NOTE_SPEED:
            {
            if (entry)
                {
                // can't avoid a divide :-(
                potDivisor = 1024 / (NOTE_SPEED_DOUBLE_WHOLE - NOTE_SPEED_EIGHTH_TRIPLET + 1);
                backupOptions = options; 
                }
            entry = false;
            if (updateDisplay)
                {
                clearScreen();
                writeNoteSpeed(led, options.noteSpeedType);
                }
            uint8_t i = isUpdated(SELECT_BUTTON, PRESSED);
            if (isUpdated(BACK_BUTTON, RELEASED) || i)
                {
                if (i)  // we don't want to call isUpdated(SELECT_BUTTON, ...) again as it resets things
                    {
                    if (backupOptions.noteSpeedType != options.noteSpeedType)
                        {
                        saveOptions();
                        }
                    }
                            
                // at any rate...
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpStateWithBackup(optionsReturnState);
                    else
                        goUpStateWithBackup(STATE_OPTIONS);
#else
                    goUpStateWithBackup(STATE_OPTIONS);
#endif
                setNotePulseRate(options.noteSpeedType);
                }
            else if (potUpdated[LEFT_POT])
                {
                // can't avoid a divide :-(
                uint8_t oldOptionsNoteSpeedType = options.noteSpeedType;
                options.noteSpeedType = (uint8_t) (pot[LEFT_POT] / potDivisor); //((potUpdated[LEFT_POT] ? pot[LEFT_POT] : pot[RIGHT_POT]) / potDivisor);
                if (oldOptionsNoteSpeedType != options.noteSpeedType) 
                    setNotePulseRate(options.noteSpeedType);
                }
            playApplication();       
            }
        break;
        case STATE_OPTIONS_SWING:
            {
            if (entry) backupOptions = options; 
            uint8_t result = doNumericalDisplay(0, 99, options.swing, 0, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    if (options.swing != currentDisplay)
                        {
                        options.swing = currentDisplay; 
                        }
                    break;
                case MENU_SELECTED:
                    if (backupOptions.swing != options.swing)
                        saveOptions();
                    // FALL THRU
                case MENU_CANCELLED:
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpStateWithBackup(optionsReturnState);
                    else
                        goUpStateWithBackup(STATE_OPTIONS);
#else
                    goUpStateWithBackup(STATE_OPTIONS);
#endif
                    break;
                }
            playApplication();     
            }
        break;
        
#ifdef INCLUDE_OPTIONS_TRANSPOSE_AND_VOLUME
        case STATE_OPTIONS_TRANSPOSE:
            {
            if (entry)
                {
                backupOptions = options;
                }
                                 
            uint8_t result = doNumericalDisplay(-60, 60, options.transpose, false, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    {
                    if (options.transpose != currentDisplay)
                        {
                        options.transpose = currentDisplay; 
                        sendAllSoundsOff();  // we must have this because if we've changed things we may never get a note off
                        }
                    }
                break;
                case MENU_SELECTED:
                    {
                    if (backupOptions.transpose != options.transpose)
                        saveOptions();
                    }
                // FALL THRU
                case MENU_CANCELLED:
                    {
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpStateWithBackup(optionsReturnState);
                    else
                        goUpStateWithBackup(STATE_OPTIONS);
#else
                    goUpStateWithBackup(STATE_OPTIONS);
#endif
                    sendAllSoundsOff();  // we must have this because if we've changed things we may never get a note off
                    }
                break;
                }
            playApplication();       
            }
        break;
        case STATE_OPTIONS_VOLUME:
            {
            uint8_t result;
            if (entry)
                {
                backupOptions = options; 
                const uint8_t _glyphs[7] = {
                    (FONT_8x5) + GLYPH_8x5_ONE_EIGHTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_FOURTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_HALF,
                    (FONT_3x5) + GLYPH_3x5_1,
                    (FONT_3x5) + GLYPH_3x5_2,
                    (FONT_3x5) + GLYPH_3x5_4,
                    (FONT_3x5) + GLYPH_3x5_8
                    };
                result = doGlyphDisplay(_glyphs, 7, NO_GLYPH, options.volume );
                entry = false;
                }
            else result = doGlyphDisplay(NULL, 7, NO_GLYPH, options.volume);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    options.volume = currentDisplay;
                    break;
                case MENU_SELECTED:
                    if (options.volume != backupOptions.volume)
                        saveOptions();
                    // FALL THRU
                case MENU_CANCELLED:
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpStateWithBackup(optionsReturnState);
                    else
                        goUpStateWithBackup(STATE_OPTIONS);
#else
                    goUpStateWithBackup(STATE_OPTIONS);
#endif
                    break;
                }
            playApplication();       
            }     
        break;
#endif

        case STATE_OPTIONS_PLAY_LENGTH:
            {
#ifdef INCLUDE_IMMEDIATE_RETURN
            stateNumerical(0, 100, options.noteLength, backupOptions.noteLength, true, false, GLYPH_NONE, 
                immediateReturn ? optionsReturnState : STATE_OPTIONS);
#else
            stateNumerical(0, 100, options.noteLength, backupOptions.noteLength, true, false, GLYPH_NONE, STATE_OPTIONS);
#endif
            playApplication();
            }
        break;
        case STATE_OPTIONS_MIDI_CHANNEL_IN:
            {
            stateNumerical(CHANNEL_OFF, CHANNEL_OMNI, options.channelIn, backupOptions.channelIn, true, true, GLYPH_OMNI, STATE_OPTIONS);
            }
        break;
        case STATE_OPTIONS_MIDI_CHANNEL_OUT:
            {
            uint8_t val = stateNumerical(CHANNEL_OFF, HIGHEST_MIDI_CHANNEL, options.channelOut, backupOptions.channelOut, true, true, GLYPH_NONE, STATE_OPTIONS);
            if (val != NO_STATE_NUMERICAL_CHANGE)
                sendAllSoundsOff();
            }
        break;
        case STATE_OPTIONS_MIDI_CHANNEL_CONTROL:
            {
#ifdef HEADLESS
            // Do not permit CHANNEL_OFF
            stateNumerical(LOWEST_MIDI_CHANNEL, HIGHEST_MIDI_CHANNEL, options.channelControl, backupOptions.channelControl, true, false, GLYPH_NONE, STATE_OPTIONS);
#else
            stateNumerical(CHANNEL_OFF, HIGHEST_MIDI_CHANNEL, options.channelControl, backupOptions.channelControl, true, true, GLYPH_NONE, STATE_OPTIONS);
#endif // HEADLESS
            }
        break;
        case STATE_OPTIONS_MIDI_CLOCK:
            {
            uint8_t result;
            if (entry) 
                {
                backupOptions = options; 
#ifdef INCLUDE_EXTENDED_MENU_DEFAULTS
                defaultMenuValue = options.clock;  // so we display the right thing
#endif
                }
            const char* menuItems[5] = { PSTR("USE"), PSTR("CONSUME"), PSTR("IGNORE"), PSTR("GENERATE"), PSTR("BLOCK") };
            result = doMenuDisplay(menuItems, 5, STATE_NONE, STATE_NONE, 1);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    {
                    // this hopefully clears up notes that sometimes get stuck when we change the clock mode
                    if (options.clock != currentDisplay)
                        sendAllSoundsOff();
                    options.clock = currentDisplay;
                    }
                break;
                case MENU_SELECTED:
                    {
                    if (!USING_EXTERNAL_CLOCK())
                        {
                        // do a restart
                        stopClock(true);
                        startClock(true);
                        }
                    saveOptions();
                    }
                // Else FALL THRU
                case MENU_CANCELLED:
                    {
                    goUpStateWithBackup(STATE_OPTIONS);
                    }
                break;
                }
            playApplication();     
            }
        break;
#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
        case STATE_OPTIONS_MIDI_CLOCK_DIVIDE:
            {
            stateNumerical(1, 16, options.clockDivisor, backupOptions.clockDivisor, true, true, GLYPH_NONE, STATE_OPTIONS);
            playApplication();
            }
        break;
#endif
        case STATE_OPTIONS_CLICK:
            {
            // The logic here is somewhat tricky. On entering, if we are presently clicking,
            // then I want to NOT click and be done with it.  Otherwise on entering or NOT,
            // I want to enter a note.  But I don't want to enter that note if I just turned
            // OFF clicking, hence the "done" thingamabob.
                        
            uint8_t done = false;
            if (entry)
                {
                if (options.click != NO_NOTE)
                    {
                    options.click = NO_NOTE;
                    saveOptions();
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpState(optionsReturnState);
                    else
                    	goUpState(STATE_OPTIONS);
#else
                    goUpState(STATE_OPTIONS);
#endif
                    done = true;
                    }
                }
                                
            if (!done)
                {
                uint8_t note = stateEnterNote(STATE_OPTIONS);
                if (note != NO_NOTE)  // it's a real note
                    {
                    options.click = note;
                    options.clickVelocity = stateEnterNoteVelocity;
                    saveOptions();
#ifdef INCLUDE_IMMEDIATE_RETURN
                    if (immediateReturn)
                        goUpState(optionsReturnState);
                    else
                    	goUpState(STATE_OPTIONS);
#else
                    goUpState(STATE_OPTIONS);
#endif
                    }
                }
            // At present if I call playApplication() here it adds over 130 bytes!  Stupid compiler.  So I can't do it right now
            // playApplication();
            }
        break;
        case STATE_OPTIONS_SCREEN_BRIGHTNESS:
            {
            if (entry) backupOptions = options; 
            uint8_t result = doNumericalDisplay(1, 16, options.screenBrightness + 1, 0, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    {
                    options.screenBrightness = currentDisplay - 1;
                    // if it's set to blocking, setScreenBrightness is too slow and it causes
                    // us to quickly get out of sync with our clock.  So we make setScreenBrightness
                    // non-blocking and so we also do it only once every 32 times, off-sync 
                    // with the screen.
                    scheduleScreenBrightnessUpdate = 1;
                    }
                break;
                case MENU_SELECTED:
                    {
                    if (backupOptions.screenBrightness != options.screenBrightness)
                        saveOptions();
                    }
                // FALL THRU
                case MENU_CANCELLED:
                    {
                    goUpStateWithBackup(STATE_OPTIONS);
                    setScreenBrightness(options.screenBrightness);  // reset
                    }
                break;
                }
            playApplication();     
            }
        break;
#ifdef INCLUDE_OPTIONS_MENU_DELAY
        case STATE_OPTIONS_MENU_DELAY:
            {
            uint8_t result;
            if (entry)
                {
                backupOptions = options; 
                const uint8_t _glyphs[11] = { 
                    (FONT_3x5) + GLYPH_3x5_0,
                    (FONT_8x5) + GLYPH_8x5_ONE_FOURTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_THIRD,
                    (FONT_8x5) + GLYPH_8x5_ONE_HALF,
                    (FONT_3x5) + GLYPH_3x5_1,
                    (FONT_3x5) + GLYPH_3x5_2,
                    (FONT_3x5) + GLYPH_3x5_3,
                    (FONT_3x5) + GLYPH_3x5_4,
                    (FONT_3x5) + GLYPH_3x5_8,
                    (FONT_8x5) + GLYPH_8x5_INFINITY,
                    (FONT_3x5) + GLYPH_3x5_S
                    };
                result = doGlyphDisplay(_glyphs, 11, NO_GLYPH, options.menuDelay );
                }
            else result = doGlyphDisplay(NULL, 11, NO_GLYPH, options.menuDelay);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    {
                    options.menuDelay = currentDisplay;
                    setMenuDelay(options.menuDelay);
                    }
                break;
                case MENU_SELECTED:
                    {
                    if (options.menuDelay != backupOptions.menuDelay)
                        saveOptions();
                    }
                // FALL THRU
                case MENU_CANCELLED:
                    {
                    goUpStateWithBackup(STATE_OPTIONS);
                    setMenuDelay(options.menuDelay);
                    }
                break;
                }
            playApplication();     
            }
        break;
#endif

        case STATE_OPTIONS_ABOUT:
            {
            goUpState(STATE_OPTIONS);
            playApplication();
            }
        break;


#ifdef INCLUDE_SPLIT
        case STATE_SPLIT_CHANNEL:
            {
            stateNumerical(LOWEST_MIDI_CHANNEL, HIGHEST_MIDI_CHANNEL, options.splitChannel, backupOptions.splitChannel, true, false, GLYPH_NONE, STATE_SPLIT);
            }
        break;

        case STATE_SPLIT_NOTE:
            {
            stateSplitNote();
            }
        break;
                  
        case STATE_SPLIT_LAYER_NOTE:
            {
            stateSplitLayerNote();
            }
        break;
#endif


#ifdef INCLUDE_THRU
        case STATE_THRU_PLAY:
            {
            stateThruPlay();
            }
        break;
                
        case STATE_THRU_EXTRA_NOTES:
            {
            stateNumerical(0, 31, options.thruExtraNotes, backupOptions.thruExtraNotes, true, true, GLYPH_NONE, STATE_THRU);
            }
        break;
                  
        case STATE_THRU_DISTRIBUTE_NOTES:
            {
            stateNumerical(0, 15, options.thruNumDistributionChannels, backupOptions.thruNumDistributionChannels, true, true, GLYPH_NONE, STATE_THRU);
            }
        break;
        
        case STATE_THRU_CHORD_MEMORY:
            {
            if (entry && options.thruChordMemorySize > 0)  // maybe entry is not necessary
                {
                options.thruChordMemorySize = 0;
                saveOptions();
                goUpState(STATE_THRU);
                }
            else        
                {
                uint8_t retval = stateEnterChord(local.thru.chordMemory, MAX_CHORD_MEMORY_NOTES, STATE_THRU);
                if (retval != NO_NOTE)
                    {
                    // now store.
                    options.thruChordMemorySize = retval;
                    memcpy(options.thruChordMemory, local.thru.chordMemory, retval);
                    saveOptions();

                    goUpState(STATE_THRU);
                    }
                }
            }
        break;
        
        case STATE_THRU_DEBOUNCE:
            {
            stateNumerical(0, 255, options.thruDebounceMilliseconds, backupOptions.thruDebounceMilliseconds, true, true, GLYPH_NONE, STATE_THRU);
            }
        break;
        
        case STATE_THRU_MERGE_CHANNEL_IN:
            {
            stateNumerical(CHANNEL_OFF, HIGHEST_MIDI_CHANNEL, options.thruMergeChannelIn, backupOptions.thruMergeChannelIn, true, true, GLYPH_NONE, STATE_THRU);
            }
        break;

/*
  case STATE_THRU_CC_NRPN:
  {
  options.thruCCToNRPN = !options.thruCCToNRPN;
  saveOptions();
  goUpState(STATE_THRU);
  }
  break;
*/
        case STATE_THRU_BLOCK_OTHER_CHANNELS:
            {
            options.thruBlockOtherChannels = !options.thruBlockOtherChannels;
            saveOptions();
            goUpState(STATE_THRU);
            }
        break;
#endif

#ifdef INCLUDE_MEASURE
        case STATE_MEASURE_MENU:
            {
            stateMeasureMenu();
            }
        break;
    
        case STATE_MEASURE_BEATS_PER_BAR:
            {
            stateNumerical(1, 16, options.measureBeatsPerBar, backupOptions.measureBeatsPerBar, true, false, GLYPH_NONE, STATE_MEASURE);
            playApplication();
            }
        break;
    
        case STATE_MEASURE_BARS_PER_PHRASE:
            {
            stateNumerical(1, 32, options.measureBarsPerPhrase, backupOptions.measureBarsPerPhrase, true, false, GLYPH_NONE, STATE_MEASURE);
            playApplication();
            }
        break;
        
        

//// SYNTHS

#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD
        case STATE_SYNTH_WALDORF_BLOFELD:
            {
            stateSynthWaldorfBlofeld();
            }
        break;
#endif INCLUDE_SYNTH_WALDORF_BLOFELD
#ifdef INCLUDE_SYNTH_KAWAI_K4
        case STATE_SYNTH_KAWAI_K4:
            {
            stateSynthKawaiK4();
            }
        break;
#endif INCLUDE_SYNTH_KAWAI_K4
#ifdef INCLUDE_SYNTH_KAWAI_K5
        case STATE_SYNTH_KAWAI_K5:
            {
            stateSynthKawaiK5();
            }
        break;
#endif INCLUDE_SYNTH_KAWAI_K5
#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
        case STATE_SYNTH_OBERHEIM_MATRIX_1000:
            {
            debug(100);
            stateSynthOberheimMatrix1000();
            }
        break;
#endif INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
#ifdef INCLUDE_SYNTH_KORG_MICROSAMPLER
        case STATE_SYNTH_KORG_MICROSAMPLER:
            {
            stateSynthKorgMicrosampler();
            }
        break;
#endif INCLUDE_SYNTH_KORG_MICROSAMPLER
#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z
        case STATE_SYNTH_YAMAHA_TX81Z:
            {
            stateSynthYamahaTX81Z();
            }
        break;
#endif INCLUDE_SYNTH_YAMAHA_TX81Z
        
        
        
#endif
        // END SWITCH       
        }
        
    // consume the pulses
    pulse = 0;
    if (notePulse == 1)
        {
        notePulse = 0;
        drawNotePulseToggle = !drawNotePulseToggle;
        }
            
    if (beat == 1)
        {
        beat = 0;
        drawBeatToggle = !drawBeatToggle;
        }
            
    // display if appropriate
    if (updateDisplay)
        writeFooterAndSend();
        
    // get rid of any new item
    newItem = 0;
    itemChannel = CHANNEL_OFF;  // we do this because some incoming MIDI handlers don't set the itemChannel at all.
    
    // clear the pots
    potUpdated[LEFT_POT] = potUpdated[RIGHT_POT] = NO_CHANGE;
    }







