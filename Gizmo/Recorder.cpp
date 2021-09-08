////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"



#ifdef INCLUDE_RECORDER

/// PLAY STATES
/// These are situations the recorder may find itself in.
/// Don't confuse these with the STATUS VALUES in Recorder.h

#define NOT_ENDED 0                             // Song playing/recording isn't over yet
#define ENDED 1                                 // Playing/recording is over




/// Resets the recorder entirely.  Called on MIDI Start etc.
void resetRecorder()
    {
    local.recorder.tick = -1;
    local.recorder.currentPos = 0;
    local.recorder.bufferPos = 0;
    local.recorder.iterations = 0;
    }
        


// Possible values for the 'load' parameter in recorderLoadNote.
// These are carefully chosen so as to make them usable in setting the high bit.
#define LOAD_NOTE_OFF 128
#define LOAD_NOTE_ON 0

// Private helper method for stateRecorderPlay() for packing notes for storage.
// Packs a NOTE ON or NOTE OFF into the buffer.  If the note is a NOTE OFF, also
// sends a NoteOFF message to MIDI, and clears the NoteOFF ID, making it available.
// Increases the recorder.length, currentPos, and recorder.notes
// appropriately.  
void recorderLoadNote(uint8_t load, uint8_t id, uint16_t time)
    {
    // take the high three bits of time and put them in the low 3 bits, plus 1 in the top bit, and the ID in bits 3, 4, 5, 6
    data.slot.data.recorder.buffer[data.slot.data.recorder.length] = 
        (uint8_t)((((uint8_t)(time >> 8)) & (4 + 2 + 1)) | (id << 3) | load);
    // take the remaining 8 bits of time and stick them in the next byte
    data.slot.data.recorder.buffer[data.slot.data.recorder.length + 1] = 
        (uint8_t)(time & 255);
    if (load == LOAD_NOTE_ON)
        {
        data.slot.data.recorder.length += 4;
        local.recorder.currentPos++;
        local.recorder.numNotes++;
        }
    else // LOAD_NOTE_OFF
        {
        data.slot.data.recorder.length += 2;
        sendNoteOff(local.recorder.notes[id], 127, options.channelOut);
        local.recorder.notes[id] = NO_NOTE;  // make available
        }
    }


// This is a dummy function which does nothing at all, because we can't presently
// play in the background.  But it's included because if we DON'T have it, then
// Utility.playApplication() increases by 100 bytes.  :-(
void playRecorder() { } 



// Private helper method for stateRecorderPlay() for drawing the measure and note positions.
// Draws a single point at position (item % 16, yoffset + item / 16), where
// yoffset = 0 is the top left hand corner, and larger yoffset increases as you go down.
// This is basically plotting a point in a 16 x N rectangle starting at yoffset.
void recorderDrawPoint(uint8_t item, uint8_t yoffset)
    {
    uint8_t y = 7 - yoffset - (item >> 4);  // integer div by 16
    uint8_t x = (item - (item >> 4) * 16);  // remainder

    if (x < 8)
        setPoint(led2, x, y);
    else
        setPoint(led, x-8, y);
    }



uint8_t atMeasureBoundary(int16_t tick)
    {
    // mod 96
    return div3(tick >> 5) * 96 == tick;            // tick >> 5 is dividing by 32, so div3 gets us to 96
    }


// Plays OR Records the song
void stateRecorderPlay()
    {
    uint8_t ended = NOT_ENDED;
       
    if (entry)
        {
        // we're having problems coming back to Play from the Options menu and going somewhere
        // else.  I'm gonna see if this fixed things
        //clearReleased();
                        
        resetRecorder();
        local.recorder.status = RECORDER_STOPPED;
        local.recorder.tickoff = 0;
        if ((currentDisplay == -1) || (data.slot.type != slotTypeForApplication(STATE_RECORDER))) // initialize
            {
            data.slot.data.recorder.length = 0;
            local.recorder.numNotes = 0;
            }
        entry = false;
        }
                
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        ended = ENDED;
        goUpState(STATE_RECORDER_EXIT);
        }
        
    // The middle button does play/stop
    // If we're playing, or recording, we set ended = ENDED (which stops and does all notes off and resets the ticks 
    // If we're stopped, we start playing
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        if (local.recorder.status == RECORDER_PLAYING || local.recorder.status == RECORDER_RECORDING)
            {
            ended = ENDED;
            }
        else 
            {
            local.recorder.status = RECORDER_PLAYING;
            }
        local.recorder.playScheduled = false;
        }
    
    // The long middle button starts a record
    // If we're doing ANYTHING other than ticking off or recording, start ready-to-record
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        local.recorder.status = RECORDER_TICKING_OFF;
        local.recorder.tickoff = 0;
        data.slot.type = SLOT_TYPE_RECORDER;
        // send ALL NOTES OFF
        sendAllSoundsOff();
        }
    
    // the long select button pops up the options  
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        immediateReturnState = STATE_RECORDER_PLAY;  
        goDownState(STATE_RECORDER_MENU);
        // we don't allow playing in the menu in recorder -- if you choose the menu, you stop.
        ended = ENDED;
        }

    // the select button stops everything and calls save
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        if (local.recorder.tick == -1) 
            {
            resetRecorder();
            sendAllSoundsOff();
            local.recorder.status = RECORDER_PLAYING;
            }
        else
            {
            local.recorder.playScheduled = !local.recorder.playScheduled;
            }
        }
        
    // Notice no else, so the select button can start playing immediately if appropriate.

    // Formats:
    //  NOTE ON:   [0], id (4 bits) time (11 bits), pitch (7 bits + 1 extra) velocity (7 bits + 1 extra)
    //  NOTE OFF:  [1], id (4 bits) time (11 bits)
    //  Note that  the largest legal time value is 2015 (the end of 21 4-beat measures)
                                                        
    if (pulse && (local.recorder.status == RECORDER_PLAYING))
        {
        local.recorder.tick++;
              
        if (data.slot.data.recorder.length != 0)        // don't want an fast unnecessary loop here
            {
            if (local.recorder.playScheduled && atMeasureBoundary(local.recorder.tick))
                {
                resetRecorder();
                sendAllSoundsOff();
                local.recorder.status = RECORDER_PLAYING;
                local.recorder.playScheduled = false;
                }
            else if ((local.recorder.bufferPos >= data.slot.data.recorder.length && atMeasureBoundary(local.recorder.tick)) ||  // out of notes and at a measure boundary, ugh, divide by 96
                (local.recorder.tick > MAXIMUM_RECORDER_TICK))  // out of time
                {
                local.recorder.iterations++;
                uint8_t maxiter = data.slot.data.recorder.repeat & 0x0F;
                if (maxiter != 0)       // Loop forever
                    {
                    const uint8_t maxIterations[16] = { 255, 1, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 };
                    if (maxIterations[maxiter] == local.recorder.iterations)        // we reached our loop
                        {
                        uint8_t where = (data.slot.data.recorder.repeat >> 4);
                        if (where == 0 || getSlotType(where - 1) != slotTypeForApplication(STATE_RECORDER))     // OFF or incorrect slot
                            {
                            // stop
                            ended = ENDED;
                            }
                        else
                            {
                            sendAllSoundsOff();
                            loadSlot(where - 1);
                            resetRecorder();
                            local.recorder.status = RECORDER_PLAYING;
                            }
                        }
                    else
                        {
                        sendAllSoundsOff();
                        local.recorder.currentPos = 0;
                        local.recorder.bufferPos = 0;
                        local.recorder.tick = 0;
                        }
                    }
                else
                    {
                    sendAllSoundsOff();
                    local.recorder.currentPos = 0;
                    local.recorder.bufferPos = 0;
                    local.recorder.tick = 0;
                    }
                }
                        
            if (!ended)
                {
                // we could have a number of items stored for this tick
                while ((local.recorder.bufferPos < data.slot.data.recorder.length) &&
                        (local.recorder.tick >=  // just in case we're below the tick but not equal to it.
                        // time is stored in the low three bits of the first byte, plus the
                        // next entire byte
                            ((((uint16_t)((data.slot.data.recorder.buffer[local.recorder.bufferPos]) & (4 + 2 + 1))) << 8) | 
                            data.slot.data.recorder.buffer[local.recorder.bufferPos + 1])))  /// ... the next note time
                    {
                    // id is in bytes 3, 4, 5, 6 of the first byte
                    uint8_t id = (data.slot.data.recorder.buffer[local.recorder.bufferPos] & (64 + 32 + 16 + 8)) >> 3;
                                                
                    // NOTE OFF is indicated by a 1 in the high bit of the first byte
                    if (data.slot.data.recorder.buffer[local.recorder.bufferPos] & LOAD_NOTE_OFF)
                        {
                        // NOTE OFF
                        sendNoteOff(local.recorder.notes[id], 127, options.channelOut);
                        local.recorder.bufferPos += 2;
                        local.recorder.notes[id] = NO_NOTE;
                        }
                    else
                        {
                        // NOTE ON

                        if (local.recorder.notes[id] != NO_NOTE)
                            {
                            // not sure what happened here, this should have been off
                            sendNoteOff(local.recorder.notes[id], 127, options.channelOut);
                            }
                                                                                                                
                        // pitch is the third byte.  We assume it's already 0...127
                        uint8_t pitch = data.slot.data.recorder.buffer[local.recorder.bufferPos + 2];
                        // velocity is the fourth byte.  We assume it's already 0...127
                        uint8_t velocity = data.slot.data.recorder.buffer[local.recorder.bufferPos + 3];
                        sendNoteOn(pitch, velocity, options.channelOut);
                        local.recorder.notes[id] = pitch;
                        //tempID = id;
                        local.recorder.currentPos++;
                        local.recorder.bufferPos += 4;
                        }
                    }
                }
            }

        // rerouting to new channel
        if (newItem && 
            itemType != MIDI_CUSTOM_CONTROLLER && 
            local.recorder.status == RECORDER_PLAYING)
            {
            TOGGLE_IN_LED();
            // figure out what the channel should be
            uint8_t channelOut = options.recorderPlayAlongChannel;
            if (channelOut == CHANNEL_DEFAULT_MIDI_OUT)
                channelOut = options.channelOut;
                                
            // send the appropriate command
            if (channelOut != NO_MIDI_OUT)
                {
                if (itemType == MIDI_NOTE_ON)
                    {
                    sendNoteOn(itemNumber, itemValue, channelOut);
                    }
                else if (itemType == MIDI_NOTE_OFF)
                    {
                    sendNoteOff(itemNumber, itemValue, channelOut);
                    }
                else if (itemType == MIDI_AFTERTOUCH_POLY)
                    {
                    sendPolyPressure(itemNumber, itemValue, channelOut);
                    }
                else if (itemType == MIDI_AFTERTOUCH)
                    {
                    sendControllerCommand(CONTROL_TYPE_AFTERTOUCH, 0, itemValue, channelOut);
                    }
                else if (itemType == MIDI_CC_7_BIT)  // we're always raw
                    {
                    sendControllerCommand(CONTROL_TYPE_CC, itemNumber, itemValue << 7, channelOut);
                    }
                else if (itemType == MIDI_PROGRAM_CHANGE)
                    {
                    sendControllerCommand(CONTROL_TYPE_PC, 0, itemValue, channelOut);
                    }
                else if (itemType == MIDI_PITCH_BEND)
                    {
                    sendControllerCommand(CONTROL_TYPE_PITCH_BEND, 0, itemValue, channelOut);
                    }
                else
                    {
                    // do nothing
                    }
                }
            }
        }
    else if ((local.recorder.status == RECORDER_RECORDING) || (local.recorder.tickoff == 4))
        {
        if (pulse && !local.recorder.tickoff)  // we're not in the preliminary period
            local.recorder.tick++;

        if  (local.recorder.tickoff != 4) // don't click when in tickoff, you're already clicking!
            doClick();
                
        if (local.recorder.playScheduled && atMeasureBoundary(local.recorder.tick))
            {
            ended = ENDED;
            }
        if ((local.recorder.tick > MAXIMUM_RECORDER_TICK) || (local.recorder.bufferPos > (RECORDER_BUFFER_SIZE - RECORDER_SIZE_OF_NOTE_OFF)))
            {
            ended = ENDED;
            }
        else
            {
            // record!
            if (newItem)
                {
                uint16_t time = (local.recorder.tick <= 0 ? 0 : local.recorder.tick);
                                
                if ((!local.recorder.tickoff) &&
                    //(2 * (targetNextPulseTime - currentTime) <= getMicrosecsPerPulse()))  // if we're closer to the NEXT pulse than we are to the CURRENT one
                    (TIME_GREATER_THAN_OR_EQUAL(getMicrosecsPerPulse(), 2 * (targetNextPulseTime - currentTime))))  // if we're closer to the NEXT pulse than we are to the CURRENT one
                    {
                    // round up to next
                    time++;
                    }

                uint8_t id = MAX_RECORDER_NOTES_PLAYING + 1;    // indicates an invalid or unknown ID
                                                                        
                if ((itemType == MIDI_NOTE_ON) && 
                    (local.recorder.numNotes < MAX_RECORDER_NOTES) && 
                    (data.slot.data.recorder.length <= (RECORDER_BUFFER_SIZE - RECORDER_SIZE_OF_NOTE_ON)))  // this last one isn't really technically needed but it's a good idea 
                    {
                    // find an open id slot
                    for(uint16_t i = 0; i < MAX_RECORDER_NOTES_PLAYING; i++)
                        {
                        if (local.recorder.notes[i] == NO_NOTE)
                            { id = i; break; }                            
                        }
                                
                    if (id == MAX_RECORDER_NOTES_PLAYING + 1)  // uh oh, no slot.  Get rid of id 0
                        {
                        // load a NOTE_OFF at id 0
                        recorderLoadNote(LOAD_NOTE_OFF, 0, time);
                        id = 0;
                        }
                                                                                                                                                                        
                    // load the slot
                    local.recorder.notes[id] = itemNumber;  // the note proper
                                                                                        
                    // load a NOTE_ON at the id
                    recorderLoadNote(LOAD_NOTE_ON, id, time);

                    // load the note pitch
                    data.slot.data.recorder.buffer[data.slot.data.recorder.length+2 - 4] = itemNumber;  // the note.  We subtract 4 because recorderLoadNote has already added it
                    // load the velocity
                    data.slot.data.recorder.buffer[data.slot.data.recorder.length+3 - 4] = itemValue;  // the velocity
                    sendNoteOn(itemNumber, itemValue, options.channelOut);

                    }
                else if ((itemType == MIDI_NOTE_OFF) && 
                    (data.slot.data.recorder.length <= (RECORDER_BUFFER_SIZE - RECORDER_SIZE_OF_NOTE_OFF)))
                    {
                    // find the id slot
                    for(uint16_t i = 0; i < MAX_RECORDER_NOTES_PLAYING; i++)
                        {
                        if (local.recorder.notes[i] == itemNumber)
                            { id = i; break; }                            
                        }
                                                                                                        
                    if (id == MAX_RECORDER_NOTES_PLAYING + 1)  // uh oh, we already got cut!
                        {
                        // do nothing 
                        }
                    else
                        {
                        // load a NOTE_OFF at id
                        recorderLoadNote(LOAD_NOTE_OFF, id, time);
                        }
                    }
                }
            }
        }
        
    // this is NOT "else if", because when the tickoff == 4, we want to be BOTH doing the RECORDER_RECORDING
    // code AND doing the RECORDER_TICKING_OFF code so we can record notes immediately before the start in order
    // to catch someone who's pressing the key just a little bit too soon.  
    if ((local.recorder.status == RECORDER_TICKING_OFF))
        {
        doClick();
        if (beat) 
            {
            local.recorder.tickoff++;
                
            if (local.recorder.tickoff == 3)        // prepare, allow one tick for early notes (see RECORDER_RECORDING)
                {
                memset(local.recorder.notes, NO_NOTE, MAX_RECORDER_NOTES_PLAYING);
                resetRecorder();
                data.slot.data.recorder.length = 0;
                local.recorder.numNotes = 0;
                }
                        
            if (local.recorder.tickoff == 5)
                {
                local.recorder.tickoff = 0;
                local.recorder.status = RECORDER_RECORDING;
                }
            }
        }

    if (ended)
        {
        resetRecorder();
        sendAllSoundsOff();
        if (local.recorder.playScheduled)
            {
            local.recorder.status = RECORDER_PLAYING;
            local.recorder.playScheduled = false;
            }
        else if (ended == ENDED)
            {
            local.recorder.status = RECORDER_STOPPED;
            }
        else 
            {
            local.recorder.status = RECORDER_PLAYING;
            }
        }

    if (updateDisplay)
        {
        clearScreen();
        
        // draw the recorder
        // this is the slow way to do it.  Too slow?
        recorderDrawPoint(local.recorder.numNotes, 0);
        recorderDrawPoint(local.recorder.currentPos, 0);
        recorderDrawPoint(div3(local.recorder.tick >> 5), 5);                 // 96 = 24 pulses per quarter note * 4 quarter notes per measure
        setPoint(led2, 6, 1);  // boundary
          
        if (local.recorder.status == RECORDER_TICKING_OFF)
            {
            if (local.recorder.tickoff > 0)
                for(uint8_t i = local.recorder.tickoff; i < 5; i++)
                    setPoint(led2, i - 1, 0);
            }
        else
            {
            // Positions 0..3 indicate status values
            setPoint(led2, local.recorder.status, 0);
            }
        }
    }
       

void stateRecorderMenuPerformanceRepeat()  
    {
    // This is forever, 1 time, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 times 
    const char* menuItems[16] = {  PSTR("LOOP"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("8"), PSTR("9"), PSTR("12"), PSTR("16"), PSTR("18"), PSTR("24"), PSTR("32"), PSTR("64"), PSTR("128") };
    if (entry) 
        {
        defaultMenuValue = data.slot.data.recorder.repeat & 0x0F;
        }
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
                
    playRecorder();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.data.recorder.repeat = ((data.slot.data.recorder.repeat & 0xF0) | (currentDisplay & 0x0F));
            resetRecorder();
            goUpState(immediateReturnState);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturnState);
            }
        break;
        }
    }

#define CHANNEL_DEFAULT_MIDI_OUT (0)

void stateRecorderMenuPerformanceKeyboard()
    {
    uint8_t result = doNumericalDisplay(CHANNEL_DEFAULT_MIDI_OUT, 16, options.recorderPlayAlongChannel, true, GLYPH_NONE);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            options.recorderPlayAlongChannel = currentDisplay;
            saveOptions();
            sendAllSoundsOff();
            isUpdated(SELECT_BUTTON, RELEASED);
            goUpState(immediateReturnState);
            }
        break;
        case MENU_CANCELLED:
            {
            // get rid of any residual select button calls, so we don't stop when exiting here
            isUpdated(SELECT_BUTTON, RELEASED);
            goUpState(immediateReturnState);
            }
        break;
        }
    }

void stateRecorderMenuPerformanceNext()
    {
    // The values are OFF, 0, 1, ..., 8
    // These correspond with stored values (in the high 4 bits of repeat) of 0...9
    uint8_t result = doNumericalDisplay(-1, 8, ((int16_t)(data.slot.data.recorder.repeat >> 4)) - 1, true, GLYPH_NONE);
    playRecorder();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.data.recorder.repeat = ((data.slot.data.recorder.repeat & 0x0F) | ((currentDisplay + 1) << 4));
            goUpState(immediateReturnState);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturnState);
            }
        break;
        }
    }
 

#define RECORDER_MENU_PERFORMANCE_KEYBOARD 0
#define RECORDER_MENU_PERFORMANCE_REPEAT 1
#define RECORDER_MENU_PERFORMANCE_NEXT 2
#define RECORDER_MENU_SAVE 3
#define RECORDER_MENU_OPTIONS 4

// Gives other options
void stateRecorderMenu()
    {
    uint8_t result;
    if (entry)
        {
        resetRecorder();
        sendAllSoundsOff();
        local.recorder.status = RECORDER_STOPPED;
        }
                
    const char* menuItems[5] = { PSTR("KEYBOARD"), PSTR("REPEAT RECORDING"), PSTR("NEXT RECORDING"), PSTR("SAVE"), options_p };
    result = doMenuDisplay(menuItems, 5, STATE_NONE, STATE_NONE, 1);

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
                case RECORDER_MENU_PERFORMANCE_KEYBOARD:
                    {
                    immediateReturnState = STATE_RECORDER_MENU;
                    goDownState(STATE_RECORDER_MENU_PERFORMANCE_KEYBOARD);
                    }
                break;
                case RECORDER_MENU_PERFORMANCE_REPEAT:
                    {
                    immediateReturnState = STATE_RECORDER_MENU;
                    goDownState(STATE_RECORDER_MENU_PERFORMANCE_REPEAT);
                    }
                break;
                case RECORDER_MENU_PERFORMANCE_NEXT:
                    {
                    immediateReturnState = STATE_RECORDER_MENU;
                    goDownState(STATE_RECORDER_MENU_PERFORMANCE_NEXT);
                    }
                break;
                case RECORDER_MENU_SAVE:
                    {
                    immediateReturnState = STATE_RECORDER_MENU;
                    goDownState(STATE_RECORDER_SAVE);
                    }
                break;
                case RECORDER_MENU_OPTIONS:
                    {
                    immediateReturnState = STATE_RECORDER_MENU;
                    goDownState(STATE_OPTIONS);
                    }
                break;
                }
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_RECORDER_PLAY);
            }
        break;
        }
    }


/*
  void stateRecorderNoteSpeed()
  {
  const uint8_t[16] glyphs = { GLYPH_8x5_EIGHTH_TRIPLET + FONT_8x5, 
  GLYPH_8x5_QUARTER_TRIPLET + FONT_8x5, 
  GLYPH_8x5_THIRTY_SECOND + FONT_8x5, 
  GLYPH_8x5_HALF_TRIPLET + FONT_8x5, 
  GLYPH_8x5_SIXTEENTH + FONT_8x5, 
  GLYPH_8x5_TRIPLET + FONT_8x5, 
  GLYPH_8x5_EIGHTH + FONT_8x5, 
  GLYPH_8x5_QUARTER_NOTE_TRIPLET + FONT_8x5, 
  GLYPH_8x5_DOTTED_HALF + FONT_8x5, 
  GLYPH_8x5_QUARTER + FONT_8x5, 
  GLYPH_8x5_DOTTED_QUARTER + FONT_8x5, 
  GLYPH_8x5_HALF + FONT_8x5, 
  GLYPH_8x5_DOTTED_HALF + FONT_8x5, 
  GLYPH_8x5_WHOLE + FONT_8x5, 
  GLYPH_8x5_DOTTED_WHOLE + FONT_8x5, 
  GLYPH_8x5_DOUBLE_WHOLE + FONT_8x5
  };
  result = doGlyphDisplay(glyphs, 16, NO_GLYPH, 0)
  }
*/

#endif
