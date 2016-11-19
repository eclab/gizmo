#include "All.h"






//// SETTING UP MIDI

MIDI_CREATE_DEFAULT_INSTANCE();



///// SCROLL DELAY
///// 


////// We only redraw once every 4 ticks.
////// Thus a delay of X means 4 * X ticks
////// A tick is 1/3125 sec, so a delay of 78 means is about 1/10 sec
////// I find that faster than 1/20 sec the screen becomes hard to watch.
////// So I'm going with 40

GLOBAL static uint8_t menuDelays[11] = { NO_MENU_DELAY, EIGHTH_MENU_DELAY, QUARTER_MENU_DELAY, THIRD_MENU_DELAY, HALF_MENU_DELAY, DEFAULT_MENU_DELAY, DOUBLE_MENU_DELAY, TREBLE_MENU_DELAY, QUADRUPLE_MENU_DELAY, EIGHT_TIMES_MENU_DELAY, HIGH_MENU_DELAY };

// SET MENU DELAY
// Changes the menu delay to a desired value (between 0: no menu delay, and 11: infinite menu delay).  The default is 5
void setMenuDelay(uint8_t index)
    {
    if (index > 10) index = 5;
    setScrollDelays(menuDelays[index], DEFAULT_SHORT_DELAY);
    }



GLOBAL uint8_t state = STATE_ROOT;                     // The current state
GLOBAL uint8_t application = STATE_ARPEGGIATOR;           // The top state (the application, so to speak): we display a dot indicating this.
GLOBAL uint8_t entry = 1;  
GLOBAL uint8_t optionsReturnState;
GLOBAL uint8_t defaultState = STATE_NONE;


/// BYPASSING

/// 2. If 'bypass' is set to 1, then all signals are sent through no matter what, and
///    furthermore, the system cannot emit any signals.  The user can turn on bypass
///    either by choosing it in the options, or by long-pressing BACK+SELECT.

GLOBAL uint8_t bypass = 0;                                        // Do we bypass the system entirely and just let MIDI flow through the box?


void toggleBypass()
    {
    if (!bypass) 
        {
        sendAllNotesOff();
        // clear the LEDs
        *port_LED_GREEN |= LED_GREEN_mask;
        *port_LED_RED |= LED_RED_mask;
        }
    bypass = !bypass;
    if (bypass) MIDI.turnThruOn();
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




/// BUTTON COUNTDOWNS
/// The updateButtons command updates buttons (either real or virtual).  Each of these buttons
/// can have a countdown to determine if when it's released it's RELEASED or RELEASED LONG.
/// If BUTTON_PRESSED_COUNTDOWN_INVALID, then a button is never considered to be released long.

#define BUTTON_PRESSED_COUNTDOWN_DEBOUNCE 100
GLOBAL int16_t buttonPressedCountdown[3] = { BUTTON_PRESSED_COUNTDOWN_DEBOUNCE, BUTTON_PRESSED_COUNTDOWN_DEBOUNCE, BUTTON_PRESSED_COUNTDOWN_DEBOUNCE };

void updateButtons(uint8_t buttonPressed[])
    {
    uint8_t oldButton[3];
    memcpy(oldButton, button, 3);
    memcpy(button, buttonPressed, 3);

    for(uint8_t i = 0; i < 3; i++)
        {
        if (!oldButton[i] && buttonPressed[i] && buttonPressedCountdown[i] == 0)                 // button was pressed
            {
            buttonUpdated[i] = PRESSED;
            buttonPressedCountdown[i] = BUTTON_PRESSED_COUNTDOWN_MAX;
            }
        else if (oldButton[i] && !buttonPressed[i])     // button was released or released long
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
            }
        }
    }




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

uint8_t updatePot(uint16_t &pot, uint16_t* potCurrent, uint16_t &potLast, uint16_t& oldPotLast, uint8_t analog)
    {
    // potLast = 1/8 analogRead + 7/8 previous potLast
    // BEWARE: if you make this big (like 1/16, 15/16) it will always average
    // to potCurrent and you can't make jump because 16 is larger than 8, which
    // is the MINIMUM_POT_DEVIATION, creating weird effects.
    potCurrent[0] = potCurrent[1];
    potCurrent[1] = potCurrent[2];
    potCurrent[2] = analogRead(analog);
    uint16_t middle = MEDIAN_OF_THREE(potCurrent[0], potCurrent[1], potCurrent[2]);
    potLast = (potLast + middle) / 2;
    // test to see if we're really turning the knob
    if (oldPotLast > potLast && oldPotLast - potLast >= MINIMUM_POT_DEVIATION ||
        potLast > oldPotLast && potLast - oldPotLast >= MINIMUM_POT_DEVIATION)
        { oldPotLast = potLast; pot = potLast; return CHANGED; }
    else return NO_CHANGE;
    }



//// Called by go() to do costly items only every once in a while, namely
//// (1) checking buttons
//// (2) checking pots
//// (3) updating the screen

uint8_t update()
    {
    switch (tickCount & 0x3) // That is, binary 11
        {
        case 0:
            {
            uint8_t buttonPressed[3];
            
            // Note that when we're pressed, the value is ZERO, so we do !pressed
            buttonPressed[BACK_BUTTON] = !(*port_BACK_BUTTON & BACK_BUTTON_mask) ;
            buttonPressed[MIDDLE_BUTTON] = !(*port_MIDDLE_BUTTON & MIDDLE_BUTTON_mask) ;
            buttonPressed[SELECT_BUTTON] = !(*port_SELECT_BUTTON & SELECT_BUTTON_mask) ;
            
            updateButtons(buttonPressed);
            return 0;  // don't update the display
            }
        break;
        case 1:
            {
            if (!lockoutPots)
                potUpdated[LEFT_POT] = updatePot(pot[LEFT_POT], potCurrent[LEFT_POT], potCurrentFinal[LEFT_POT], potLast[LEFT_POT], A0);
            return 0;  // don't update the display
            }
        break;
        case 2:
            {
            if (!lockoutPots)
                potUpdated[RIGHT_POT] = updatePot(pot[RIGHT_POT], potCurrent[RIGHT_POT], potCurrentFinal[RIGHT_POT], potLast[RIGHT_POT], A1);
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
  



/// hehe, global local
GLOBAL _local local;
        



// FULLRESET()  
// This resets all the parameters to their default values in the EEPROM,
// then reboots the board.

void fullReset()
    {
    resetOptions();
    saveOptions();

    for(uint8_t i = 0; i < NUM_SLOTS; i++)
        {
        loadSlot(i);
        data.slot.type = SLOT_TYPE_EMPTY;
        saveSlot(i);
        }
  
    for(uint8_t i = 0; i < NUM_ARPS; i++)
        {
        //loadArpeggio(i);
        LOAD_ARPEGGIO(i);
        data.arp.length = 0;
        SAVE_ARPEGGIO(i);
        }

    soft_restart();
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
    uint8_t ts = application - STATE_ARPEGGIATOR;
    drawRange(led, 1, 0, MAX_APPLICATIONS, application - STATE_ARPEGGIATOR);
    if (bypass)
        {
        blinkPoint(led, 7, 0);
        }
    else
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



#define MAX_MENU_ITEMS 16
#define MAX_MENU_ITEM_LENGTH 30
#define NO_MENU_SELECTED 100
#define MENU_SELECTED 101
#define MENU_CANCELLED 102
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
        char menuItem[MAX_MENU_ITEM_LENGTH];

        // copy over the PSTRs but don't convert them.
        memcpy(menu, _menu, menuLen * sizeof(const char*));
    
        // can't avoid a divide :-(
        potDivisor = 1024 / menuLen;
        
        // what do we display first?
        
        if (baseState == STATE_NONE)                                            // These aren't states, so just use the first item
            currentDisplay = 0;
        else if (defaultState == STATE_NONE)                            // We don't have a default state specified.  So use the first item
            currentDisplay = 0;
        else                                                                                            // We have a default state.  So use it
            currentDisplay = defaultState - baseState;
                
        newDisplay = FORCE_NEW_DISPLAY;                                         // This tells us that we MUST compute a new display
        entry = false;
        defaultState = STATE_NONE;                                                      // We're done with this
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
            application = STATE_ARPEGGIATOR + currentDisplay;
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
    else if (isUpdated(SELECT_BUTTON, PRESSED))
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
// includeOther can be set to OTHER_NONE, OTHER_OMNI, OTHER_DEFAULT, or 
// OTHER_GLYPH, and if anything but OTHER_NONE, it will correspond to the maximum value.
// Furthermore if OTHER_GLYPH is used, then the glyphs array is scanned to determine 
// a glyph to display on the far left side of the screen for each possible numerical
// value.  At present this array is only MAX_GLYPHS in length, so if your max value is 
// larger than this, you're going to have a problem.  



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
    
    // if we're doing a high-resolution number (> 256 basically) then
    // we check the right pot and use it as a fine-tuning
    if (potUpdated[RIGHT_POT] && (maxValue - minValue + 1) >= 256)
        {
        // this is gonna have funky effects at the boundaries
        
        currentDisplay -= potFineTune;  // old one
        potFineTune = (pot[RIGHT_POT] >> 3) - 64;  // right pot always maps to a delta of -64 ... 64.  I want -128 ... 128 but it's just too small an angle for a step size of 1  
        currentDisplay += potFineTune;

        currentDisplay = boundValue(currentDisplay, minValue, maxValue);
        }        

    if (updateDisplay)
        {
        clearScreen();

        if (includeOff && (currentDisplay == minValue))
            {
            // write "--"
            write3x5Glyphs(GLYPH_NONE);
            }
        else if (includeOther == OTHER_OMNI && (currentDisplay == maxValue))
            {
            write3x5Glyphs(GLYPH_OMNI);
            }
        else if (includeOther == OTHER_DEFAULT && (currentDisplay == maxValue))
            {
            write3x5Glyphs(GLYPH_DEFAULT);
            }
        else if (includeOther == OTHER_INCREMENT && (currentDisplay == maxValue))
            {
            write3x5Glyphs(GLYPH_INCREMENT);
            }
        else if (includeOther == OTHER_DECREMENT && (currentDisplay == maxValue))
            {
            write3x5Glyphs(GLYPH_DECREMENT);
            }
        else
            {
            writeNumber(led, led2, currentDisplay);
            if (includeOther == OTHER_GLYPH)
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


// STATE NUMERICAL
// This function performs the basic functions for a state whose entire purpose is to compute a numerical value
// and set *value* to that value.  If updateOptions is true, then stateNumerical will recover from a cancel by
// overwriting the options with the backup options. Otherwise it will recover by overwriting value with backupValue.  
void stateNumerical(uint8_t start, uint8_t end, uint8_t &value, uint8_t &backupValue,
    uint8_t updateOptions, uint8_t includeOff, uint8_t other, uint8_t backState)
    {
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
                
    if (updateDisplay)
        {
        clearScreen();
        drawGlyphForGlyphDisplay(led, glyphs[currentDisplay]);
        if (otherGlyph != NO_GLYPH) drawGlyphForGlyphDisplay(led2, otherGlyph);
        }
        
    return NO_MENU_SELECTED;
    }


void addGaugeNumberNoTrim(uint16_t val)
    {
    char b[6];
    numberToString(b, val);
    addToBuffer(b);
    }

void addGaugeNumber(uint16_t val)
    {
    char b[6];
    char* a;
    numberToString(b, val);
    a = b;
    while(a[0] == ' ') a++;  // trim out initial spaces
    addToBuffer(a);
    }

void writeGaugeNote()
    {
    writeNotePitch(led2, (uint8_t) itemNumber);       // Note
    writeShortNumber(led, (uint8_t) itemValue, false);            // Velocity
    }



//// OCCASIONAL MIDI SIGNALS
////
//// These are occasional (not constant) MIDI signals
//// that we can reasonably display on our gauge.  This is used for STATE_GAUGE_ANY
//// Omitted are: MIDI CLock, Active Sensing, both AfterTouch forms, Time Code Quarter Frame
///



GLOBAL uint8_t  newItem;                        // newItem can be 0, 1, or WAIT_FOR_A_SEC
GLOBAL uint8_t itemType;
GLOBAL uint16_t itemNumber;             // Note on/off/poly aftertouch use this for NOTE PITCH
GLOBAL uint16_t itemValue;                      // Note on/off/poly aftertouch use this for NOTE VELOCITY / AFTERTOUCH


// Used solely for outputting via I2C, which is monophonic and needs to know if a noteOff
// corresponds to the the note it's playing and it should turn it off
GLOBAL uint8_t lastNotePlayed = NO_NOTE;


// some shorthand so we can save a bit of program space.  Of course
// this uses up some of our working memory.  These are PSTR strings
// used in more than one location in the program.  They're set in the .ino file
GLOBAL const char* nrpn_p;// = PSTR("NRPN");
GLOBAL const char* rpn_p;// = PSTR("RPN");
GLOBAL const char* cc_p;// = PSTR("CC");
GLOBAL const char* v_p;// = PSTR("IS");
GLOBAL const char* up_p;// = PSTR("^");
GLOBAL const char* down_p;// = PSTR("%");
GLOBAL const char* voltage_p;// = PSTR("VOLTAGE");
GLOBAL const char* length_p; // = PSTR("LENGTH");
GLOBAL const char* options_p;  // = PSTR("OPTIONS");


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


    // update our internal clock
    if (options.clock == GENERATE_MIDI_CLOCK || 
        options.clock == IGNORE_MIDI_CLOCK || 
        options.clock == BLOCK_MIDI_CLOCK)
        {
        if (currentTime > targetNextPulseTime)
            {
            targetNextPulseTime += microsecsPerPulse;
            pulseClock();
            }
        }

    if (swingTime != 0 && currentTime >= swingTime)
        {
        // play!
        notePulse = 1;
        swingTime = 0;
        }       
    
    if (pulse)
        {
        if (--notePulseCountdown == 0)
            {
            // we may start to swing.  Figure extra swing hold time
            if (swingToggle && options.swing > 0)
                {
                swingTime = currentTime + div100(notePulseRate * microsecsPerPulse * options.swing);
                }
            else
                {
                // redundant with above, but if I move this to a function, the code bytes go up
                notePulse = 1;
                swingTime = 0;
                }                       

            notePulseCountdown = notePulseRate;
                        
            if (options.noteSpeedType == NOTE_SPEED_THIRTY_SECOND || 
                options.noteSpeedType == NOTE_SPEED_SIXTEENTH ||
                options.noteSpeedType == NOTE_SPEED_EIGHTH ||
                options.noteSpeedType == NOTE_SPEED_QUARTER ||
                options.noteSpeedType == NOTE_SPEED_HALF)
                swingToggle = !swingToggle;
            else
                swingToggle = 0;
            }
                
        if (--beatCountdown == 0)
            {
            beat = 1;
            beatCountdown = PULSES_PER_BEAT;
            }
        }
  
    // update the screen, read from the sensors, or update the board LEDs
    updateDisplay = update();
    
    if (isUpdated(BACK_BUTTON, RELEASED_LONG))
        {
        toggleBypass();
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
            const char* menuItems[6] = { PSTR("ARPEGGIATOR"), PSTR("STEP SEQUENCER"), PSTR("RECORDER"), PSTR("GAUGE"), PSTR("CONTROLLER"), options_p };
            doMenuDisplay(menuItems, 6, STATE_ARPEGGIATOR, STATE_ROOT, 1);
            }
        break;  
        case STATE_ARPEGGIATOR:
            {
            stateArpeggiator();
            }
        break;
        case STATE_STEP_SEQUENCER:
            {
            if (entry)
                stopStepSequencer();
            stateLoad(STATE_STEP_SEQUENCER_PLAY, STATE_STEP_SEQUENCER_FORMAT, STATE_ROOT, STATE_STEP_SEQUENCER);
            }
        break;
        case STATE_RECORDER:
            {
            stateLoad(STATE_RECORDER_PLAY, STATE_RECORDER_PLAY, STATE_ROOT, STATE_RECORDER);
            }
        break;
        case STATE_CONTROLLER:
            {            
            const char* menuItems[5] = { PSTR("GO"), PSTR("LEFT KNOB"), PSTR("RIGHT KNOB"), PSTR("MIDDLE BUTTON"), PSTR("RIGHT BUTTON") };
            doMenuDisplay(menuItems, 5, STATE_CONTROLLER_PLAY, STATE_ROOT, 1);
            }
        break;
        case STATE_GAUGE:
            {   
            if (entry) 
                {
                backupOptions = options;
                clearScreen();
                clearBuffer();
                memset(local.gauge.fastMidi, 0, 3);
                entry = false; 
                }
            else
                {
                // at present I'm saying (newItem) rather than (newItem == NEW_ITEM)
                // so even the WAIT_FOR_A_SEC stuff gets sent through.  Note sure
                // if testing for (newItem=1) will result in display starvation
                // when every time the display comes up we have a new incomplete
                // CC, NRPN, or RPN.
        
                if (newItem)
                    {
                    const char* str = NULL;
                                                    
                    if ((itemType > MIDI_ACTIVE_SENSING))   // It's not fast midi
                        {
                        clearScreen();
                        if (itemType < MIDI_CC_7_BIT) // it's not a CC, RPN, or NRPN, and it's not displayable FAST MIDI
                            {
                            clearBuffer(); // so we stop scrolling
                            }
                        }

                    switch(itemType)
                        {
                        // we have the "Fast MIDI" stuff first so we can look it up in the array easily
                                
                        // Fast MIDI stuff comes first so we can use itemType as the index in the fastMidi array :-)
                        case MIDI_CLOCK: 
                            // FALL THRU
                        case MIDI_TIME_CODE: 
                            // FALL THRU
                        case MIDI_ACTIVE_SENSING: 
                            {
                            local.gauge.fastMidi[itemType] = !local.gauge.fastMidi[itemType];
                            }
                        break;
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
                            write3x5Glyph(led2, GLYPH_3x5_A, 0);
                            write3x5Glyph(led2, GLYPH_3x5_T, 4);
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
                            write3x5Glyph(led2, GLYPH_3x5_P, 0);
                            write3x5Glyph(led2, GLYPH_3x5_C, 4);
                            writeShortNumber(led, (uint8_t) itemNumber, false);
                            }
                        break;
                        case MIDI_PITCH_BEND:
                            {
                            writeNumber(led, led2, ((int16_t) itemValue) - 8192);           // pitch bend is actually signed
                            }
                        break;
                        case MIDI_SYSTEM_EXCLUSIVE: 
                            {
                            write3x5Glyphs(GLYPH_SYSEX);
                            }
                        break;
                        case MIDI_SONG_POSITION:
                            {
                            write3x5Glyphs(GLYPH_SONG_POSITION);
                            }
                        break;
                        case MIDI_SONG_SELECT: 
                            {
                            write3x5Glyphs(GLYPH_SONG_SELECT);
                            }
                        break;
                        case MIDI_TUNE_REQUEST:
                            {
                            write3x5Glyphs(GLYPH_TUNE_REQUEST);
                            }
                        break;
                        case MIDI_START: 
                            {
                            write3x5Glyphs(GLYPH_START);
                            }
                        break;
                        case MIDI_CONTINUE:
                            {
                            write3x5Glyphs(GLYPH_CONTINUE);
                            }
                        break;
                        case MIDI_STOP:
                            {
                            write3x5Glyphs(GLYPH_STOP);
                            }
                        break;
                        case MIDI_SYSTEM_RESET: 
                            {
                            write3x5Glyphs(GLYPH_SYSTEM_RESET);
                            }
                        break;
                        case MIDI_CC_7_BIT:
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
                        }
                                
                    if (str != NULL)
                        {                               
                        char b[5];

                        clearBuffer();
                                
                        // If we're incrementing/decrementing, add UP or DOWN
                        // Sadly, compressing this code makes it 2 bytes bigger
                        if ((itemType == MIDI_NRPN_INCREMENT) ||
                            (itemType == MIDI_RPN_INCREMENT))
                            {
                            addToBuffer("   ");             // push down to right side of screen
                            strcpy_P(b, up_p);
                            addToBuffer(b);
                            }
                        else if ((itemType == MIDI_NRPN_DECREMENT) ||
                            (itemType == MIDI_RPN_DECREMENT))
                            {
                            addToBuffer("   ");             // push down to right side of screen
                            strcpy_P(b, down_p);
                            addToBuffer(b);
                            }
                                
                        // else if we're 7-bit CC, just add the value
                        else if (itemType == MIDI_CC_7_BIT)
                            {
                            addGaugeNumberNoTrim(itemValue);
                            }
                                
                        // else add the MSB
                        else
                            {
                            addGaugeNumberNoTrim(itemValue >> 7);
                            }
                                
                        // Next load the name
                        addToBuffer(" "); 
                        strcpy_P(b, str);
                        addToBuffer(b);
                                
                        // Next the number
                        addToBuffer(" ");
                        addGaugeNumber(itemNumber);
                                                                
                        if (itemType != MIDI_CC_7_BIT)          // either we indicate how much we increment/decrement, or show the full 14-bit number
                            {
                            addToBuffer(" (");
                            addGaugeNumber(itemValue);                                      
                            addToBuffer(")");
                            }
                        }
                    }
                else
                    {
                    // Clear the bypass/beat so it can draw itself again,
                    // because we don't update ourselves every single time 
                    for(uint8_t i = 0; i < 8; i++)
                        clearPoint(led, i, 0);          
                    }

                if (getBufferLength() > 0 && updateDisplay)  // we've got a scrollbuffer loaded.  Won't happen on first update.
                    {
                    clearScreen();
                    scrollBuffer(led, led2);
                    }

                if (newItem == WAIT_FOR_A_SEC)
                    newItem = NEW_ITEM;
                else
                    newItem = NO_NEW_ITEM;
                }
                        
            if (isUpdated(BACK_BUTTON, RELEASED))
                {
                goUpStateWithBackup(STATE_ROOT);
                }

            // blink the fast MIDI stuff
            for(uint8_t i = 0; i < 3; i++)
                {
                if (local.gauge.fastMidi[i])
                    {
                    setPoint(led, i + 5, 1);
                    }
                else
                    {
                    clearPoint(led, i + 5, 1);
                    }
                }

            if (options.channelIn == CHANNEL_OFF)
                drawMIDIChannel(CHANNEL_OFF);
            else
                drawMIDIChannel(options.channelIn);                
            }
        break;
        case STATE_OPTIONS:
            {
            if (isUpdated(MIDDLE_BUTTON, RELEASED))
                {
                if (getClockState() == CLOCK_RUNNING)
                    stopClock(true);
                else
                    startClock(true);
                }
                        
            if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
                {
                continueClock(true);
                }

#ifdef USE_DACS
            const char* menuItems[15] = { PSTR("TEMPO"), PSTR("NOTE SPEED"), PSTR("SWING"), PSTR("TRANSPOSE"), 
                                          PSTR("VOLUME"), PSTR("LENGTH"), PSTR("IN MIDI"), PSTR("OUT MIDI"), PSTR("CONTROL MIDI"), PSTR("CLOCK"), 
                                          ((options.click == NO_NOTE) ? PSTR("CLICK") : PSTR("NO CLICK")),
                                          PSTR("BRIGHTNESS"), PSTR("MENU DELAY"), 
                                          (options.voltage ? voltage_p : PSTR("NO VOLTAGE")),
                                          PSTR("GIZMO V1 (C) 2016 SEAN LUKE") };
            doMenuDisplay(menuItems, 15, STATE_OPTIONS_TEMPO, optionsReturnState, 1);
#else
            const char* menuItems[14] = { PSTR("TEMPO"), PSTR("NOTE SPEED"), PSTR("SWING"), PSTR("TRANSPOSE"), 
                                          PSTR("VOLUME"), PSTR("LENGTH"), PSTR("IN MIDI"), PSTR("OUT MIDI"), PSTR("CONTROL MIDI"), PSTR("CLOCK"), 
                                          ((options.click == NO_NOTE) ? PSTR("CLICK") : PSTR("NO CLICK")),
                                          PSTR("BRIGHTNESS"), PSTR("MENU DELAY"), 
                                          PSTR("GIZMO V1 (C) 2016 SEAN LUKE") };
            doMenuDisplay(menuItems, 14, STATE_OPTIONS_TEMPO, optionsReturnState, 1);
#endif
            playApplication();     
            }
        break;
        /*
          case STATE_SPLIT:
          {
          if (entry)      
          {
          entry = false;
          }
        
          // despite the select button release ignoring, we occasionally get button
          // bounces on the select button so I'm moving the main stuff to the middle button
          // and only having long releases on the select button
                
          if (isUpdated(MIDDLE_BUTTON, RELEASED))
          {
          state = STATE_SPLIT_NOTE;
          entry = true;
          }
          else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
          {
          state = STATE_SPLIT_CHANNEL;
          entry = true;
          }
          else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
          {
          options.splitControls = !options.splitControls;
          saveOptions();
          }
          else if (isUpdated(BACK_BUTTON, RELEASED))
          {
          goUpState(STATE_ROOT);
          }
          else if (updateDisplay)
          {
          clearScreen();
          writeNotePitch(led2, options.splitNote);
          write3x5Glyph(led, options.splitControls == 0 ? GLYPH_3x5_R : GLYPH_3x5_L, 5);
          }
          }
          break;
        */
        
        case STATE_SPLIT:
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
            {
            goUpState(STATE_ROOT);
            }
        break;
        case STATE_ARPEGGIATOR_PLAY:
            {
            stateArpeggiatorPlay();
            }
        break;
        case STATE_ARPEGGIATOR_PLAY_OCTAVES:
            {
            stateNumerical(0, ARPEGGIATOR_MAX_OCTAVES, options.arpeggiatorPlayOctaves, backupOptions.arpeggiatorPlayOctaves, true, false, OTHER_NONE,  STATE_ARPEGGIATOR_PLAY);
            playArpeggio();
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
            stateNumerical(1, 17, local.stepSequencer.outMIDI[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, OTHER_DEFAULT, STATE_STEP_SEQUENCER_MENU);
            }
        break;
        case STATE_STEP_SEQUENCER_VELOCITY:
            {
            // yes, it's *128*, not 127, because 128 represents the default velocity
            stateNumerical(0, 128, local.stepSequencer.velocity[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, OTHER_DEFAULT, STATE_STEP_SEQUENCER_MENU);
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_FADER:
            {
            stateNumerical(0, 127, local.stepSequencer.fader[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, OTHER_NONE, STATE_STEP_SEQUENCER_MENU);
            playStepSequencer();
            }
        break;
        case STATE_STEP_SEQUENCER_LENGTH:
            {
            stateNumerical(0, 101, local.stepSequencer.noteLength[local.stepSequencer.currentTrack], local.stepSequencer.backup, false, false, OTHER_DEFAULT, STATE_STEP_SEQUENCER_MENU);
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
        case STATE_RECORDER_MENU:
            {
            stateRecorderMenu();
            }
        break;
        case STATE_CONTROLLER_PLAY:
            {
            int16_t displayValue = -1;          // -1 signifies "off", else displayValue is the number we want to show
            
            if (entry)
                {
                middleButtonToggle = 0;
                selectButtonToggle = 0;
                entry = false;
                }
        
            if (isUpdated(BACK_BUTTON, RELEASED))
                {
                goUpState(STATE_CONTROLLER);
                }
            else
                {
                displayValue = -1;
                // this region is redundant but simplifying to a common function call makes the code bigger 
        
                if (isUpdated(MIDDLE_BUTTON, PRESSED))
                    {
                    middleButtonToggle = !middleButtonToggle;
                    if (options.middleButtonControlType != CONTROL_TYPE_OFF)
                        {
                        displayValue = (middleButtonToggle ? options.middleButtonControlOn : options.middleButtonControlOff); 
                        if (displayValue != 0)
                            sendControllerCommand( options.middleButtonControlType, options.middleButtonControlNumber, displayValue - 1);
                        }
                    }

                if (isUpdated(SELECT_BUTTON, PRESSED))
                    {
                    selectButtonToggle = !selectButtonToggle;
                    if (options.selectButtonControlType != CONTROL_TYPE_OFF)
                        {
                        displayValue = (selectButtonToggle ?  options.selectButtonControlOn :  options.selectButtonControlOff);
                        if (displayValue != 0)
                            sendControllerCommand( options.selectButtonControlType, options.selectButtonControlNumber, displayValue - 1); 
                        }
                    }
        
                if (potUpdated[LEFT_POT] && options.leftKnobControlType != CONTROL_TYPE_OFF)
                    {
#ifdef USE_DACS
                    if (options.leftKnobControlType == CONTROL_TYPE_VOLTAGE_A || options.leftKnobControlType == CONTROL_TYPE_VOLTAGE_B)
                        displayValue = pot[LEFT_POT];
                    else 
#endif
                        displayValue = pot[LEFT_POT] >> 3;
            
                    sendControllerCommand( options.leftKnobControlType, options.leftKnobControlNumber, displayValue);
                    }
          
                if (potUpdated[RIGHT_POT] && options.rightKnobControlType != CONTROL_TYPE_OFF)
                    {
#ifdef USE_DACS
                    if (options.leftKnobControlType == CONTROL_TYPE_VOLTAGE_A || options.leftKnobControlType == CONTROL_TYPE_VOLTAGE_B)
                        displayValue = pot[RIGHT_POT];
                    else 
#endif
                        displayValue = pot[RIGHT_POT] >> 3;
            
                    sendControllerCommand( options.rightKnobControlType, options.rightKnobControlNumber, displayValue); 
                    }
                }
   
            if (updateDisplay)
                {
                clearScreen();
                if (displayValue >= 0)  // isn't "off"
                    writeNumber(led, led2, ((unsigned int)displayValue));
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
            setControllerButtonOnOff(options.middleButtonControlOn, backupOptions.middleButtonControlOn, STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF);
            }
        break;
        case STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_ON:
            {
            setControllerButtonOnOff(options.selectButtonControlOn, backupOptions.selectButtonControlOn, STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF);
            }
        break;
        case STATE_CONTROLLER_SET_MIDDLE_BUTTON_VALUE_OFF:
            {
            setControllerButtonOnOff(options.middleButtonControlOff, backupOptions.middleButtonControlOff, STATE_CONTROLLER);
            }
        break;
        case STATE_CONTROLLER_SET_SELECT_BUTTON_VALUE_OFF:
            {
            setControllerButtonOnOff(options.selectButtonControlOff, backupOptions.selectButtonControlOff, STATE_CONTROLLER);
            }
        break;
        case STATE_OPTIONS_TEMPO:
            {
            if (entry)
                {
                backupOptions = options; 
                }
            uint8_t result = doNumericalDisplay(1, MAXIMUM_BPM, options.tempo, 0, OTHER_NONE);
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
                    goUpStateWithBackup(STATE_OPTIONS);
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
            uint8_t result = doNumericalDisplay(0, 99, options.swing, 0, OTHER_NONE);
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
        case STATE_OPTIONS_TRANSPOSE:
            {
            if (entry)
                {
                backupOptions = options;
                }
                                 
            uint8_t result = doNumericalDisplay(-60, 60, options.transpose, false, OTHER_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    if (options.transpose != currentDisplay)
                        {
                        options.transpose = currentDisplay; 
                        sendAllNotesOff();  // we must have this because if we've changed things we may never get a note off
                        }
                    break;
                case MENU_SELECTED:
                    if (backupOptions.transpose != options.transpose)
                        saveOptions();
                    // FALL THRU
                case MENU_CANCELLED:
                    goUpStateWithBackup(STATE_OPTIONS);
                    sendAllNotesOff();  // we must have this because if we've changed things we may never get a note off
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
        case STATE_OPTIONS_PLAY_LENGTH:
            {
            stateNumerical(0, 100, options.noteLength, backupOptions.noteLength, true, false, OTHER_NONE, STATE_OPTIONS);
            playArpeggio();
            }
        break;
        case STATE_OPTIONS_MIDI_CHANNEL_IN:
            {
            // 0 is CHANNEL_OFF
            // 17 is CHANNEL_OMNI
            stateNumerical(0, 17, options.channelIn, backupOptions.channelIn, true, true, OTHER_OMNI, STATE_OPTIONS);
            }
        break;
        case STATE_OPTIONS_MIDI_CHANNEL_OUT:
            {
            // 0 is CHANNEL_OFF
            stateNumerical(0, 16, options.channelOut, backupOptions.channelOut, true, true, OTHER_NONE, STATE_OPTIONS);
            }
        break;
        case STATE_OPTIONS_MIDI_CHANNEL_CONTROL:
            {
            // 0 is CHANNEL_OFF
            stateNumerical(0, 16, options.channelControl, backupOptions.channelControl, true, true, OTHER_NONE, STATE_OPTIONS);
            }
        break;
        case STATE_OPTIONS_MIDI_CLOCK:
            {
            uint8_t result;
            if (entry) 
                {
                backupOptions = options; 
                }
            const char* menuItems[5] = { PSTR("IGNORE"), PSTR("USE"), PSTR("CONSUME"),  PSTR("GENERATE"), PSTR("BLOCK") };
            result = doMenuDisplay(menuItems, 5, STATE_NONE, STATE_NONE, 1);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    {
                    options.clock = currentDisplay;
                    }
                break;
                case MENU_SELECTED:
                    {
                    if (options.clock != backupOptions.clock)  // got something new
                        {
                        // If I'm moving to being controlled by MIDI I should assume I'm stopped
                        if (options.clock == USE_MIDI_CLOCK || options.clock == CONSUME_MIDI_CLOCK)
                            {
                            stopClock(false);
                            }
                        // In all other cases, I should do a full restart
                        else
                            {
                            stopClock(true);
                            startClock(true);
                            }
                
                        saveOptions();
                        }
                    }
                // FALL THRU
                case MENU_CANCELLED:
                    {
                    goUpStateWithBackup(STATE_OPTIONS);
                    }
                break;
                }
            playApplication();     
            }
        break;
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
                uint8_t note = stateEnterNote(GLYPH_NOTE, STATE_OPTIONS);
                if (note != NO_NOTE)  // it's a real note
                    {
                    options.click = note;
                    options.clickVelocity = stateEnterNoteVelocity;
                    saveOptions();
                    goUpState(STATE_OPTIONS);
                    }
                }
            }
        break;
        case STATE_OPTIONS_SCREEN_BRIGHTNESS:
            {
            if (entry) backupOptions = options; 
            uint8_t result = doNumericalDisplay(1, 16, options.screenBrightness + 1, 0, OTHER_NONE);
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
        case STATE_OPTIONS_MENU_DELAY:
            {
            uint8_t result;
            if (entry)
                {
                backupOptions = options; 
                const uint8_t _glyphs[11] = { (FONT_3x5) + GLYPH_3x5_0,
                    (FONT_8x5) + GLYPH_8x5_ONE_EIGHTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_FOURTH,
                    (FONT_8x5) + GLYPH_8x5_ONE_THIRD,
                    (FONT_8x5) + GLYPH_8x5_ONE_HALF,
                    (FONT_3x5) + GLYPH_3x5_1,
                    (FONT_3x5) + GLYPH_3x5_2,
                    (FONT_3x5) + GLYPH_3x5_3,
                    (FONT_3x5) + GLYPH_3x5_4,
                    (FONT_3x5) + GLYPH_3x5_8,
                    (FONT_8x5) + GLYPH_8x5_INFINITY };
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
#ifdef USE_DACS
        case STATE_OPTIONS_VOLTAGE:
            {
            options.voltage = !options.voltage;
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

/*
  case STATE_SPLIT_CHANNEL:
  {
  stateNumerical(0, 16, options.splitChannel, backupOptions.splitChannel, true, true, OTHER_NONE, STATE_SPLIT);
  }
  break;

  case STATE_SPLIT_NOTE:
  {
  if (stateEnterNote(GLYPH_NOTE, STATE_SPLIT, false) != NO_NOTE)
  {
  options.splitNote = itemNumber;
  saveOptions();
  goUpState(STATE_SPLIT);
  }
  else if (isUpdated(BACK_BUTTON, RELEASED))
  {
  goUpState(STATE_SPLIT);
  }
  }
  break;
*/
     
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
    
    // clear the pots
    potUpdated[LEFT_POT] = potUpdated[RIGHT_POT] = NO_CHANGE;
    }







////////// MIDI HANDLERS  


/// Yet ANOTHER small code-shortening function!
/// Updates incoming midi items only if they came in the proper channel, and returns TRUE
/// if this occurred. 

uint8_t updateMIDI(byte channel, uint8_t _itemType, uint16_t _itemNumber, uint16_t _itemValue)
    {
    TOGGLE_IN_LED();
    if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
        {
        newItem = NEW_ITEM;
        itemType = _itemType;
        itemNumber = _itemNumber;
        itemValue = _itemValue;
        //itemChannel = channel;
        return 1;
        }
    else 
        {
        newItem = NO_NEW_ITEM;
        return 0;
        }
    }


// A code-shortening effort.  Processes a MIDI clock command (start/stop/continue/clock)
void handleClockCommand(void (*clockFunction)(), midi::MidiType clockMIDI)
    {
    newItem = NEW_ITEM;
    itemType = MIDI_CLOCK;

    TOGGLE_IN_LED();
    if (bypass || options.clock == IGNORE_MIDI_CLOCK)  // pass it right through, don't process it
        { 
        if (!bypass) MIDI.sendRealTime(clockMIDI);
        TOGGLE_OUT_LED();
        }
    // note NOT else, so that bypass will pass through the clock even if we "CONSUME" it
    if (options.clock == USE_MIDI_CLOCK || options.clock == CONSUME_MIDI_CLOCK)
        {
        clockFunction();
        }
    }

void handleStart()
    {
    newItem = NEW_ITEM;
    itemType = MIDI_START;
    
    handleClockCommand(startClock, MIDIStart);
    }

void handleStop()
    {
    newItem = NEW_ITEM;
    itemType = MIDI_STOP;
    
    handleClockCommand(stopClock, MIDIStop);
    }

void handleContinue()
    {
    newItem = NEW_ITEM;
    itemType = MIDI_CONTINUE;
    
    handleClockCommand(continueClock, MIDIContinue);
    }
  
void handleClock()
    {
    handleClockCommand(pulseClock, MIDIClock);
    }

/*
  uint8_t applicationSplit()
  {
  if (application == STATE_SPLIT && !bypass)
  {
  TOGGLE_OUT_LED();
  return 1;
  }
  return 0;
  }
*/

void handleNoteOff(byte channel, byte note, byte velocity)
    {
    if (updateMIDI(channel, MIDI_NOTE_OFF, note, velocity))
        {
        if (application == STATE_ARPEGGIATOR && local.arp.playing)
            arpeggiatorRemoveNote(note);
        /*
          else if (application == STATE_SPLIT && !bypass)
          sendNoteOff(note, velocity, note < options.splitNote ? options.splitChannel : options.channelOut);
        */

#ifdef USE_DACS
        if (lastNotePlayed == note)
            {
            if (options.leftKnobControlType != CONTROL_TYPE_VOLTAGE_A &&
                options.rightKnobControlType != CONTROL_TYPE_VOLTAGE_A)
                setNote(DAC_A, 0);
            if (options.leftKnobControlType != CONTROL_TYPE_VOLTAGE_B &&
                options.rightKnobControlType != CONTROL_TYPE_VOLTAGE_B)
                setNote(DAC_B, 0);
            }
#endif
        }

    if (bypass) TOGGLE_OUT_LED();
    }
  
void handleNoteOn(byte channel, byte note, byte velocity)
    {
    if (updateMIDI(channel, MIDI_NOTE_ON, note, velocity))
        {
        if (application == STATE_ARPEGGIATOR && local.arp.playing)
            {
            // the arpeggiation velocity shall be the velocity of the most recently added note
            local.arp.velocity = velocity;
            arpeggiatorAddNote(note);
            }
        /*
          else if (application == STATE_SPLIT && !bypass)
          {
          sendNoteOn(note, velocity, note < options.splitNote ?  options.splitChannel : options.channelOut);
          }
        */

#ifdef USE_DACS
        if (options.leftKnobControlType != CONTROL_TYPE_VOLTAGE_A &&
            options.rightKnobControlType != CONTROL_TYPE_VOLTAGE_A)
            { setNote(DAC_A, note); lastNotePlayed = note; }
        if (options.leftKnobControlType != CONTROL_TYPE_VOLTAGE_B &&
            options.rightKnobControlType != CONTROL_TYPE_VOLTAGE_B)
            { setNote(DAC_B, velocity); lastNotePlayed = note; }
#endif
        }
    
    if (bypass) TOGGLE_OUT_LED();
    }
  
void handleAfterTouchPoly(byte channel, byte note, byte pressure)
    {
    /*
      if (updateMIDI(channel, MIDI_AFTERTOUCH_POLY, note, pressure))
      {
      if (applicationSplit())
      { 
      int16_t n = note + (uint16_t)options.transpose;
      n = bound(n, 0, 127);
      MIDI.sendPolyPressure((uint8_t) n, pressure, note < options.splitNote ?  options.splitChannel : options.channelOut);
      }
      }
    */
    updateMIDI(channel, MIDI_AFTERTOUCH_POLY, note, pressure);

    if (bypass) TOGGLE_OUT_LED();
    }
    





//// CHANNEL CONTROL, NRPN, and RPN
////
//// My parser breaks these out and calls the sub-handlers
//// handleControlChange, handleNRPN, and handleRPN.
//// These are passed certain TYPES of data, indicating the
//// nature of the numerical value.

// Data types
#define VALUE 0                                 // 14-bit values given to CC
#define VALUE_MSB_ONLY 1                // 
#define INCREMENT 2
#define DECREMENT 3
#define VALUE_7_BIT_ONLY 4  // this is last so we have a contiguous switch for NRPN and RPN handlers, which don't use this




void handleControlChange(byte channel, byte number, uint16_t value, byte type)
    {
    // all values that come in are 14 bit with the MSB in the top 7 bits
    // and either the LSB *or* ZERO in the bottom 7 bits, EXCEPT for VALUE_7_BIT_ONLY,
    // which is just the raw number
    if (updateMIDI(channel, MIDI_CC_14_BIT, number, value))
        {
        newItem = NEW_ITEM;
        switch (type)
            {
            case VALUE:
                {
                itemType = MIDI_CC_14_BIT;
                }
            break;
            case VALUE_MSB_ONLY:
                {
                itemType = MIDI_CC_14_BIT;
                newItem = WAIT_FOR_A_SEC;
                }
            break;
            case INCREMENT:
                {
                // never happens
                }
            // FALL THRU
            case DECREMENT:
                {
                // never happens
                }
            // FALL THRU
            case VALUE_7_BIT_ONLY:
                {
                itemType = MIDI_CC_7_BIT;
                }
            break;
            }
        }
    } 
        
        
        
// These are button states sent to use via NRPN messages
GLOBAL uint8_t buttonState[3] = { 0, 0, 0 };

void handleNRPN(byte channel, uint16_t parameter, uint16_t value, uint8_t valueType)
    {
    if (updateMIDI(channel, MIDI_NRPN_14_BIT, parameter, value))
        {
        switch (valueType)
            {
            case VALUE:
                {
                itemType = MIDI_NRPN_14_BIT;
                }
            break;
            case VALUE_MSB_ONLY:
                {
                itemType = MIDI_NRPN_14_BIT;
                newItem = WAIT_FOR_A_SEC;
                }
            break;
            case INCREMENT:
                {
                itemType = MIDI_NRPN_INCREMENT;
                }
            break;
            case DECREMENT:
                {
                itemType = MIDI_NRPN_DECREMENT;
                }
            break;
            }
        }

    if (channel == options.channelControl)  // options.channelControl can be zero remember
        {
        lockoutPots = 1;
                        
        switch (parameter)
            {
            case NRPN_BACK_BUTTON_PARAMETER:
                {
                buttonState[BACK_BUTTON] = (value != 0);
                updateButtons(buttonState); 
                }
            break;
            case NRPN_MIDDLE_BUTTON_PARAMETER:
                {
                buttonState[MIDDLE_BUTTON] = (value != 0);
                updateButtons(buttonState); 
                }
            break;
            case NRPN_SELECT_BUTTON_PARAMETER:
                {
                buttonState[SELECT_BUTTON] = (value != 0);
                updateButtons(buttonState); 
                }
            break;
            case NRPN_LEFT_POT_PARAMETER:
                {
                pot[LEFT_POT] = (value >> 4); 
                potUpdated[LEFT_POT] = CHANGED;
                }
            break;
            case NRPN_RIGHT_POT_PARAMETER:
                {
                pot[RIGHT_POT] = (value >> 4); 
                potUpdated[RIGHT_POT] = CHANGED;
                }
            break;
            case NRPN_BYPASS_PARAMETER:
                {
                toggleBypass();
                }
            break;
            case NRPN_UNLOCK_PARAMETER:
                {
                lockoutPots = 0;
                }
            break;
            case NRPN_START_PARAMETER:
                {
                startClock(true);
                }
            break;
            case NRPN_STOP_PARAMETER:
                {
                stopClock(true);
                }
            break;
            case NRPN_CONTINUE_PARAMETER:
                {
                continueClock(true);
                }
            break;
            }
        }
    }

void handleRPN(byte channel, uint16_t parameter, uint16_t value, uint8_t valueType)
    {
    if (updateMIDI(channel, MIDI_RPN_14_BIT, parameter, value))
        {
        switch (valueType)
            {
            case VALUE:
                {
                itemType = MIDI_NRPN_14_BIT;
                }
            break;
            case VALUE_MSB_ONLY:
                {
                itemType = MIDI_NRPN_14_BIT;
                newItem = WAIT_FOR_A_SEC;
                }
            break;
            case INCREMENT:
                {
                itemType = MIDI_RPN_INCREMENT;
                }
            break;
            case DECREMENT:
                {
                itemType = MIDI_RPN_DECREMENT;
                }
            break;
            }
        }

    }    
    
    
///// INTRODUCTION TO THE CC/RPN/NRPN PARSER
///// The parser is located in handleGeneralControlChange(...), which
///// can be set up to be the handler for CC messages by the MIDI library.
/////
///// CC messages take one of a great many forms, which we handle in the parser
/////
///// 7-bit CC messages:
///// 1. number >=64 and < 96 or >= 102 and < 120, with value
/////           -> handleControlChange(channel, number, value, VALUE_7_BIT_ONLY)
/////
///// Potentially 7-bit CC messages:
///// 1. number >= 0 and < 32, other than 6, with value
/////           -> handleControlChange(channel, number, value, VALUE_MSB_ONLY)
/////
///// 14-bit CC messages:
///// 1. number >= 0 and < 32, other than 6, with MSB
///// 2. same number + 32, with LSB
/////           -> handleControlChange(channel, number, value, VALUE)
/////    NOTE: this means that a 14-bit CC message will have TWO handleControlChange calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 14-bit CC messages:
///// 1. same number again + 32, with LSB
/////           -> handleControlChange(channel, number, revised value, VALUE)
/////           It's not clear if this is valid but I think it is
/////
///// Potentially 7-bit NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 6, with value
/////           -> handleNRPN(channel, parameter, value, VALUE_MSB_ONLY)
/////
///// 14-bit NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 6, with MSB of value
///// 4. number == 38, with LSB of value
/////           -> handleNRPN(channel, parameter, value, VALUE)
/////    NOTE: this means that a 14-bit NRPN message will have TWO handleNRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 7-bit NRPN messages:
///// 4. number == 38 again, with value
/////           -> handleNRPN(channel, number, revised value, VALUE_MSB_ONLY)
/////
///// Continuing 14-bit NRPN messages A:
///// 3. number == 6 again, with MSB of value
/////           -> handleNRPN(channel, number, revised value, VALUE)
/////
///// Continuing 14-bit NRPN messages C:
///// 3. number == 6 again, with MSB of value
///// 4. number == 38 again, with LSB of value
/////           -> handleNRPN(channel, number, revised value, VALUE)
/////    NOTE: this means that a continuing 14-bit NRPN message will have TWO handleNRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Incrementing NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 96, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, INCREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, INCREMENT)
/////
///// Decrementing NRPN messages:
///// 1. number == 99, with MSB of NRPN parameter
///// 2. number == 98, with LSB of NRPN parameter
///// 3. number == 97, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, DECREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, DECREMENT)
/////
///// Potentially 7-bit RPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 6, with value
/////           -> handleNRPN(channel, parameter, value, VALUE_MSB_ONLY)
/////
///// 14-bit RPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 6, with MSB of value
///// 4. number == 38, with LSB of value
/////           -> handleNRPN(channel, parameter, value, VALUE)
/////    NOTE: this means that a 14-bit NRPN message will have TWO handleNRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Continuing 7-bit RPN messages A:
///// 3. number == 6 again, with value
/////           -> handleRPN(channel, number, revised value, VALUE_MSB_ONLY)
/////
///// Continuing 14-bit RPN messages A:
///// 4. number == 38 again, with new LSB of value
/////           -> handleRPN(channel, number, revised value, VALUE)
/////
///// Continuing 14-bit RPN messages B:
///// 3. number == 6 again, with MSB of value
///// 4. number == 38 again, with LSB of value
/////           -> handleRPN(channel, number, revised value, VALUE)
/////    NOTE: this means that a continuing 14-bit RPN message will have TWO handleRPN calls.
/////          There's not much we can do about this, as we simply don't know if the LSB will arrive.  
/////
///// Incrementing RPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 96, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, INCREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, INCREMENT)
/////
///// Decrementing NRPN messages:
///// 1. number == 101, with MSB of RPN parameter other than 127
///// 2. number == 100, with LSB of RPN parameter other than 127
///// 3. number == 97, with value
/////       If value == 0
/////                   -> handleNRPN(channel, parameter, 1, DECREMENT)
/////       Else
/////                   -> handleNRPN(channel, parameter, value, DECREMENT)
/////
///// NULL messages:
///// 1. number == 101, value = 127
///// 2. number == 100, value = 127
/////           [nothing happens, but parser resets]
/////
/////
///// The big problem we have is that the MIDI spec got the MSB and LSB backwards for their data
///// entry values, so we don't now if the LSB is coming and have to either ignore it when it comes
///// in or send two messages, one MSB-only and one MSB+LSB.  This happens for CC, RPN, and NRPN.
/////
/////
///// Our parser maintains four bytes in a struct called _controlParser:
/////
///// 0. status.  This is one of:
/////             INVALID: the struct holds junk.  CC: the struct is building a CC.  
/////                     RPN_START, RPN_END, RPN_MSB, RPN_INCREMENT_DECREMENT: the struct is building an RPN.
/////                     NRPN_START, NRPN_END, NRPN_MSB, NRPN_INCREMENT_DECREMENT: the struct is building an NRPN.
///// 1. controllerNumberMSB.  In the low 7 bits.
///// 2. controllerNumberLSB.  In the low 7 bits.
///// 3. controllerValueMSB.  In the low 7 bits. This holds the previous MSB for potential "continuing" messages.

// Parser status values
#define INVALID 0
#define CC 1
#define NRPN_START 2
#define NRPN_END 3
#define NRPN_MSB 4
#define NRPN_INCREMENT_DECREMENT 5
#define RPN_START 6
#define RPN_END 7
#define RPN_MSB 8
#define RPN_INCREMENT_DECREMENT 9

struct _controlParser
    {
    uint8_t status = INVALID;
        
    // The high bit of the controllerNumberMSB is either
    // NEITHER_RPN_NOR_NRPN or it is RPN_OR_NRPN. 
    uint8_t controllerNumberMSB;
        
    // The high bit of the controllerNumberLSB is either
    // RPN or it is NRPN
    uint8_t controllerNumberLSB;
        
    // The controllerValueMSB is either a valid MSB or it is 
    // NO_MSB (128).
    uint8_t controllerValueMSB;
    };
        


// We have two parsers: one for MIDI IN messages, and one for MIDI CONTROL messages
GLOBAL struct _controlParser midiInParser;
GLOBAL struct _controlParser midiControlParser;

void handleGeneralControlChange(byte channel, byte number, byte value)
    {
    if (bypass) TOGGLE_OUT_LED();
    /*
      else if (application == STATE_SPLIT && channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
      { MIDI.sendControlChange(number, options.channelOut, value); TOGGLE_OUT_LED(); return; }
    */
            
    // we're only interested in parsing CHANNEL IN and CHANNEL CONTROL    
    _controlParser* parser;
    if (channel == options.channelControl)
        {
        parser = &midiControlParser;
        }
    else if (channel == options.channelIn || options.channelIn == CHANNEL_OMNI)
        {
        parser = &midiInParser;
        }
    else return;    
    

    // BEGIN PARSER
    
    
    // potentially 14-bit CC messages
    if (number < 6 || 
        (number > 6 && number < 32))
        {
        parser->status = CC;
        parser->controllerValueMSB = value;
        handleControlChange(channel, number, value << 7, VALUE_MSB_ONLY);
        }
                
    // LSB for 14-bit CC messages, including continuation
    else if (number >= 32 && number < 64 && number != 38)
        {
        if (parser->status == CC)
            {
            handleControlChange(channel, number, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
            }
        else parser->status = INVALID;
        }
                
    // 7-bit only CC messages
    else if ((number >= 64 && number < 96) || 
        (number >= 102 && number < 120))
        {
        parser->status = INVALID;
        handleControlChange(channel, number, value, VALUE_7_BIT_ONLY);
        }
                
    // Start of NRPN
    else if (number == 99)
        {
        parser->status = NRPN_START;
        parser->controllerNumberMSB = value;
        }

    // End of NRPN
    else if (number == 98)
        {
        if (parser->status == NRPN_START)
            {
            parser->status = NRPN_END;
            parser->controllerNumberLSB = value;
            }
        else parser->status = INVALID;
        }
        
    // Start of RPN or NULL
    else if (number == 101)
        {
        if (value == 127)  // this is the NULL termination tradition, see for example http://www.philrees.co.uk/nrpnq.htm
            {
            parser->status = INVALID;
            }
        else
            {
            parser->status = RPN_START;
            parser->controllerNumberMSB = value;
            }
        }

    // End of RPN or NULL
    else if (number == 100)
        {
        if (value == 127)  // this is the NULL termination tradition, see for example http://www.philrees.co.uk/nrpnq.htm
            {
            parser->status = INVALID;
            }
        else if (parser->status == RPN_START)
            {
            parser->status = RPN_END;
            parser->controllerNumberLSB = value;
            }
        }

    // Data Entry MSB for NRPN and RPN
    else 
        {
        uint16_t controllerNumber =  (((uint16_t) parser->controllerNumberMSB) << 7) | parser->controllerNumberLSB ;
        if (number == 6)
            {
            if (parser->status == NRPN_END || parser->status == NRPN_MSB || parser->status == NRPN_INCREMENT_DECREMENT)
                {
                parser->controllerValueMSB = value;
                handleNRPN(channel, controllerNumber, ((uint16_t)value) << 7, VALUE_MSB_ONLY);
                parser->status = NRPN_MSB;
                }
            else if (parser->status == RPN_END || parser->status == RPN_MSB || parser->status == RPN_INCREMENT_DECREMENT)
                {
                parser->controllerValueMSB = value;
                handleRPN(channel, controllerNumber , ((uint16_t)value) << 7, VALUE_MSB_ONLY);
                parser->status = RPN_MSB;
                }
            else parser->status = INVALID;
            }
                
        // Data Entry LSB for RPN, NRPN
        else if (number == 38)
            {
            if (parser->status == NRPN_MSB)
                {
                handleNRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
                }
            else if (parser->status == RPN_MSB)
                {
                handleRPN(channel, controllerNumber, (((uint16_t)parser->controllerValueMSB) << 7) | value, VALUE);
                }
            else parser->status = INVALID;
            }
                
        // Data Increment for RPN, NRPN
        else if (number == 96)
            {
            if (parser->status == NRPN_END || parser->status == NRPN_MSB || parser->status == NRPN_INCREMENT_DECREMENT)
                {
                handleNRPN(channel, controllerNumber , (value ? value : 1), INCREMENT);
                parser->status = NRPN_INCREMENT_DECREMENT;
                }
            else if (parser->status == RPN_END || parser->status == RPN_MSB || parser->status == RPN_INCREMENT_DECREMENT)
                {
                handleRPN(channel, controllerNumber , (value ? value : 1), INCREMENT);
                parser->status = RPN_INCREMENT_DECREMENT;
                }
            else parser->status = INVALID;
            }

        // Data Decrement for RPN, NRPN
        else if (number == 97)
            {
            if (parser->status == NRPN_END || parser->status == NRPN_MSB || parser->status == NRPN_INCREMENT_DECREMENT)
                {
                handleNRPN(channel, controllerNumber , (value ? value : 1), DECREMENT);
                parser->status = NRPN_INCREMENT_DECREMENT;
                }
            else if (parser->status == RPN_END || parser->status == RPN_MSB || parser->status == RPN_INCREMENT_DECREMENT)
                {
                handleRPN(channel, controllerNumber , (value ? value : 1), DECREMENT);
                parser->status = RPN_INCREMENT_DECREMENT;
                }
            else parser->status = INVALID;
            }
        
        // Can only be 120...127, which should never appear here
        else parser->status = INVALID;
        }
    }
  






void handleProgramChange(byte channel, byte number)
    {
    updateMIDI(channel, MIDI_PROGRAM_CHANGE, number, 1);
/*    if (updateMIDI(channel, MIDI_PROGRAM_CHANGE, number, 1))
      {
      if (applicationSplit())
      { MIDI.sendProgramChange(number, options.channelOut); }
      }
*/
    if (bypass) TOGGLE_OUT_LED();
    }
  
void handleAfterTouchChannel(byte channel, byte pressure)
    {
    updateMIDI(channel, MIDI_AFTERTOUCH, 1, pressure);
    /*
      if (updateMIDI(channel, MIDI_AFTERTOUCH, 1, pressure))
      {
      if (applicationSplit())
      { MIDI.sendAfterTouch(pressure, options.channelOut); }
      }
    */

    if (bypass) TOGGLE_OUT_LED();
    }
  
void handlePitchBend(byte channel, int bend)
    {
    updateMIDI(channel, MIDI_PITCH_BEND, 1, (uint16_t) bend + 8192);
    /*
      if (updateMIDI(channel, MIDI_PITCH_BEND, 1, (uint16_t) bend + 8192))
      {                    
      if (applicationSplit())
      { MIDI.sendPitchBend(bend, options.channelOut); }
      }
    */
    
    if (bypass) TOGGLE_OUT_LED();
    }
  
void handleTimeCodeQuarterFrame(byte data)
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_TIME_CODE;

    // always pass through.
    MIDI.sendTimeCodeQuarterFrame(data);
    }
  

void handleSystemExclusive(byte* array, unsigned size)
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_SYSTEM_EXCLUSIVE;

    // always pass through
    MIDI.sendSysEx(size, array);
    }
  
void handleSongPosition(unsigned beats)
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_SONG_POSITION;

    // always pass through
    MIDI.sendSongPosition(beats);
    }

void handleSongSelect(byte songnumber)
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_SONG_SELECT;

    // always pass through
    MIDI.sendSongSelect(songnumber);
    }
  
void handleTuneRequest()
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_TUNE_REQUEST;

    // always pass through
    MIDI.sendTuneRequest();
    }
  
void handleActiveSensing()
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_ACTIVE_SENSING;
        
    // always pass through
    MIDI.sendRealTime(MIDIActiveSensing);
    }
  
void handleSystemReset()
    {
    toggleBothLEDs();
    newItem = NEW_ITEM;
    itemType = MIDI_SYSTEM_RESET;

    // always pass through
    MIDI.sendRealTime(MIDISystemReset);
    }

/// MIDI OUT

void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    if (bypass) return;
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    uint16_t v = velocity;
    if (options.volume < 3)
        v = v >> (3 - options.volume);
    else if (options.volume > 3)
        v = v << (options.volume - 3);
    if (v > 127) v = 127;
    MIDI.sendNoteOn((uint8_t) n, (uint8_t) v, channel);
    TOGGLE_OUT_LED();
    }

void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    if (bypass) return;
    int16_t n = note + (uint16_t)options.transpose;
    n = bound(n, 0, 127);
    MIDI.sendNoteOff((uint8_t) n, velocity, channel);
    // dont' toggle the LED because if we're going really fast it toggles
    // the LED ON and OFF for a noteoff/noteon pair and you can't see the LED
    }
                
void sendAllNotesOff()
    {
    if (bypass) return;
    MIDI.sendControlChange(123, 0, options.channelOut);
    }




