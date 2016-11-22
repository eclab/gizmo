#include "All.h"


void drawPoint(uint8_t item, uint8_t yoffset)
    {
    uint8_t y = 7 - yoffset - (item >> 4);  // integer div by 16
    uint8_t x = (item - (item >> 4) * 16);  // remainder

    if (x < 8)
        setPoint(led2, x, y);
    else
        setPoint(led, x-8, y);
    }


#define LOAD_NOTE_OFF 128
#define LOAD_NOTE_ON 0


void loadNote(uint8_t load, uint8_t id, uint16_t time)
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
        data.slot.data.recorder.notes++;
        }
    else // LOAD_NOTE_OFF
        {
        data.slot.data.recorder.length += 2;
        sendNoteOff(local.recorder.notes[id], 127, options.channelOut);
        local.recorder.notes[id] = NO_NOTE;  // make available
        }
    }

void resetRecorder()
    {
    local.recorder.tick = -1;
    local.recorder.currentPos = 0;
    local.recorder.bufferPos = 0;
    }
        
#define NOT_ENDED 0
#define ENDED 1
#define ENDED_REPEATING 2


// Plays OR Records the song
void stateRecorderPlay()
    {
    uint8_t ended = NOT_ENDED;
       
    if (entry)
        {
        resetRecorder();
        local.recorder.status = RECORDER_STOPPED;
        local.recorder.tickoff = 0;
        if ((currentDisplay == -1) || (data.slot.type != slotTypeForApplication(STATE_RECORDER))) // initialize
            {
            data.slot.data.recorder.length = 0;
            data.slot.data.recorder.notes = 0;
            }
        entry = false;
        }
                
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        ended = ENDED;
        goUpState(STATE_RECORDER_SURE);
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
        }
    
    // The long middle button starts a record
    // If we're doing ANYTHING other than ticking off or recording, start ready-to-record
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        local.recorder.status = RECORDER_TICKING_OFF;
        local.recorder.tickoff = 0;
        data.slot.type = SLOT_TYPE_RECORDER;
        // send ALL NOTES OFF
        sendAllNotesOff();
        }
    
    // the select button stops everything and calls save
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        ended = ENDED;
        state = STATE_RECORDER_SAVE;
        entry = true;
        }
        
    // the long select button pops up the options menu
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        optionsReturnState = STATE_RECORDER_PLAY;  
        goDownState(STATE_RECORDER_MENU);
        }

    // Formats:
    //  NOTE ON:   [0], id (4 bits) time (11 bits), pitch (7 bits + 1 extra) velocity (7 bits + 1 extra)
    //  NOTE OFF:  [1], id (4 bits) time (11 bits)
    //  Note that  the largest legal time value is 2015 (the end of 21 4-beat measures)
                                                        
    else if (pulse && (local.recorder.status == RECORDER_PLAYING))
        {
        local.recorder.tick++;
                
        if ((local.recorder.bufferPos >= data.slot.data.recorder.length && local.recorder.tick % 96 == 0) ||  // out of notes and at a measure boundary, ugh, divide by 96
            (local.recorder.tick > MAXIMUM_RECORDER_TICK))  // out of time
            {
            ended = options.recorderRepeat + 1;                     // if recorderRepeat is false, this is ENDED.  Else it is ENDED_REPEAT
            }
        else
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
    else if ((local.recorder.status == RECORDER_RECORDING) || (local.recorder.tickoff == 4))
        {
        if (pulse && !local.recorder.tickoff)  // we're not in the preliminary period
            local.recorder.tick++;

        if  (local.recorder.tickoff != 4) // don't click when in tickoff, you're already clicking!
            doClick();
                
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
                    (2 * (targetNextPulseTime - currentTime) <= microsecsPerPulse))  // if we're closer to the NEXT pulse than we are to the CURRENT one
                    {
                    // round up to next
                    time++;
                    }

                uint8_t id = MAX_RECORDER_NOTES_PLAYING + 1;    // indicates an invalid or unknown ID
                                                                        
                if ((itemType == MIDI_NOTE_ON) && 
                    (data.slot.data.recorder.notes < MAX_RECORDER_NOTES) && 
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
                        loadNote(LOAD_NOTE_OFF, 0, time);
                        id = 0;
                        }
                                                                                                                                                                        
                    // load the slot
                    local.recorder.notes[id] = itemNumber;  // the note proper
                                                                                        
                    // load a NOTE_ON at the id
                    loadNote(LOAD_NOTE_ON, id, time);

                    // load the note pitch
                    data.slot.data.recorder.buffer[data.slot.data.recorder.length+2 - 4] = itemNumber;  // the note.  We subtract 4 because loadNote has already added it
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
                        loadNote(LOAD_NOTE_OFF, id, time);
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
                data.slot.data.recorder.notes = 0;
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
        sendAllNotesOff();
        if (ended == ENDED)
            local.recorder.status = RECORDER_STOPPED;
        else 
            local.recorder.status = RECORDER_PLAYING;
        }

    if (updateDisplay)
        {
        clearScreen();
        
        // draw the recorder
        // this is the slow way to do it.  Too slow?
        drawPoint(data.slot.data.recorder.notes, 0);
        drawPoint(local.recorder.currentPos, 0);
        drawPoint(local.recorder.tick / 96, 5);                 // 96 = 24 pulses per quarter note * 4 quarter notes per measure
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
                
        if (options.recorderRepeat)
            setPoint(led2, 6, 0);
        }
    }
        
        
        
        
        
        
#define RECORDER_MENU_REPEAT 0
#define RECORDER_MENU_OPTIONS 1

// Gives other options
void stateRecorderMenu()
    {
    uint8_t result;
    if (entry)
        {
        resetRecorder();
        sendAllNotesOff();
        local.recorder.status = RECORDER_STOPPED;
        }
                
    const char* menuItems[2] = {    
        (options.recorderRepeat ? PSTR("NO REPEAT") : PSTR("REPEAT")),
        options_p 
        };
    result = doMenuDisplay(menuItems, 2, STATE_NONE, STATE_NONE, 1);

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
                case RECORDER_MENU_REPEAT:
                    {
                    options.recorderRepeat = !options.recorderRepeat;
                    saveOptions();
                    }
                break;
                case RECORDER_MENU_OPTIONS:
                    {
                    optionsReturnState = STATE_RECORDER_MENU;
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
