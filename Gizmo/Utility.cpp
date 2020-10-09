////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License
//////
////// UTILITY
//////
////// Utility.h/.cpp define various utility functions used by a variety of applications
////// 



#include "All.h"


/// IMMEDIATE RETURN FACILITY

#define NO_AUTO_RETURN (0)
GLOBAL uint32_t autoReturnTime;
GLOBAL uint8_t autoReturn;
GLOBAL uint8_t immediateReturn;
GLOBAL uint8_t immediateReturnState;

void allowImmediateReturn(uint8_t state)
    {
    immediateReturnState = state;
    immediateReturn = true;
    }
    
void allowAutoReturn(uint8_t state)
    {
    allowImmediateReturn(state);
    autoReturn = true;
    }
    
void removeAutoReturnTime()
    {
    autoReturnTime = NO_AUTO_RETURN_TIME_SET;
    autoReturn = false;
    }
    
void setAutoReturnTime()
    {
//    if (entry)
//        {
    if (autoReturn && (options.autoReturnInterval != NO_AUTO_RETURN))
        {
        autoReturnTime = tickCount + ((uint32_t)3125) * (uint32_t)(options.autoReturnInterval);
        }
    else 
        {
        removeAutoReturnTime();
        }
//        }
    //autoReturn = false;
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
//    MENU ITEM (not STATE) as defaultMenuValue.  backState is entirely
//    ignored.  As the user is scrolling through various options,
//    doMenuDisplay will return NO_MENU_SELECTED, but you can see which
//    item is being considered as currentDisplay.  If the user selects a
//    menu item, then MENU_SELECTED is set (again the item
//    is currentDisplay).  Finally if the user goes back, then
//    MENU_CANCELLED is returned.



// Divide this into the pot[LEFT_POT] to determine what the menu state should be.
// If negative, then MULTIPLY pot[LEFT_POT] by -potDivisor to determine what the state should be.
GLOBAL int16_t potDivisor;


GLOBAL static const char* menu[MAX_MENU_ITEMS];                        // This is an array of pointers into PROGMEM
GLOBAL int16_t currentDisplay;                                                             // currently displayed menu item

// NOTE: This creates a temporary char buffer of length MAX_MENU_ITEM_LENGTH.
// It's better than storing a buffer 8xMAX_MENU_ITEM_LENGTH though.
// So you need to subtract about 28 (for the moment) from the local variable storage left

#define FORCE_NEW_DISPLAY 255

uint8_t doMenuDisplay(const char** _menu, uint8_t menuLen, uint8_t baseState, uint8_t backState, uint8_t scrollMenu, uint8_t extra) //  = 0)
    {
    // What we'd like to display next.  By default it's set to currentDisplay to indicate that we're not changing
    uint8_t newDisplay;
    
    if (entry)
        {
                
        // copy over the PSTRs but don't convert them.
        memcpy(menu, _menu, menuLen * sizeof(const char*));
    
        // can't avoid a divide :-(
        potDivisor = 1024 / menuLen;
        
        // what do we display first
        if (baseState == STATE_NONE)                                            // These aren't states
            {
            currentDisplay = defaultMenuValue;                                  // Display the first item unless the default state was specified
            defaultMenuValue = 0;
            }
        else                                                                                                                                    // These are states
            {
            if (defaultState == STATE_NONE)                                     // We don't have a default state specified.  So use the first item
                currentDisplay = 0;
            else                                                                // We have a default state.  So use it
                currentDisplay = defaultState - baseState;
            }
                
        newDisplay = FORCE_NEW_DISPLAY;                                         // This tells us that we MUST compute a new display
        defaultState = STATE_NONE;                                              // We're done with this
        setExtraButton(DOWN_BUTTON, 0);

        setAutoReturnTime();

        entry = false;
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
            setAutoReturnTime();
            }
        else if (isUpdated(MIDDLE_BUTTON, RELEASED))
            {
            newDisplay++;
            if (newDisplay >= menuLen)
                newDisplay = 0; 
            setAutoReturnTime();
            }
        else if (getExtraButton(DOWN_BUTTON))
            {
            if (newDisplay == 0)
                newDisplay = menuLen - 1;
            else
                newDisplay--;
            setExtraButton(DOWN_BUTTON, 0);
            setAutoReturnTime();
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
            application = FIRST_APPLICATION + currentDisplay;
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        removeAutoReturnTime();
        if (baseState != STATE_NONE)
            {
            if (state == STATE_ROOT)
                return NO_MENU_SELECTED;
            goUpState(backState);
            }
        return MENU_CANCELLED;
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED) || ((autoReturnTime != NO_AUTO_RETURN_TIME_SET) && TIME_GREATER_THAN(tickCount, autoReturnTime)))   //(tickCount > autoReturnTime)))
        {
        /// This code seems to cure a heisenbug in the compiler.  Without it, 
        /// auto-return is magically turned ON in the arpeggiator's arpeggio type
        /// selector.  I'm guessing it's because the call to debug(...) is not inlineable
        /// and this forces some stack check on the  || ((autoReturnTime != NO_AUTO_RETURN_TIME_SET) && TIME_GREATER_THAN(tickCount, autoReturnTime)))
        /// bit.

        if (autoReturnTime != NO_AUTO_RETURN_TIME_SET)
            {
            if (tickCount <= autoReturnTime)                // this will never happen due to the if-statement above
                debug(999);
            }
                        
        /// End purposeless heisenbug fix

        if (baseState != STATE_NONE)
            {
            state = baseState + currentDisplay;
            entry = true;
            }
            
        removeAutoReturnTime();        
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
GLOBAL uint8_t secondGlyph = NO_GLYPH;

uint8_t doNumericalDisplay(int16_t minValue, int16_t maxValue, int16_t defaultValue, uint8_t includeOff, uint8_t includeOther)
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
        setExtraButton(DOWN_BUTTON, 0);

        setAutoReturnTime();
        entry = false;
        }
    
    /*
      if (autoReturnTime != NO_AUTO_RETURN_TIME_SET)
      {
      debug(998);
      }
                
      if (autoReturnTime != NO_AUTO_RETURN_TIME_SET && 
      !TIME_GREATER_THAN(tickCount, autoReturnTime) &&
      (tickCount > autoReturnTime))
      {
      debug(999);
      }
    */
                        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        removeAutoReturnTime();
        return MENU_CANCELLED;
        }
    else if (isUpdated(SELECT_BUTTON, PRESSED) || (autoReturnTime != NO_AUTO_RETURN_TIME_SET && TIME_GREATER_THAN(tickCount, autoReturnTime)))   //(tickCount > autoReturnTime)))
        {
        removeAutoReturnTime();
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
        setAutoReturnTime();
        }
    else if (getExtraButton(DOWN_BUTTON))
        {
        if (currentDisplay == minValue)
            currentDisplay = maxValue;
        else
            currentDisplay--;
        setExtraButton(DOWN_BUTTON, 0);
        setAutoReturnTime();
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        currentDisplay++;
        if (currentDisplay > maxValue)
            currentDisplay = minValue; 
        setAutoReturnTime();
        }
    
    // if we're doing a high-resolution number (> 128 basically) then
    // we check the right pot and use it as a fine-tuning
    if (potUpdated[RIGHT_POT] && (maxValue - minValue + 1) > 128)
        {
        // this is gonna have funky effects at the boundaries
        
        currentDisplay -= potFineTune;  // old one
        potFineTune = (pot[RIGHT_POT] >> 3) - 64;  // right pot always maps to a delta of -64 ... 63.
        currentDisplay += potFineTune;

        currentDisplay = boundValue(currentDisplay, minValue, maxValue);
        setAutoReturnTime();
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
    if (entry)
        {
        setAutoReturnTime();

        currentDisplay = defaultValue;
        // can't avoid a divide this time!
        potDivisor = 1024 / numGlyphs;
        memcpy(glyphs, _glyphs, numGlyphs);
        entry = false;
        setExtraButton(DOWN_BUTTON, 0);
        }
    
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        removeAutoReturnTime();
        return MENU_CANCELLED;
        }
    else if (isUpdated(SELECT_BUTTON, PRESSED) || (autoReturnTime != NO_AUTO_RETURN_TIME_SET && TIME_GREATER_THAN(tickCount, autoReturnTime)))   //(tickCount > autoReturnTime)))
        {
        removeAutoReturnTime();
        return MENU_SELECTED;
        }
    else if (potUpdated[LEFT_POT])
        {
        // can't avoid a divide this time!
        currentDisplay = (uint8_t) (pot[LEFT_POT] / potDivisor); // ((potUpdated[LEFT_POT] ? pot[LEFT_POT] : pot[RIGHT_POT]) / potDivisor);
        if (currentDisplay >= numGlyphs)
            currentDisplay = numGlyphs - 1;
        setAutoReturnTime();
        }
    else if (getExtraButton(DOWN_BUTTON))
        {
        if (currentDisplay == 0)
            currentDisplay = numGlyphs - 1;
        else
            currentDisplay--;
        setExtraButton(DOWN_BUTTON, 0);
        setAutoReturnTime();
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        currentDisplay++;
        if (currentDisplay  >= numGlyphs)
            currentDisplay = 0; 
        setAutoReturnTime();
        }
                
    if (updateDisplay)
        {
        clearScreen();
        drawGlyphForGlyphDisplay(led, glyphs[currentDisplay]);
        if (otherGlyph != NO_GLYPH)
            drawGlyphForGlyphDisplay(led2, otherGlyph);
        }
        
    return NO_MENU_SELECTED;
    }







//// COMPUTING MEDIANS

// presently unused
uint16_t medianOfFive(uint16_t array[])
    {
    uint16_t a[5];
    uint16_t c;
    memcpy(a, array, 5 * sizeof(uint16_t));

// From http://stackoverflow.com/questions/11350471/finding-median-of-5-elements        
// 1) use 3 comparisons to arrange elements in array such that a[1]<a[2] , a[4]<a[5] and a[1]<a[4]
// a) compare a[1] and a[2] and swap if necessary
        
    if (a[0] > a[1]) { c = a[0]; a[0] = a[1]; a[1] = c; }
        
// b) compare a[4] and a[5] and swap if necessary 

    if (a[3] > a[4]) { c = a[3]; a[3] = a[4]; a[4] = c; }
        
// c) compare a[1] and a[4].if a[4] is smaller than a[1] , then swap a[1] wid a[4] and a[2] wid a[5]

    if (a[0] > a[3]) { c = a[0]; a[0] = a[3]; a[3] = c; 
        c = a[1]; a[1] = a[4]; a[4] = c; }

// 2)if a[3]>a[2].if a[2]<a[4] median value = min(a[3],a[4]) else median value=min(a[2],a[5]) 

    if (a[2] > a[1])
        {
        if (a[1] < a[3])
            {
            return (a[2] < a[3] ? a[2] : a[3]);
            }
        else
            {
            return (a[1] < a[4] ? a[1] : a[4]);
            }
        }

// 3)if a[3]<a[2].if a[3]>a[4] median value = min(a[3],a[5]) else median value=min(a[2],a[4])
        
    else
        {
        if (a[3] < a[2])
            {
            return (a[2] < a[4] ? a[2] : a[4]);
            }
        else
            {
            return (a[1] < a[3] ? a[1] : a[3]);
            }
        }
    }




////// DEBUGGING CODE


uint8_t debug(int16_t val)
    {
    clearMatrix(led);
    clearMatrix(led2);
    writeNumber(led, led2, val);
    sendMatrix(led, led2);
    delay(500);
    return 1;
    }

uint8_t debug(const char* str, int8_t val)
    {
    char s[3];
    strncpy_P(s, str, 2);
    s[2] = 0;
    clearMatrix(led);
    clearMatrix(led2);
    memcpy_P(led2, font_3x5[GLYPH_3x5_A + s[0] - 'A'], 3);
    memcpy_P(led2 + 5, font_3x5[GLYPH_3x5_A + s[1] - 'A'], 3);
    writeShortNumber(led, val, false);
    sendMatrix(led, led2);
    delay(500);
    return 1;
    }
        
uint8_t debug(int8_t val1, int8_t val2)
    {
    clearMatrix(led);
    clearMatrix(led2);
    writeShortNumber(led2, val1, true);
    writeShortNumber(led, val2, false);
    sendMatrix(led, led2);
    delay(500);
    return 1;
    }










void goDownState(uint8_t nextState)
    {
    goUpState(nextState);
    defaultState = STATE_NONE;
    }

void goUpState(uint8_t nextState)
    {
    defaultState = state; 
    state = nextState;
    entry = true;
    clearReleased();
    }


void goUpStateWithBackup(uint8_t _nextState)
    {
    goUpState(_nextState);
    options = backupOptions;
    }

void goDownStateWithBackup(uint8_t _nextState)
    {
    goDownState(_nextState);
    options = backupOptions;
    }





#ifdef INCLUDE_STEP_SEQUENCER
        
// Starting at position pos, distributes the bits of the given byte among the high bytes >= pos
// Note that the bits are in reverse order: the high bit is the first one,
// and the low bit is the last one.
void distributeByte(uint16_t pos, uint8_t byte)
    {
    for(uint8_t i = 0; i < 8; i++)
        {
        data.slot.data.stepSequencer.buffer[pos + i] = 
            ((data.slot.data.stepSequencer.buffer[pos + i] & 127) | (byte & 128));
        byte = (byte << 1);
        }
    }

// Gathers high bits starting at position pos to form a complete byte.
// Note that the bits are in reverse order: the high bit is the first one,
// and the low bit is the last one.
uint8_t gatherByte(uint16_t pos)
    {
    uint8_t byte = 0;
    for(uint8_t i = 0; i < 8; i++)
        {
        byte = ((byte << 1) | (data.slot.data.stepSequencer.buffer[pos + i] >> 7));
        }
    return byte;
    }
        
void stripHighBits()
    {
    // strip the high bits
    for(uint16_t i = 0; i < STEP_SEQUENCER_BUFFER_SIZE; i++)
        data.slot.data.stepSequencer.buffer[i] = (127 & data.slot.data.stepSequencer.buffer[i]);
    }

#endif INCLUDE_STEP_SEQUENCER









////// GENERIC STATE FUNCTIONS


// Private function, used by stateSave and stateLoad
// Displays all the slots, and their slot type, and lets the user choose
uint8_t stateSaveLoad(uint8_t includeOff)
    {
    // Maybe search for first empty slot first?  Not doing it now though.
    if (entry)
        {
        for(uint8_t i = 0; i < NUM_SLOTS; i++)
            {
            glyphs[i] = getSlotType(i);
            }
        }
    return doNumericalDisplay(includeOff ? -1 : 0, (NUM_SLOTS - 1), 0, includeOff, GLYPH_OTHER);
    }




// Saves a slot, either for the recorder or the sequencer. 
void stateSave(uint8_t backState)
    {
    secondGlyph = GLYPH_3x5_S;
    uint8_t result = stateSaveLoad(false);
    entry = false;
    switch (result)
        {
        case NO_MENU_SELECTED:
            break;
        case MENU_SELECTED:
            data.slot.type = slotTypeForApplication(application);
            switch(application)
                {
#ifdef INCLUDE_STEP_SEQUENCER
                case STATE_STEP_SEQUENCER:
                    {
                    uint8_t len = GET_TRACK_FULL_LENGTH();
                    uint8_t num = GET_NUM_TRACKS();
                                        
                    // pack the high-bit parts
                    for(uint8_t i = 0; i < num; i++)
                        {
                        uint16_t pos = i * len * 2;
                        //// 1 bit type of data
                        if (local.stepSequencer.data[i] == STEP_SEQUENCER_DATA_NOTE)
                            {
                            distributeByte(pos, 0 << 7);  // Note data is a 0
                            
                            //// 1 bit mute
                            //// 5 bits MIDI out channel (including "use default")
                            //// 7 bits length
                            //// 7 bits velocity (including "use per-note velocity")
                            //// 1 bit transposable
                            //// 5 bits fader
                            //// 4 bits pattern
        
                            distributeByte(pos + 1, local.stepSequencer.muted[i] << 7);  // will work for STEP_SEQUENCER_MUTED and STEP_SEQUENCER_MUTE_OFF_SCHEDULED
                            distributeByte(pos + 2, local.stepSequencer.outMIDI[i] << 3);
                            distributeByte(pos + 7, local.stepSequencer.noteLength[i] << 1);
                            distributeByte(pos + 14, local.stepSequencer.velocity[i] << 1);      
                            distributeByte(pos + 21, local.stepSequencer.transposable[i] << 7);      
                            distributeByte(pos + 22, local.stepSequencer.fader[i] << 3);
                            distributeByte(pos + 27, local.stepSequencer.pattern[i] << 4);
                            }
                        else
                            {
                            distributeByte(pos, 1 << 7);  // Control data is a 1
                                
                            ////     3 bits: CC, NRPN, RPN, PC, BEND, AFTERTOUCH
                            ////     7 bits MSB of Parameter 
                            ////         7 bits LSB of Parameter
                            ////     5 bits MIDI out channel
                            ////         4 bits pattern

                            uint8_t controlDataType = local.stepSequencer.data[i] - 1;
                            distributeByte(pos + 1, controlDataType << 4);
                            distributeByte(pos + 4, local.stepSequencer.noteLength[i] << 1);                // MSB of Control Parameter
                            distributeByte(pos + 11, local.stepSequencer.velocity[i] << 1);                 // LSB of Control Parameter
                            distributeByte(pos + 18, local.stepSequencer.outMIDI[i] << 3);
                            distributeByte(pos + 23, local.stepSequencer.pattern[i] << 4);
                            }
                        }
                    saveSlot(currentDisplay);
                    stripHighBits();                        
                    }
                break;
#endif INCLUDE_STEP_SEQUENCER
#ifdef INCLUDE_RECORDER
                case STATE_RECORDER:
                    {
                    saveSlot(currentDisplay);
                    }
                break;
#endif INCLUDE_RECORDER
#ifdef INCLUDE_DRUM_SEQUENCER
                case STATE_DRUM_SEQUENCER:
                    {
                    packDrumSequenceData();
                    saveSlot(currentDisplay);
                    }
                break;
#endif INCLUDE_DRUM_SEQUENCER
                }
                                        
            goUpState(backState);
            break;
        case MENU_CANCELLED:
            goUpState(backState);
            break;
        }
    }




void stateLoad(uint8_t selectedState, uint8_t initState, uint8_t backState, uint8_t _defaultState)
    {
    secondGlyph = GLYPH_3x5_L;
    uint8_t result = stateSaveLoad(true);
    entry = false;
    switch (result)
        {
        case NO_MENU_SELECTED:
            break;
        case MENU_SELECTED:
            if (currentDisplay == -1)  // Init
                {
                state = initState;
                }
            else
                {
                loadSlot(currentDisplay);
                        
                if ((data.slot.type != slotTypeForApplication(application)))
                    {
                    state = initState;
                    }
                else
                    {
                    state = selectedState;
#ifdef INCLUDE_DRUM_SEQUENCER
                    if (application == STATE_DRUM_SEQUENCER)
                        {
                        //// FIXME:  Need to set the note pulse rate first before the following....? (which was stolen from StepSequencer code in Utility.cpp)
                        //// FIXME:  Need to stop the sequencer?  Etc.

                        unpackDrumSequenceData();
                        local.drumSequencer.playState = PLAY_STATE_STOPPED;
                        local.drumSequencer.performanceMode = false;

                        // FIXME: did I fix the issue of synchronizing the beats with the sequencer notes?
                        local.drumSequencer.currentPlayPosition = div12((24 - beatCountdown) * notePulseRate) >> 1;   // get in sync with beats
                        }
                    else
#endif INCLUDE_DRUM_SEQUENCER
#ifdef INCLUDE_STEP_SEQUENCER
                        if (application == STATE_STEP_SEQUENCER)
                            {
                            // FIXME: did I fix the issue of synchronizing the beats with the sequencer notes?
                            local.stepSequencer.currentPlayPosition = div12((24 - beatCountdown) * notePulseRate) >> 1;   // get in sync with beats

                            uint8_t len = GET_TRACK_FULL_LENGTH();
                            uint8_t num = GET_NUM_TRACKS();
                                
                            // unpack the high-bit info
                            for(uint8_t i = 0; i < num; i++)
                                {
                                uint16_t pos = i * len * 2;

                                //// 1 bit type of data
                                if (gatherByte(pos) >> 7 == 0)  // It's a note
                                    {
                                    //// 1 bit mute
                                    //// 5 bits MIDI out channel (including "use default")
                                    //// 7 bits length
                                    //// 8 bits velocity (including "use per-note velocity")
                                    //// 4 bits fader
                                    local.stepSequencer.data[i] = STEP_SEQUENCER_DATA_NOTE;
                                
                                    local.stepSequencer.muted[i] = (gatherByte(pos + 1) >> 7); // first bit
                                    local.stepSequencer.outMIDI[i] = (gatherByte(pos + 2) >> 3);  // top 5 bits moved down 3
                                    local.stepSequencer.noteLength[i] = (gatherByte(pos + 7) >> 1); // top 7 bits moved down 1
                                    local.stepSequencer.velocity[i] = (gatherByte(pos + 14) >> 1); // top 7 bits moved down 1
                                    local.stepSequencer.transposable[i] = (gatherByte(pos + 21) >> 7); // top 1 bits moved down 7
                                    local.stepSequencer.fader[i] = (gatherByte(pos + 22) >> 3);  // top 5 bits moved down 3
                                    local.stepSequencer.pattern[i] = (gatherByte(pos + 27) >> 4);  // top 4 bits moved down 4
                                    }
                                else                        // It's a control sequence
                                    {                               
                                    ////     3 bits: CC, NRPN, RPN, PC, BEND, AFTERTOUCH
                                    ////     7 bits MSB of Parameter 
                                    ////     7 bits LSB of Parameter
                                    ////     5 bits MIDI out channel
                                    ////     4 bits pattern

                                    uint8_t controlDataType = (gatherByte(pos + 1) >> 4);
                                    local.stepSequencer.data[i] = controlDataType + 1;
                                    local.stepSequencer.noteLength[i] = (gatherByte(pos + 4) >> 1);         // MSB
                                    local.stepSequencer.velocity[i] = (gatherByte(pos + 11) >> 1);          // LSB
                                    local.stepSequencer.outMIDI[i] = (gatherByte(pos + 18) >> 3);
                                    local.stepSequencer.pattern[i] = (gatherByte(pos + 23) >> 4);
                                    }
                                }
                            
                            stripHighBits();
                        
                            local.stepSequencer.markTrack = 0;
                            local.stepSequencer.markPosition = 0;
                            resetStepSequencerCountdown();
                            }
                        else
#endif INCLUDE_STEP_SEQUENCER
                            {}
                    }
                }

#ifdef INCLUDE_STEP_SEQUENCER
            if (application == STATE_STEP_SEQUENCER)
                {                                
                local.stepSequencer.solo = 0;
                local.stepSequencer.currentTrack = 0;
                local.stepSequencer.transpose = 0;
                setParseRawCC(local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC);
                local.stepSequencer.currentEditPosition = 0;
                stopStepSequencer();
                }
#endif INCLUDE_STEP_SEQUENCER
                
            defaultState = STATE_NONE;
            entry = true;
            
            break;
        case MENU_CANCELLED:
            goUpState(backState);
            defaultState = _defaultState;
            break;
        }
    }





void stateSure(uint8_t cancelState, uint8_t sureState)
    {
    if (updateDisplay)
        {
        clearScreen();
        write8x5Glyph(led2, GLYPH_8x5_SURE_PT1);
        write8x5Glyph(led, GLYPH_8x5_SURE_PT2);
        }

    if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        goDownState(sureState);
        }
    else if (isUpdated(BACK_BUTTON, PRESSED))
        {
        // just go back, don't reenter
        state = cancelState;
        }
    }

void stateExit(uint8_t cancelState, uint8_t exitState)
    {
    if (updateDisplay)
        {
        clearScreen();
        write8x5Glyph(led2, GLYPH_8x5_EXIT_PT1);
        write8x5Glyph(led, GLYPH_8x5_EXIT_PT2);
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        // send ALL NOTES OFF
        MIDI.sendControlChange(123, 0, options.channelOut);
        
        goUpState(exitState);
        defaultState = cancelState;
        }
    else if (isUpdated(SELECT_BUTTON, PRESSED))
        {
        // just go back, don't reenter
        state = cancelState;
        }
    }

void stateCant(uint8_t nextState)
    {
    if (updateDisplay)
        {
        clearScreen();
        write3x5Glyphs(GLYPH_CANT);
        }

    if (isUpdated(BACK_BUTTON, RELEASED) || isUpdated(SELECT_BUTTON, RELEASED))
        {
        state = nextState;
        }
    }



// bounds n to be between min and max 
uint8_t bound(uint8_t n, uint8_t min, uint8_t max)
    {
    if (n > max) n = max;
    if (n < min) n = min;
    return n;
    }

// increments n, then wraps it to 0 if it is >= max
uint8_t incrementAndWrap(uint8_t n, uint8_t max)
    {
    if (++n >= max)
        return 0;
    return n;
    }

        
GLOBAL uint8_t stateEnterNoteVelocity;

uint8_t stateEnterNote(uint8_t backState)
    {
    if (entry)
        {
        newItem = 0;            // clear any current note
        clearScreen();
        write3x5Glyphs(GLYPH_NOTE);
        entry = false;
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(backState);
        }
    
    // process these but do nothing with them so if the user
    // accidentally presses these buttons BEFORE he chooses a note,
    // the button press isn't queued for later -- I do that a lot.
    isUpdated(SELECT_BUTTON, RELEASED);
    isUpdated(MIDDLE_BUTTON, RELEASED);
        
    if (newItem == NEW_ITEM && itemType == MIDI_NOTE_ON)
        {
        // we don't send a NOTE ON here because we have no easy place to STOP playing it
        stateEnterNoteVelocity = itemValue;  // velocity
        return itemNumber;
        }
        
    return NO_NOTE;
    }


GLOBAL uint8_t chordCount;

uint8_t stateEnterChord(uint8_t* chord, uint8_t maxChordNotes, uint8_t backState)
    {
    if (entry)
        {
        newItem = 0;            // clear any current note
        clearScreen();
        write3x5Glyphs(GLYPH_CHORD);
        entry = false;
        chordCount = 0;
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(backState);
        }
    
    // process these but do nothing with them so if the user
    // accidentally presses these buttons BEFORE he chooses a note,
    // the button press isn't queued for later -- I do that a lot.
    isUpdated(SELECT_BUTTON, RELEASED);
    isUpdated(MIDDLE_BUTTON, RELEASED);
        
    if (newItem == NEW_ITEM && chordCount < maxChordNotes)
        {
        if (itemType == MIDI_NOTE_ON)
            {
            for(uint8_t i = 0; i < chordCount; i++)
                {
                if ((chord[i] & 127) == itemNumber)
                    return NO_NOTE;  // already have that one
                }
                
            // okay, so it's a new note
            chord[chordCount] = (itemNumber | 128);
            chordCount++;
            stateEnterNoteVelocity = itemValue;  // velocity
            }
        else if (itemType == MIDI_NOTE_OFF)
            {
            // first mark the note as up
                
            for(uint8_t i = 0; i < chordCount; i++)
                {
                if ((chord[i] & 127) == itemNumber)
                    {
                    chord[i] = itemNumber;  // move off of +128 if it's already there
                    break;
                    }
                }
                        
            // next check to see if any notes are still down
            // (this could easily be done with a count, but we only have a few notes, so no biggie)
            for(uint8_t i = 0; i < chordCount; i++)
                {
                if (chord[i] >= 128)  // something's being held down
                    return NO_NOTE;
                }

            // at this point nothing is held down.  Insertion Sort FTW!
            int8_t j;  // j goes negative so we have to be signed
            for(uint8_t i = 1; i < chordCount; i++)
                {
                uint8_t val = chord[i];
                j = i - 1;
                while((j >= 0) && (chord[j] > val))
                    {
                    chord[j + 1] = chord[j];
                    j = j - 1;
                    }
                chord[j + 1] = val;
                }

            // return the count
            return chordCount;
            }
        }
        
    return NO_NOTE;
    }


void playApplication()
    {
    switch(immediateReturnState)
        {
#ifdef INCLUDE_ARPEGGIATOR
        case STATE_ARPEGGIATOR_MENU:
        case STATE_ARPEGGIATOR_PLAY:
            playArpeggio();          
            break; 
#endif INCLUDE_ARPEGGIATOR
#ifdef INCLUDE_STEP_SEQUENCER
        case STATE_STEP_SEQUENCER_PLAY:
        case STATE_STEP_SEQUENCER_MENU:
            playStepSequencer();
            break;
#endif INCLUDE_STEP_SEQUENCER
#ifdef INCLUDE_DRUM_SEQUENCER
        case STATE_DRUM_SEQUENCER_PLAY:
        case STATE_DRUM_SEQUENCER_MENU:
            playDrumSequencer();
            break;
#endif INCLUDE_DRUM_SEQUENCER
#ifdef INCLUDE_RECORDER
        case STATE_RECORDER_PLAY:  // note not MENU: we go directly to options from PLAY
            // This is a dummy function, which we include to keep the switch statement from growing by 100 bytes (!)
            // Because we do NOT want to play the recorder in the background ever.
            playRecorder();
            break; 
#endif INCLUDE_RECORDER
#ifdef INCLUDE_MEASURE
        case STATE_MEASURE:
            playMeasure();
            break;
#endif INCLUDE_MEASURE
#ifdef INCLUDE_THRU
        case STATE_THRU_PLAY:
            playThru();
            break;
#endif INCLUDE_THRU
        }
    }
        



//// CLEARSCREEN()
//
// Clears the screen buffer.  If there are two screens, clears both of them.

void clearScreen()
    {
    clearMatrix(led);
    clearMatrix(led2);
    }






GLOBAL static uint8_t glyphTable[24][4] = 
    {
    // These first: ----, ALLC, DFLT, DECR, and INCR, must be the FIRST ones
    // because the correspond with the five glyph types in doNumericalDisplay
    {GLYPH_3x5_MINUS, GLYPH_3x5_MINUS, GLYPH_3x5_MINUS, GLYPH_3x5_MINUS},   // ----
    {GLYPH_3x5_A, GLYPH_3x5_L, GLYPH_3x5_L, GLYPH_3x5_C},   // ALLC
    {GLYPH_3x5_D, GLYPH_3x5_F, GLYPH_3x5_L, GLYPH_3x5_T},   // DFLT
    {GLYPH_3x5_D, GLYPH_3x5_E, GLYPH_3x5_C, GLYPH_3x5_R},   // DECR
    {GLYPH_3x5_I, GLYPH_3x5_N, GLYPH_3x5_C, GLYPH_3x5_R},   // INCR
    {GLYPH_3x5_F, GLYPH_3x5_R, GLYPH_3x5_E, GLYPH_3x5_E},   // FREE
    {GLYPH_3x5_N, GLYPH_3x5_O, GLYPH_3x5_T, GLYPH_3x5_E},   // NOTE
    {GLYPH_3x5_S, GLYPH_3x5_Y, GLYPH_3x5_S, GLYPH_3x5_X},   // SYSX
    {GLYPH_3x5_S, GLYPH_3x5_P, GLYPH_3x5_O, GLYPH_3x5_S},   // SPOS
    {GLYPH_3x5_S, GLYPH_3x5_S, GLYPH_3x5_E, GLYPH_3x5_L},   // SSEL
    {GLYPH_3x5_T, GLYPH_3x5_R, GLYPH_3x5_E, GLYPH_3x5_Q},   // TREQ
    {GLYPH_3x5_S, GLYPH_3x5_T, GLYPH_3x5_R, GLYPH_3x5_T},   // STRT
    {GLYPH_3x5_C, GLYPH_3x5_O, GLYPH_3x5_N, GLYPH_3x5_T},   // CONT
    {GLYPH_3x5_S, GLYPH_3x5_T, GLYPH_3x5_O, GLYPH_3x5_P},   // STOP
    {GLYPH_3x5_R, GLYPH_3x5_S, GLYPH_3x5_E, GLYPH_3x5_T},   // RSET
    {GLYPH_3x5_F, GLYPH_3x5_A, GLYPH_3x5_D, GLYPH_3x5_E},   // FADE
    {GLYPH_3x5_P, GLYPH_3x5_L, GLYPH_3x5_A, GLYPH_3x5_Y},   // PLAY
    {GLYPH_3x5_C, GLYPH_3x5_H, GLYPH_3x5_R, GLYPH_3x5_D},   // CHRD
    {GLYPH_3x5_H, GLYPH_3x5_I, GLYPH_3x5_G, GLYPH_3x5_H},   // HIGH
    {GLYPH_3x5_T, GLYPH_3x5_R, GLYPH_3x5_A, GLYPH_3x5_N},   // TRAN
    {GLYPH_3x5_F, GLYPH_3x5_A, GLYPH_3x5_I, GLYPH_3x5_L},   // FAIL
    {GLYPH_3x5_L, GLYPH_3x5_O, GLYPH_3x5_O, GLYPH_3x5_P},   // LOOP
    {GLYPH_3x5_M, GLYPH_3x5_O, GLYPH_3x5_R, GLYPH_3x5_E},   // MORE
    {GLYPH_3x5_C, GLYPH_3x5_A, GLYPH_3x5_N, GLYPH_3x5_T},   // CANT
    };


// Writes any of the above glyph sets to the screen
void write3x5Glyphs(uint8_t index)
    {
    uint8_t *glyphs = glyphTable[index];
    write3x5Glyph(led2, glyphs[0], 0);
    write3x5Glyph(led2, glyphs[1], 4);
    write3x5Glyph(led, glyphs[2], 0);
    write3x5Glyph(led, glyphs[3], 4);
    }



void drawMIDIChannel(uint8_t channel)
    {
    // finally, draw the channel
    if (channel == CHANNEL_OMNI)
        {
        for(uint8_t i = 2; i < 6; i++)
            setPoint(led2, i, 0);
        }
    else if (channel == CHANNEL_OFF)
        {
        setPoint(led2, 0, 0);
        setPoint(led2, 2, 0);
        setPoint(led2, 5, 0);
        setPoint(led2, 7, 0);
        }
    else
        {
        drawRange(led2, 0, 0, 16, (uint8_t)(channel - 1));
        }
    }


GLOBAL static uint8_t clickNote = NO_NOTE;
        
void doClick()
    {
    // turn off previous click
    if (pulse && (clickNote != NO_NOTE))
        {
        sendNoteOff(clickNote, 127, options.channelOut);
        clickNote = NO_NOTE;
        }
                
    // turn on new click
    if (beat && (options.click != NO_NOTE))
        {
        sendNoteOn(options.click, options.clickVelocity, options.channelOut);
        clickNote = options.click;
        }
    }




///// SCROLL DELAY
///// 


////// We only redraw once every 32 ticks.  A tick is 1/3125 sec.
////// The rule is that the delay value is multplied by 12, unless it is HIGH_MENU_DELAY, which is infinite,
////// or SLOW_MENU_DELAY, which is the same as DEFAULT_MENU_DELAY except that the standard text scroll
////// is slowed by 4, or NO_MENU_DELAY, which has no initial delay.
////// So for example a delay of 1 = 1 * 12 * 32 / 3125 - 0.12288 ~ 0.125 sec.

GLOBAL static uint8_t menuDelays[11] = { NO_MENU_DELAY, QUARTER_MENU_DELAY, THIRD_MENU_DELAY, HALF_MENU_DELAY, DEFAULT_MENU_DELAY, DOUBLE_MENU_DELAY, TREBLE_MENU_DELAY, QUADRUPLE_MENU_DELAY, EIGHT_TIMES_MENU_DELAY, HIGH_MENU_DELAY, SLOW_MENU_DELAY };

// SET MENU DELAY
// Changes the menu delay to a desired value (between 0: no menu delay, and 9: infinite menu delay to 10: slow delay).  
// The default is 5
void setMenuDelay(uint8_t index)
    {
    uint16_t del = DEFAULT_SHORT_DELAY;
    uint16_t val = menuDelays[index];
    
    if (val == 0)               // no delay, so we do the same thing as the default short delay
        {
        val = DEFAULT_SHORT_DELAY;
        }
    else if (val == HIGH_MENU_DELAY)    // Don't scroll at all.
        {
        val = NO_SCROLLING;
        }
    else if (val == SLOW_MENU_DELAY)
        {
        val = 12 * DEFAULT_MENU_DELAY;
        del = DEFAULT_SHORT_DELAY * 2;
        }
    else 
        {
        val = 12 * val;
        }
    setScrollDelays(val, del);
    }
    
