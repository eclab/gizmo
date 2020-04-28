////// Copyright 2020 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"



#ifdef INCLUDE_GAUGE


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




void stateGauge()
	{
            if (entry) 
                {
                backupOptions = options;
                clearScreen();
                clearBuffer();
                memset(local.gauge.fastMidi, 0, 3);
                setParseRawCC(options.gaugeMidiInProvideRawCC);
                entry = false; 
                }
            else
                {
                uint8_t dontShowValue = 0;

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
                                        
                if (isUpdated(SELECT_BUTTON, RELEASED))
                    {
                    setParseRawCC(options.gaugeMidiInProvideRawCC = !options.gaugeMidiInProvideRawCC);
                    saveOptions();
                    }
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
                            case MIDI_CC_7_BIT:
                                {
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
                            	if (itemValue == RPN_NULL)  // note FALL THRU
                            		{
                            		newItem = NO_NEW_ITEM;
                            		str = NULL; 
                            		}
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
                                if (!dontShowValue)
                                    addGaugeNumberNoTrim(itemValue);
                                }
                                                                
                            // else add the MSB
                            else
                                {
                                addGaugeNumberNoTrim(itemValue >> 7);
                                }
                                                                
                            // Next load the name
                            if (!dontShowValue)
                                addToBuffer(" "); 
                            strcpy_P(b, str);
                            addToBuffer(b);
                                                                
                            if (itemType != MIDI_CC_7_BIT || itemNumber < 120)  // we don't want channel mode drawing here
                                {       
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
                        }
                    else if (newItem != NO_NEW_ITEM)                   // Fast MIDI is all that's left.  RPN_NULL will trigger NO_NEW_ITEM
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

                if (options.gaugeMidiInProvideRawCC)
                    setPoint(led, 5, 1);
                else
                    clearPoint(led, 5, 1);

                // At any rate...                
                // Clear the bypass/beat so it can draw itself again,
                // because we don't update ourselves every single time 
                for(uint8_t i = 0; i < 8; i++)
                    clearPoint(led, i, 0);          
                }
     

            if (isUpdated(BACK_BUTTON, RELEASED))
                {
                setParseRawCC(false);
                goUpStateWithBackup(STATE_ROOT);
                }
        	}

#endif INCLUDE_GAUGE
