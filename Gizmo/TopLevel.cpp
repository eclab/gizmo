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

GLOBAL uint8_t bypass = 0;                                        // Do we bypass the system entirely and just let MIDI flow through the box?


void toggleBypass(uint8_t channel)
    {
    sendAllSoundsOffDisregardBypass(channel);
    if (!bypass) 
        {
        // clear the LEDs
#ifndef HEADLESS
        *port_LED_GREEN |= LED_GREEN_mask;
        *port_LED_RED |= LED_RED_mask;
#endif
        }
    bypass = !bypass;
    if (bypass) { MIDI.turnThruOn(); }
    else MIDI.turnThruOff();
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

#define BACK_BUTTON 0
#define MIDDLE_BUTTON 1
#define SELECT_BUTTON 2

GLOBAL uint8_t button[3];
GLOBAL uint8_t buttonUpdated[3] = { NO_CHANGE, NO_CHANGE, NO_CHANGE };
GLOBAL static uint8_t ignoreNextButtonRelease[3] = { false, false, false };


#define LEFT_POT 0
#define RIGHT_POT 1

GLOBAL uint16_t pot[2];        // The current left pot value OR MIDI controlled value
GLOBAL uint8_t potUpdated[2];       // has the left pot been updated?  CHANGED or NO_CHANGE
GLOBAL static uint16_t potCurrent[2][3];     // The filtered current left pot value
GLOBAL static uint16_t potCurrentFinal[2];    //  
GLOBAL static uint16_t potLast[2];     // The last pot value submitted 

// Divide this into the pot[LEFT_POT] to determne what the menu state should be.
// If negative, then MULTIPLY pot[LEFT_POT] by -potDivisor to determine what the state should be.
GLOBAL static int16_t potDivisor;

// SETUP POTS
// initializes the pots
void setupPots()
    {
    memset(potCurrent, 0, 2 * 3);
    memset(potUpdated, NO_CHANGE, 2);
    memset(pot, 0, 2);
    memset(potCurrentFinal, 0, 2);
    memset(potLast, 0, 2);
    }

//// Clears the 'released' and 'released long' flag on all buttons.
void clearReleased()
    {
    for(uint8_t i = BACK_BUTTON; i <= SELECT_BUTTON; i++)
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
        else                            // button used to be pressed
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
                        
            updateButtons(buttonPressed);
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
            return 0;  // don't update the display
            }
        break;
        case 2:
            {
#ifndef HEADLESS
            if (!lockoutPots)
                potUpdated[RIGHT_POT] = updatePot(pot[RIGHT_POT], potCurrent[RIGHT_POT], potCurrentFinal[RIGHT_POT], potLast[RIGHT_POT], A1);
#endif // HEADLESS
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




GLOBAL static const char* menu[MAX_MENU_ITEMS];                        // This is an array of pointers into PROGMEM
GLOBAL int16_t currentDisplay;                     // currently displayed menu item

// NOTE: This creates a temporary char buffer of length MAX_MENU_ITEM_LENGTH.
// It's better than storing a buffer 8xMAX_MENU_ITEM_LENGTH though.
// So you need to subtract about 28 (for the moment) from the local variable storage left

#define FORCE_NEW_DISPLAY 255

uint8_t doMenuDisplay(const char** _menu, uint8_t menuLen, uint8_t baseState, uint8_t backState, uint8_t scrollMenu, uint8_t extra = 0)
    {
    // What we'd like to display next.  By default it's set to currentDisplay to indicate that we're not changing
    uint8_t newDisplay;
    
    if (entry)
        {
        // copy over the PSTRs but don't convert them.
        memcpy(menu, _menu, menuLen * sizeof(const char*));
    
        // can't avoid a divide :-(
        potDivisor = 1024 / menuLen;
        
        // what do we display first?
        
        if (baseState == STATE_NONE)                                            // These aren't states, so just use the first item
            currentDisplay = 0;
        else if (defaultState == STATE_NONE)                                            // We don't have a default state specified.  So use the first item
            currentDisplay = 0;
        else                                                                    // We have a default state.  So use it
            currentDisplay = defaultState - baseState;
                
        newDisplay = FORCE_NEW_DISPLAY;                                         // This tells us that we MUST compute a new display
        entry = false;
        defaultState = STATE_NONE;                                              // We're done with this
        }
    else
        {
        newDisplay = currentDisplay;            // we're not changing by default
        
        if (potUpdated[LEFT_POT])
            {
            // can't avoid a divide :-(
            newDisplay = (uint8_t) (pot[LEFT_POT] / potDivisor); //(uint8_t)((potUpdated[LEFT_POT] ? pot[LEFT_POT] : pot[RIGHT_POT]) / potDivisor);
            if (newDisplay >= menuLen)        // this can happen because potDivisor is discrete
                newDisplay = menuLen - 1; 
            }
#ifdef INCLUDE_MIDDLE_BUTTON_INCREMENTS_MENU
        else if (isUpdated(MIDDLE_BUTTON, RELEASED))
            {
            newDisplay++;
            if (newDisplay >= menuLen)
                newDisplay = 0; 
            }
#endif
        }

    if (newDisplay != currentDisplay)                                           // we're starting fresh (FORCE_NEW_DISPLAY) or have something new
        {
        char menuItem[MAX_MENU_ITEM_LENGTH];

        if (newDisplay != FORCE_NEW_DISPLAY)
            currentDisplay = newDisplay;
                
        clearBuffer();
        strcpy_P(menuItem, menu[currentDisplay]);  
        addToBuffer(menuItem, extra);
                
        if (state == STATE_ROOT)
            application = FIRST_APPLICATION + currentDisplay;
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        if (baseState != STATE_NONE)
            {
            if (state == STATE_ROOT)
                return NO_MENU_SELECTED;
            goUpState(backState);
            }
        return MENU_CANCELLED;
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        if (baseState != STATE_NONE)
            {
            state = baseState + currentDisplay;
            entry = true;
            }
        return MENU_SELECTED;
        }

    if (updateDisplay)
        {
        clearScreen();
        if (scrollMenu)
            scrollBuffer(led, led2);
        else 
            writeBuffer(led, led2);
        }    
    return NO_MENU_SELECTED;
    }
    
    

          
          






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


int16_t boundValue(int16_t val, int16_t minValue, int16_t maxValue)
    {
    if (val < minValue) val = minValue;
    if (val > maxValue) val = maxValue;
    return val;
    }

GLOBAL static int8_t potFineTune;
GLOBAL static uint8_t secondGlyph = NO_GLYPH;

uint8_t doNumericalDisplay(int16_t minValue, int16_t maxValue, int16_t defaultValue, 
    uint8_t includeOff, uint8_t includeOther)
    {
    if (maxValue > 19999)
        maxValue = 19999;
        
    if (entry)
        {
        defaultValue = boundValue(defaultValue, minValue, maxValue);

        currentDisplay = defaultValue;
        if (maxValue - minValue + 1 > 1024)  // whooo boy
            {
            potDivisor = -((maxValue - minValue + 1) >> 10);  // division by 1024
            }
        else
            {
            // can't avoid a divide :-(
            potDivisor = 1024 / (maxValue - minValue + 1);
            }
        potFineTune = 0;
        entry = false;
        }
    
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        return MENU_CANCELLED;
        }
    else if (isUpdated(SELECT_BUTTON, PRESSED))
        {
        return MENU_SELECTED;
        }
    
    if (potUpdated[LEFT_POT])
        {
        potFineTune = 0;
        // can't avoid a divide this time!
        if (potDivisor > 0)  // small numbers
            {
            currentDisplay = boundValue((pot[LEFT_POT] / potDivisor) + minValue, minValue, maxValue);
            }
        else            // big numbers
            {
            currentDisplay = boundValue((pot[LEFT_POT] * (-potDivisor)) + minValue, minValue, maxValue);
            }
        }
#ifdef INCLUDE_MIDDLE_BUTTON_INCREMENTS_MENU
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        currentDisplay++;
        if (currentDisplay > maxValue)
            currentDisplay = minValue; 
        }
#endif
    
    // if we're doing a high-resolution number (> 128 basically) then
    // we check the right pot and use it as a fine-tuning
    if (potUpdated[RIGHT_POT] && (maxValue - minValue + 1) > 128)
        {
        // this is gonna have funky effects at the boundaries
        
        currentDisplay -= potFineTune;  // old one
        potFineTune = (pot[RIGHT_POT] >> 3) - 64;  // right pot always maps to a delta of -64 ... 63.
        currentDisplay += potFineTune;

        currentDisplay = boundValue(currentDisplay, minValue, maxValue);
        }        

    if (updateDisplay)
        {
        clearScreen();

        if (includeOff && (currentDisplay == minValue))
            {
            // write "--"
            write3x5Glyphs(GLYPH_OFF);
            }
        else if ((includeOther != GLYPH_NONE) && (includeOther != GLYPH_OTHER) && (currentDisplay == maxValue))
            {
            write3x5Glyphs(includeOther);
            }
        else
            {
            writeNumber(led, led2, currentDisplay);
            if (includeOther == GLYPH_OTHER)
                {
                if (glyphs[currentDisplay] != SLOT_TYPE_EMPTY &&
                    glyphs[currentDisplay] != slotTypeForApplication(application))  // need to indicate that we're overwriting
                    {
                    blink3x5Glyph(led2, glyphs[currentDisplay], 4);
                    }
                else 
                    {
                    write3x5Glyph(led2, glyphs[currentDisplay], 4);
                    }
                }
            if (secondGlyph != NO_GLYPH)
                write3x5Glyph(led2, secondGlyph, 0);
            secondGlyph = NO_GLYPH;
            }
        }
        
    return NO_MENU_SELECTED;
    }




#define NO_STATE_NUMERICAL_CHANGE 255

// STATE NUMERICAL
// This function performs the basic functions for a state whose entire purpose is to compute a numerical value
// and set *value* to that value.  If updateOptions is true, then stateNumerical will recover from a cancel by
// overwriting the options with the backup options. Otherwise it will recover by overwriting value with backupValue.  
uint8_t stateNumerical(uint8_t start, uint8_t end, uint8_t &value, uint8_t &backupValue,
    uint8_t updateOptions, uint8_t includeOff, uint8_t other, uint8_t backState)
    {
    uint8_t oldValue = value;
    if (entry)
        { 
        if (updateOptions) backupOptions = options; 
        else backupValue = value;
        }
        
    uint8_t result = doNumericalDisplay(start, end, value, includeOff, other);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            if (value != currentDisplay)
                {
                value = currentDisplay; 
                }
            }
        break;
        case MENU_SELECTED:
            {
            if (updateOptions)
                saveOptions();
            else
                backupValue = value;
            }
        // FALL THRU
        case MENU_CANCELLED:
            {
            goUpState(backState);
            if (updateOptions) 
                options = backupOptions;
            else 
                value = backupValue;
            }
        break;
        }
    if (value != oldValue) 
        return oldValue;
    else return NO_STATE_NUMERICAL_CHANGE;
    }




///// DOGLYPHDISPLAY()
//
// This function updates a display to one of several glyphs, while 
// optionally updating the alternate display to a specific glyph.
// The glyphs are given as an array of GLYPH NUMBERS.  A glyph number
// is the glyph constant (such as GLYPH_4x5_NEGATIVE_6) 
// plus the font constant (such as FONT_4x5).
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




GLOBAL uint8_t glyphs[MAX_GLYPHS];

// used internally for doGlyphDisplay()
void drawGlyphForGlyphDisplay(uint8_t* mat, const uint8_t glyph)
    {
    switch(glyph >> 6)
        {
        case 0:         // FONT_3x5
            {
            write3x5Glyph(mat, glyph & 63, 5);
            }
        break;
        case 1:         // FONT_4x5
            {
            write4x5Glyph(mat, glyph & 63, 4);
            }
        break;
        case 2:         // FONT_8x5
            {
            write8x5Glyph(mat, glyph & 63);
            }
        break;
        case 3:
            {
            // DO NOTHING
            }
        break;
        }
    }


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


uint8_t doGlyphDisplay(const uint8_t* _glyphs, uint8_t numGlyphs, const uint8_t otherGlyph, int16_t defaultValue)
    {
    if (_glyphs != NULL)
        {
        currentDisplay = defaultValue;
        // can't avoid a divide this time!
        potDivisor = 1024 / numGlyphs;
        memcpy(glyphs, _glyphs, numGlyphs);
        entry = false;
        }
    
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        return MENU_CANCELLED;
        }
    else if (isUpdated(SELECT_BUTTON, PRESSED))
        {
        return MENU_SELECTED;
        }
    else if (potUpdated[LEFT_POT])
        {
        // can't avoid a divide this time!
        currentDisplay = (uint8_t) (pot[LEFT_POT] / potDivisor); // ((potUpdated[LEFT_POT] ? pot[LEFT_POT] : pot[RIGHT_POT]) / potDivisor);
        if (currentDisplay >= numGlyphs)
            currentDisplay = numGlyphs - 1;
        }
#ifdef INCLUDE_MIDDLE_BUTTON_INCREMENTS_MENU
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        currentDisplay++;
        if (currentDisplay  >= numGlyphs)
            currentDisplay = 0; 
        }
#endif
                
    if (updateDisplay)
        {
        clearScreen();
        drawGlyphForGlyphDisplay(led, glyphs[currentDisplay]);
        if (otherGlyph != NO_GLYPH)
            drawGlyphForGlyphDisplay(led2, otherGlyph);
        }
        
    return NO_MENU_SELECTED;
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
//// Omitted are: MIDI CLock, Active Sensing, both AfterTouch forms, Time Code Quarter Frame



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
            const char* menuItems[10] = { PSTR("ARPEGGIATOR"), PSTR("STEP SEQUENCER"), PSTR("RECORDER"), PSTR("GAUGE"), PSTR("CONTROLLER"), PSTR("SPLIT"), PSTR("THRU"), PSTR("SYNTH"), PSTR("MEASURE"), options_p };
            doMenuDisplay(menuItems, 10, FIRST_APPLICATION, STATE_ROOT, 1);
#endif
#if defined(__UNO__)
            const char* menuItems[6] = { PSTR("ARPEGGIATOR"), PSTR("STEP SEQUENCER"), PSTR("RECORDER"), PSTR("GAUGE"), PSTR("CONTROLLER"), options_p };
            doMenuDisplay(menuItems, 6, FIRST_APPLICATION, STATE_ROOT, 1);
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
            const char* menuItems[7] = { PSTR("GO"), PSTR("L KNOB"), PSTR("R KNOB"), PSTR("M BUTTON"), PSTR("R BUTTON"), PSTR("WAVE"), PSTR("RANDOM") };
            doMenuDisplay(menuItems, 7, STATE_CONTROLLER_PLAY, STATE_ROOT, 1);
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
            const char* menuItems[5] = { 
#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD
                PSTR("WALDORF BLOFELD"),
#endif INCLUDE_SYNTH_WALDORF_BLOFELD
#ifdef INCLUDE_SYNTH_KAWAI_K4
                PSTR("KAWAI K4"),
#endif INCLUDE_SYNTH_KAWAI_K4
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
            const char* menuItems[16] = { PSTR("TEMPO"), PSTR("NOTE SPEED"), PSTR("SWING"), PSTR("TRANSPOSE"), 
                                          PSTR("VOLUME"), PSTR("LENGTH"), PSTR("IN MIDI"), PSTR("OUT MIDI"), PSTR("CONTROL MIDI"), PSTR("CLOCK"), PSTR("DIVIDE"),
                                          ((options.click == NO_NOTE) ? PSTR("CLICK") : PSTR("NO CLICK")),
                                          PSTR("BRIGHTNESS"), 
                                          PSTR("MENU DELAY"), 
                                              (options.voltage == NO_VOLTAGE ? PSTR("CV+VELOCITY") : 
                                              (options.voltage == VOLTAGE_WITH_VELOCITY ? PSTR("CV+AFTERTOUCH") : PSTR("NO CV"))),
                                          PSTR("GIZMO V3 (C) 2017 SEAN LUKE") };
            doMenuDisplay(menuItems, 16, STATE_OPTIONS_TEMPO, optionsReturnState, 1);
#endif
#if defined(__UNO__)
            const char* menuItems[11] = { PSTR("TEMPO"), PSTR("NOTE SPEED"), PSTR("SWING"), 
                                          PSTR("LENGTH"), PSTR("IN MIDI"), PSTR("OUT MIDI"), PSTR("CONTROL MIDI"), PSTR("CLOCK"), 
                                          ((options.click == NO_NOTE) ? PSTR("CLICK") : PSTR("NO CLICK")),
                                          PSTR("BRIGHTNESS"),
                                          PSTR("GIZMO V3 (C) 2017 SEAN LUKE") };
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
            stateArpeggiatorCreate();
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
                
            // 17 represents DEFAULT channel
            uint8_t val = stateNumerical(0, 17, local.stepSequencer.outMIDI[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, true, GLYPH_DEFAULT, STATE_STEP_SEQUENCER_MENU);
            if (val != NO_STATE_NUMERICAL_CHANGE)
                sendAllSoundsOff();
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_VELOCITY:
            {
            // 128 represents FREE velocity
            stateNumerical(0, 128, local.stepSequencer.velocity[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_FREE, STATE_STEP_SEQUENCER_MENU);
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_FADER:
            {
            stateNumerical(0, 127, local.stepSequencer.fader[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_NONE, STATE_STEP_SEQUENCER_MENU);
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_LENGTH:
            {
            // 101 represents DEFAULT length
            stateNumerical(0, 101, local.stepSequencer.noteLength[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, GLYPH_DEFAULT, STATE_STEP_SEQUENCER_MENU);
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
                entry = false;
                }

            if (isUpdated(BACK_BUTTON, RELEASED))
                {
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
                        if (local.control.displayType == CONTROL_TYPE_VOLTAGE_A ||
                            local.control.displayType == CONTROL_TYPE_VOLTAGE_B)
                            {
                            writeNumber(led, led2, (uint16_t)(local.control.displayValue));
                            }
                        else if (local.control.displayType == CONTROL_TYPE_PITCH_BEND)
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
                    options.tempo = ((options.tempo + newTempo) >> 1);

                    if (options.tempo < 1) options.tempo = 1;
                    if (options.tempo > 999) options.tempo = 999;

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
                goUpStateWithBackup(STATE_OPTIONS);
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
                    goUpStateWithBackup(STATE_OPTIONS);
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
                    goUpStateWithBackup(STATE_OPTIONS);
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
                    goUpStateWithBackup(STATE_OPTIONS);
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
                defaultState = options.clock;  // so we display the right thing
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
                    goUpState(STATE_OPTIONS);
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
                    goUpState(STATE_OPTIONS);
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
                    (FONT_8x5) + GLYPH_8x5_ONE_EIGHTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_FOURTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_THIRD,
                    (FONT_8x5) + GLYPH_8x5_ONE_HALF,
                    (FONT_3x5) + GLYPH_3x5_1,
                    (FONT_3x5) + GLYPH_3x5_2,
                    (FONT_3x5) + GLYPH_3x5_3,
                    (FONT_3x5) + GLYPH_3x5_4,
                    (FONT_3x5) + GLYPH_3x5_8,
                    (FONT_8x5) + GLYPH_8x5_INFINITY 
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

#ifdef INCLUDE_VOLTAGE
        case STATE_OPTIONS_VOLTAGE:
            {
            options.voltage++;
            if (options.voltage > VOLTAGE_WITH_AFTERTOUCH)
                options.voltage = NO_VOLTAGE;
            saveOptions();
            goUpState(STATE_OPTIONS);
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
#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
        case STATE_SYNTH_OBERHEIM_MATRIX_1000:
            {
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







