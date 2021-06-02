////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#ifdef INCLUDE_THRU

void resetDistributionNotes() 
    { 
    memset(local.thru.distributionNotes, NO_NOTE, NUM_MIDI_CHANNELS);
    local.thru.currentDistributionChannelIndex = 0; 
    }


void performThruNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    // NOTE DISTRIBUTION OVER MULTIPLE CHANNELS
    if (options.thruNumDistributionChannels > 0)
        {
        // turn off ALL instances of this note on ALL channels
        for(uint8_t i = 0; i <= options.thruNumDistributionChannels; i++)
            {
            if (local.thru.distributionNotes[i] == note)
                {
                uint8_t newchannel = (options.channelOut + i - 1) % NUM_MIDI_CHANNELS + 1;
                for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
                    {
                    sendNoteOff(note, velocity, newchannel);
                    }
                local.thru.distributionNotes[i] = NO_NOTE;
                }
            }
        }
    else
        {
        // NOTE REPLICATION
        for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
            {
            sendNoteOff(note, velocity, channel);
            }
        }
                
    // CHORD MEMORY
    for(uint8_t i = 1; i < options.thruChordMemorySize; i++)  // Yes, I see the *1*.  We aren't playing the bottom note a second time
        {
        uint8_t chordNote = options.thruChordMemory[i] - options.thruChordMemory[0] + note;  // can't overflow, it'll only go to 254 (127 + 127).
        if (chordNote <= 127)
            {
            sendNoteOff(chordNote, velocity, channel);
            }
        }
    }

void performThruNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    // NOTE DISTRIBUTION OVER MULTIPLE CHANNELS
    if (options.thruNumDistributionChannels > 0)
        {
        // revise the channel
        channel = (options.channelOut + local.thru.currentDistributionChannelIndex - 1) % NUM_MIDI_CHANNELS + 1;
                                                        
        // do I need to turn off a note?
        if (local.thru.distributionNotes[local.thru.currentDistributionChannelIndex] != NO_NOTE)
            {
            for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
                {
                sendNoteOff(local.thru.distributionNotes[local.thru.currentDistributionChannelIndex], 127, channel);
                }
            }
        
        // store the note and update
        local.thru.distributionNotes[local.thru.currentDistributionChannelIndex] = note;
        local.thru.currentDistributionChannelIndex++;
        if (local.thru.currentDistributionChannelIndex > options.thruNumDistributionChannels )  // yes, it's > not >= because options.thruNumDistributionChannels starts at *1*
            local.thru.currentDistributionChannelIndex = 0;
        }

    // NOTE REPLICATION
    for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
        {
        sendNoteOn(note, itemValue, channel);
        }
        
    // CHORD MEMORY
    for(uint8_t i = 1; i < options.thruChordMemorySize; i++)  // Yes, I see the *1*.  We aren't playing the bottom note a second time
        {
        uint8_t chordNote = options.thruChordMemory[i] - options.thruChordMemory[0] + note;  // can't overflow, it'll only go to 254 (127 + 127).
        if (chordNote <= 127)
            {
            sendNoteOn(chordNote, itemValue, channel);
            }
        }
    }
        
void performThruPolyAftertouch(uint8_t note, uint8_t velocity, uint8_t channel)
    {
    // We note here that although a NOTE ON can be filtered out, 
    // its corresponding NOTE OFF will not, nor will any POLYPHONIC AFTERTOUCH.
    // I had considered storing the note on that was being filtered so
    // as to also filter out the later note off, but this has downsides,
    // namely if you have a bounce that gets filtered, but the note off hasn't
    // happened yet, then you get ANOTHER BOUNCE, it can't get filtered :-(

    // NOTE DISTRIBUTION OVER MULTIPLE CHANNELS
    if (options.thruNumDistributionChannels > 0)
        {
        // change ALL instances of this note on ALL channels
        for(uint8_t i = 0; i <= options.thruNumDistributionChannels; i++)
            {
            if (local.thru.distributionNotes[i] == note)
                {
                channel = (options.channelOut + i - 1) % NUM_MIDI_CHANNELS + 1;
                // We do NOT send extra notes with poly aftertouch because it consumes
                // so much buffer space that we will block on output and then start missing
                // incoming messages 
                //for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
                    {
                    sendPolyPressure(note, itemValue, channel);
                    }
                }
            }
        }
    else
        {
        // NOTE REPLICATION
        // We do NOT send extra notes with poly aftertouch because it consumes
        // so much buffer space that we will block on output and then start missing
        // incoming messages 
        //for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
            {
            sendPolyPressure(note, itemValue, channel);
            }
        }

    // CHORD MEMORY
    // We do NOT send extra notes with poly aftertouch because it consumes
    // so much buffer space that we will block on output and then start missing
    // incoming messages 
    /*
      for(uint8_t i = 1; i < options.thruChordMemorySize; i++)  // Yes, I see the *1*.  We aren't playing the bottom note a second time
      {
      uint8_t chordNote = options.thruChordMemory[i] - options.thruChordMemory[0] + note;  // can't overflow, it'll only go to 254 (127 + 127).
      if (chordNote <= 127)
      sendPolyPressure(chordNote, itemValue, channel);
      }
    */
    }


void playThru()
    {
    // here we check if it's time to submit a NOTE OFF
    if (local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED && 
        //local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000 <= currentTime)
        TIME_GREATER_THAN_OR_EQUAL(currentTime, local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000))
        {
        performThruNoteOff(local.thru.debounceNote, 127, options.channelOut);
        local.thru.debounceState = DEBOUNCE_STATE_OFF;
        }
        
    if (!bypass && newItem && options.channelOut != CHANNEL_OFF && 
        (itemChannel == options.channelIn || options.channelIn == CHANNEL_OMNI || itemChannel == options.thruMergeChannelIn))
        {
        uint8_t channel = options.channelOut;
                
        if (itemType == MIDI_NOTE_ON)
            {
            if (options.thruDebounceMilliseconds == 0)          // if debounce is off, don't do any debounce stuff!
                {
                performThruNoteOn(itemNumber, itemValue, channel);
                }
            else if (itemNumber != local.thru.debounceNote ||
                local.thru.debounceState == DEBOUNCE_STATE_OFF ||
                    ((local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED) &&
                    //local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000 <= currentTime))
                    TIME_GREATER_THAN_OR_EQUAL(currentTime, local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000)))
                {
                if (itemNumber != local.thru.debounceNote && local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED)  // new note, kill the old one
                    {
                    performThruNoteOff(local.thru.debounceNote, itemValue, channel);
                    }
                        
                if (options.thruDebounceMilliseconds != 0)
                    {
                    // set up state machine
                    local.thru.debounceState = DEBOUNCE_STATE_FIRST_NOTE_DOWN;
                    local.thru.debounceTime = currentTime;
                    local.thru.debounceNote = itemNumber;
                    }

                performThruNoteOn(itemNumber, itemValue, channel);
                }
            else
                {
                // Filter out, but we're pressing again, so:
                local.thru.debounceState = DEBOUNCE_STATE_FIRST_NOTE_DOWN;
                }
            }
        else if (itemType == MIDI_NOTE_OFF)
            {
            // If the note is too short, and it's what we're holding down, hold off and wait
            if (options.thruDebounceMilliseconds > 0 &&
                local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_DOWN && 
                local.thru.debounceNote == itemNumber)
                {
                //if (local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000 >= currentTime)  
                if (TIME_GREATER_THAN_OR_EQUAL(local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000, currentTime))
                    {
                    local.thru.debounceState = DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED;
                    local.thru.debounceTime = currentTime;
                    }
                else
                    {
                    performThruNoteOff(itemNumber, itemValue, channel);
                    local.thru.debounceState = DEBOUNCE_STATE_OFF;
                    }
                }
            else            // we don't care about this one
                {
                performThruNoteOff(itemNumber, itemValue, channel);
                }
            }
        else if (itemType == MIDI_AFTERTOUCH_POLY)
            {
            performThruPolyAftertouch(itemNumber, itemValue, channel);
            }
        else if ((itemType == MIDI_AFTERTOUCH) ||
            (itemType >= MIDI_PROGRAM_CHANGE && itemType <= MIDI_RPN_DECREMENT))
            {
            // Yes, I realize this is largely a copy of Control.sendControllerCommand()
            // but I can't modify that function without going over the Uno's memory limit,
            // so I'm unable to use it because I can't pass a channel in.  Maybe later if
            // we jettison the Uno.
                        
            // this includes raw CC, note
                        
            /*
            // CC->NRPN Mapping.  We just retag CC as if it was NRPN here.
            if (options.thruCCToNRPN && (itemType == MIDI_CC_7_BIT || itemType == MIDI_CC_14_BIT))
            {
            itemType = MIDI_NRPN_14_BIT;
            }
            */
                        
            for(uint8_t i = 0; i <= options.thruNumDistributionChannels; i++)
                {
                channel = (options.channelOut + i - 1) % NUM_MIDI_CHANNELS + 1;
                switch(itemType)
                    {
                    case MIDI_AFTERTOUCH:
                        {
                        MIDI.sendAfterTouch(itemValue, channel);
                        }
                    break;
                    case MIDI_PROGRAM_CHANGE:
                        {
                        MIDI.sendProgramChange(itemNumber, channel);
                        }
                    break;
                    case MIDI_PITCH_BEND:
                        {
                        MIDI.sendPitchBend((int)itemValue, channel);
                        }
                    break;
                    case MIDI_CC_7_BIT:
                        {
                        MIDI.sendControlChange(itemNumber, itemValue, channel);
                        }
                    break;
                    case MIDI_CC_14_BIT:
                        {
                        sendControllerCommand(CONTROL_TYPE_CC, itemNumber, itemValue, channel);
                        }
                    break;
                    case MIDI_NRPN_14_BIT:
                        {
                        sendControllerCommand(CONTROL_TYPE_NRPN, itemNumber, itemValue, channel);
                        }
                    break;
                    case MIDI_RPN_14_BIT:
                        {
                        sendControllerCommand(CONTROL_TYPE_RPN, itemNumber, itemValue, channel);
                        }
                    break;
                    case MIDI_NRPN_INCREMENT:
                        {
                        sendControllerCommand(CONTROL_TYPE_NRPN, itemNumber, CONTROL_VALUE_INCREMENT << 7, channel);
                        }
                    break;
                    case MIDI_RPN_INCREMENT:
                        {
                        sendControllerCommand(CONTROL_TYPE_RPN, itemNumber, CONTROL_VALUE_INCREMENT << 7, channel);
                        }
                    break;
                    case MIDI_NRPN_DECREMENT:
                        {
                        sendControllerCommand(CONTROL_TYPE_NRPN, itemNumber, CONTROL_VALUE_DECREMENT << 7, channel);
                        }
                    break;
                    case MIDI_RPN_DECREMENT:
                        {
                        sendControllerCommand(CONTROL_TYPE_RPN, itemNumber, CONTROL_VALUE_DECREMENT << 7, channel);
                        }
                    break;
                    }
                }
            TOGGLE_OUT_LED(); 
            }
        }
    }
    

void stateThruPlay()
    {
    if (entry)
        {
        sendAllSoundsOff();
        resetDistributionNotes();
        local.thru.debounceState = DEBOUNCE_STATE_OFF;
        entry = false;
        }
                        
    if (updateDisplay)
        {
        clearScreen();
        write3x5Glyphs(GLYPH_PLAY);
        }
        
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_THRU);
        sendAllSoundsOff();
        }
        
    else if (isUpdated(SELECT_BUTTON, PRESSED))
        {
        goDownState(STATE_OPTIONS);
        immediateReturnState = STATE_THRU_PLAY;
        }
        
    playThru();
    }
        
void stateThruChordMemory()
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
        
void stateThruBlockOtherChannels()
    {
    options.thruBlockOtherChannels = !options.thruBlockOtherChannels;
    saveOptions();
    goUpState(STATE_THRU);
    }      

#endif

