#include "All.h"


#ifdef INCLUDE_MEASURE




#define MEASURE_BEATS_PER_BAR 0
#define MEASURE_BARS_PER_PHRASE 1
#define MEASURE_OPTIONS 2

void stateMeasureMenu()
    {
    uint8_t result;
    const char* menuItems[3] = {    
        PSTR("BEATS PER BAR"),
        PSTR("BARS PER PHRASE"),
        options_p 
        };
    result = doMenuDisplay(menuItems, 3, STATE_NONE, STATE_NONE, 1);
    playMeasure();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            switch(currentDisplay)
                {
                case MEASURE_BEATS_PER_BAR:
                    {
                    goDownState(STATE_MEASURE_BEATS_PER_BAR);   
                    }
                break;
                case MEASURE_BARS_PER_PHRASE:
                    {
                    goDownState(STATE_MEASURE_BARS_PER_PHRASE);                            
                    }
                break;
                case MEASURE_OPTIONS:
                    {
                    optionsReturnState = STATE_MEASURE_MENU;
                    goDownState(STATE_OPTIONS);
                    }
                break;
                }
            }
        break;
        case MENU_CANCELLED:
            {
            // get rid of any residual select button calls, so we don't stop when exiting here
            isUpdated(SELECT_BUTTON, RELEASED);
            goUpState(STATE_MEASURE);
            }
        break;
        }
        
    }

// Select Button Starts/Stops

void resetMeasure()
    {
    local.measure.initialTime = currentTime;
    local.measure.beatsSoFar = 0;
    }
        
void playMeasure()
    {
    if (beat && local.measure.running)
        {
        local.measure.beatsSoFar++;
        }
    }

void stateMeasure()
    {
    if (entry)
        {
        resetMeasure();  // this will also reset the counter
        local.measure.displayElapsedTime = false;
        local.measure.running = false;
        entry = false;
        }
                
    playMeasure();
        
    if (updateDisplay)
        {
        clearScreen();
        if (local.measure.displayElapsedTime)
            {
            if (local.measure.beatsSoFar == 0)
                {
                writeShortNumber(led2, 0, true);
                writeShortNumber(led, 0, false);
                }
            else
                {               
                uint16_t eighthsSoFar = (uint16_t)((currentTime - local.measure.initialTime) / 100000);
                uint16_t secondsSoFar = eighthsSoFar / 8;
                uint16_t eighths = eighthsSoFar % 8;
                uint16_t minutes = secondsSoFar / 60;
                uint16_t seconds = secondsSoFar % 60;
                                                
                if (minutes > 127)              // this cannot possibly happen since time is only 72 minutes long!
                    {
                    // Write "HI" on led2
                    write3x5Glyph(led2, GLYPH_3x5_H, 0);
                    write3x5Glyph(led2, GLYPH_3x5_I, 4);
                    }
                else
                    {
                    writeShortNumber(led2, minutes, true);
                    }
                                                
                writeShortNumber(led, seconds, false);
                drawRange(led2, 0, 0, 16, eighths);                     // This will use the whole width I think
                }
                                
            setPoint(led, 7, 1);
            }
        else
            {
            if (local.measure.beatsSoFar == 0)
                {
                writeShortNumber(led2, 1, true);
                writeShortNumber(led, 1, false);
                }
            else
                {               
                // we adjust things so that the first "beat" is *0*
                uint16_t beatsSoFar = local.measure.beatsSoFar - 1;
                        
                // this is going to be quite costly, but it's simple
                uint16_t measuresSoFar = beatsSoFar / options.measureBeatsPerBar;
                uint16_t beats = beatsSoFar % options.measureBeatsPerBar;
                uint16_t phrases = measuresSoFar / options.measureBarsPerPhrase;
                uint16_t measures = (uint8_t)(measuresSoFar % options.measureBarsPerPhrase);
                        
                if (phrases > 127) // can't display it
                    {
                    // Write "HI" on led2
                    write3x5Glyph(led2, GLYPH_3x5_H, 0);
                    write3x5Glyph(led2, GLYPH_3x5_I, 4);
                    }
                else
                    {
                    writeShortNumber(led2, phrases + 1, true);
                    }
                                                                
                writeShortNumber(led, measures + 1, false);
                // the idea here is to rotate things so as draw an EMPTY RANGE when we're on BEAT ZERO
                drawRange(led2, 0, 0, 16, beats);
                }
            
            setPoint(led, 6, 1);
            }
                        
        if (local.measure.running)
            {
            setPoint(led, 3, 1);
            }
        }
                
    if (isUpdated(BACK_BUTTON, RELEASED))  // time to exit
        {
        goUpState(STATE_ROOT);
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        if (local.measure.running)
            {
            resetMeasure();
            }
        else
            {
            local.measure.running = true;
            }
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        local.measure.running = false;
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        local.measure.displayElapsedTime = !local.measure.displayElapsedTime;
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        goDownState(STATE_MEASURE_MENU);
        }
    }


        
#endif
