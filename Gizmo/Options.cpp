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
#if defined(__MEGA__)
    setMenuDelay(options.menuDelay);
#endif defined(__MEGA__)
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
#endif INCLUDE_ARPEGGIATOR

    options.clockDivisor = 1;
    options.transpose = 0;
    options.volume = 3;  // corresponds to no volume modification
#if defined(__MEGA__)
    options.menuDelay = DEFAULT_MENU_DELAY;  // corresponds to DEFAULT_MENU_DELAY
#endif defined(__MEGA__)

#ifdef INCLUDE_RECORDER
    options.recorderRepeat = true;
#endif INCLUDE_RECORDER

#ifdef INCLUDE_SPLIT
    options.splitChannel = 1;
    options.splitNote = 60;  // Middle C
    options.splitLayerNote = NO_NOTE;
#endif INCLUDE_SPLIT

#ifdef INCLUDE_STEP_SEQUENCER
    options.stepSequencerRestNote = STEP_SEQUENCER_REST_NOTE;			// no rest note
#endif INCLUDE_STEP_SEQUENCER

#ifdef INCLUDE_DRUM_SEQUENCER
    options.drumSequencerDefaultVelocity = 5;
#endif INCLUDE_DRUM_SEQUENCER

#ifdef INCLUDE_MEASURE
    options.measureBeatsPerBar = 4;
    options.measureBarsPerPhrase = 4;
#endif INCLUDE_MEASURE

#if defined(HEADLESS_RESET)
    options.channelControl = 16;
#endif defined(HEADLESS_RESET)

#ifdef INCLUDE_CONTROLLER
    // Length values for waves 2 through 8 are OFF by default.
    // Length value for wave 1 is 0 by default, hence we start at 3.
    for(uint8_t i = 3; i < 16; i+=2)
        {
        options.waveEnvelope[i] = 255;
        }
    options.randomInitialValue = 64;
    options.randomRange = 127;
#endif INCLUDE_CONTROLLER
    }




void stateOptionsTempo()
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
            if (immediateReturn)
                goUpStateWithBackup(immediateReturnState);
            else
                goUpStateWithBackup(STATE_OPTIONS);
            setPulseRate(options.tempo);
            break;
        }
    playApplication();       
    }
        
void stateOptionsNoteSpeed()
    {
    if (entry)
        {
        setAutoReturnTime();
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
    if (isUpdated(BACK_BUTTON, RELEASED) || i || (autoReturnTime != NO_AUTO_RETURN_TIME_SET && tickCount > autoReturnTime))
        {
        if (i || (autoReturnTime != NO_AUTO_RETURN_TIME_SET && tickCount > autoReturnTime))  // we don't want to call isUpdated(SELECT_BUTTON, ...) again as it resets things
            {
            if (backupOptions.noteSpeedType != options.noteSpeedType)
                {
                saveOptions();
                }
            }
        removeAutoReturnTime();
                            
        // at any rate...
        if (immediateReturn)
            goUpStateWithBackup(immediateReturnState);
        else
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
        setAutoReturnTime();
        }
    playApplication();   
    }
        
void stateOptionsSwing()
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
            if (immediateReturn)
                goUpStateWithBackup(immediateReturnState);
            else
                goUpStateWithBackup(STATE_OPTIONS);
            break;
        }
    playApplication();     
    }
        
void stateOptionsTranspose()
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
            if (immediateReturn)
                goUpStateWithBackup(immediateReturnState);
            else
                goUpStateWithBackup(STATE_OPTIONS);
            sendAllSoundsOff();  // we must have this because if we've changed things we may never get a note off
            }
        break;
        }
    playApplication();      
    }
        
void stateOptionsVolume()
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
            if (immediateReturn)
                goUpStateWithBackup(immediateReturnState);
            else
                goUpStateWithBackup(STATE_OPTIONS);
            break;
        }
    playApplication();       
    }
        
void stateOptionsMIDIClock()
    {
    uint8_t result;
    if (entry) 
        {
        backupOptions = options; 
        defaultMenuValue = options.clock;  // so we display the right thing
        }
    const char* menuItems[6] = { PSTR("USE"), PSTR("CONSUME"), PSTR("IGNORE"), PSTR("GENERATE"), PSTR("MERGE"), PSTR("BLOCK") };
    result = doMenuDisplay(menuItems, 6, STATE_NONE, STATE_NONE, 1);
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
        
void stateOptionsClick()
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
            if (immediateReturn)
                goUpState(immediateReturnState);
            else
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
            if (immediateReturn)
                goUpState(immediateReturnState);
            else
                goUpState(STATE_OPTIONS);
            }
        }
    playApplication();
    }
        
void stateOptionsScreenBrightness()
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
        
#if defined(__MEGA__)
void stateOptionsMenuDelay()
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
#endif defined(__MEGA__)
