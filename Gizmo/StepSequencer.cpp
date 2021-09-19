////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#ifdef INCLUDE_STEP_SEQUENCER

// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
GLOBAL uint8_t _trackLength[6] = { 16, 24, 32, 48, 64, 96 };
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
GLOBAL uint8_t _numTracks[6] = { 12, 8, 6, 4, 3, 2 };


// This function exists in order for us to use IS_MONO to route
// all requests about MIDI on tracks to track 0
uint8_t outMIDI(uint8_t track)
    {
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    if (IS_MONO() || IS_DUO()) return local.stepSequencer.outMIDI[0];
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
    return local.stepSequencer.outMIDI[track];
    }

void stateStepSequencerMidiChannelOut()
    {
    uint8_t track = local.stepSequencer.currentTrack;

// If we're mono, we need to force the track to be 0
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    if (IS_MONO() || IS_DUO())
        {
        track = 0;
        }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    if (entry)
        {
        clearNotesOnTracks(true);
        }
        
    // 17 represents DEFAULT channel
    uint8_t val = stateNumerical(0, 17, local.stepSequencer.outMIDI[track], local.stepSequencer.backup, false, true, GLYPH_DEFAULT, 
        immediateReturn ? immediateReturnState : STATE_STEP_SEQUENCER_MENU);
    if (val != NO_STATE_NUMERICAL_CHANGE)
        {
        sendAllSoundsOff();
        }
    playStepSequencer();
    }



void stateStepSequencerMenuEditMark()
    {
    local.stepSequencer.markTrack = local.stepSequencer.currentTrack;
    local.stepSequencer.markPosition = local.stepSequencer.currentEditPosition;
    if (local.stepSequencer.markPosition < 0)
        local.stepSequencer.markPosition = 0;
    goUpState(immediateReturnState);
    }



// Copies/Moves note data from the mark track to the current track
void stateStepSequencerMenuEditCopy(uint8_t splat, uint8_t move)
    {
    // verify that the two tracks are the same type
    if (local.stepSequencer.data[local.stepSequencer.markTrack] == local.stepSequencer.data[local.stepSequencer.currentTrack]) 
        {
        uint8_t len = GET_TRACK_LENGTH();
        uint8_t buf[MAXIMUM_TRACK_LENGTH * 2];
        // copy it to third location
        memcpy(buf, data.slot.data.stepSequencer.buffer + local.stepSequencer.markTrack * ((uint16_t)len) * 2, len * 2);
        
        if (move)
            {
            // clear mark track
            memset(data.slot.data.stepSequencer.buffer + local.stepSequencer.markTrack * ((uint16_t)len) * 2, 
                local.stepSequencer.data[local.stepSequencer.markTrack] == STEP_SEQUENCER_DATA_NOTE ? 0 : CONTROL_VALUE_EMPTY, 
                len * 2);
            }
        
        // now copy
        uint8_t p0 = local.stepSequencer.markPosition;
        int8_t p1 = local.stepSequencer.currentEditPosition;
        if (p1 < 0) p1 = 0;
        for(uint8_t i = 0; i < len; i++)
            {
            data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + p1) * 2] = buf[p0 * 2];
            data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + p1) * 2 + 1] = buf[p0 * 2 + 1];
            p0++;
            if (p0 >= len ) { if (splat) break; else p0 = 0; }
            p1++;
            if (p1 >= len ) { if (splat) break; else p1 = 0; }
            }
                        
        // If we were splatting, and the data is note data, we need to eliminate invalid ties.  This isn't needed for
        // moving or copying, because the whole track is moved even if it's rotated.
        // Can't use removeSuccessiveTies() here.  Maybe we could merge those functions in the future.
        if (splat && local.stepSequencer.data[local.stepSequencer.markTrack] == STEP_SEQUENCER_DATA_NOTE)
            {
            for(uint8_t j = 0; j < 2; j++) // do this twice to make sure we get everything.  I think I just need to check one more note...
                {
                for(uint8_t i = 0; i < len; i++)
                    {
                    if (
                        // I am a tie
                        data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + i) * 2] == 0 &&
                        data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + i) * 2 + 1] == 1 &&
                        // ...and the note before me is a rest (if I'm at pos 0, then the note before me is at pos (len - 1) )
                            (i == 0 ? 
                                (data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + (len - 1)) * 2] == 0 &&
                                data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + (len - 1)) * 2 + 1] == 0) :
                                (data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + (i - 1)) * 2] == 0 &&
                                data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + (i - 1)) * 2 + 1] == 0)))
                        {
                        // then set me to a rest
                        data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + i) * 2] = 0;
                        data.slot.data.stepSequencer.buffer[(local.stepSequencer.currentTrack * (uint16_t)len + i) * 2 + 1] = 0;
                        }
                    }
                }
            }
                        
        // we gotta do this because we just deleted some notes :-(
        sendAllSoundsOff();
                        
        goUpState(immediateReturnState);
        }
    else
        {
        goDownState(STATE_STEP_SEQUENCER_MENU_CANT);  // failed
        }
    }



void stateStepSequencerMenuEditDuplicate()
    {
    local.stepSequencer.data[local.stepSequencer.currentTrack] = local.stepSequencer.data[local.stepSequencer.markTrack];
    local.stepSequencer.noteLength[local.stepSequencer.currentTrack] = local.stepSequencer.noteLength[local.stepSequencer.markTrack];
    local.stepSequencer.muted[local.stepSequencer.currentTrack] = local.stepSequencer.muted[local.stepSequencer.markTrack];
    local.stepSequencer.velocity[local.stepSequencer.currentTrack] = local.stepSequencer.velocity[local.stepSequencer.markTrack];
    local.stepSequencer.fader[local.stepSequencer.currentTrack] = local.stepSequencer.fader[local.stepSequencer.markTrack];
    local.stepSequencer.offTime[local.stepSequencer.currentTrack] = local.stepSequencer.offTime[local.stepSequencer.markTrack];
    local.stepSequencer.noteOff[local.stepSequencer.currentTrack] = local.stepSequencer.noteOff[local.stepSequencer.markTrack];
    local.stepSequencer.shouldPlay[local.stepSequencer.currentTrack] = local.stepSequencer.shouldPlay[local.stepSequencer.markTrack];
    local.stepSequencer.transposable[local.stepSequencer.currentTrack] = local.stepSequencer.transposable[local.stepSequencer.markTrack];
    local.stepSequencer.dontPlay[local.stepSequencer.currentTrack] = local.stepSequencer.dontPlay[local.stepSequencer.markTrack];
    local.stepSequencer.pattern[local.stepSequencer.currentTrack] = local.stepSequencer.pattern[local.stepSequencer.markTrack];

// Advanced step sequencer transfers control data
// Also if we're mono, we need to deny the ability to transfer MIDI data since all MIDI data is handled on track 0
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] = local.stepSequencer.controlParameter[local.stepSequencer.markTrack];
    local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = local.stepSequencer.lastControlValue[local.stepSequencer.markTrack];

    if (IS_MONO() || IS_DUO())
        {
        // don't ever copy midiOut(event) data.  For DUO and TRIO we'll have to avoid sending repeats (pattern) data too maybe?
        }
    else
        {
        local.stepSequencer.outMIDI[local.stepSequencer.currentTrack] = local.stepSequencer.outMIDI[local.stepSequencer.markTrack];
        }
#else
    local.stepSequencer.outMIDI[local.stepSequencer.currentTrack] = local.stepSequencer.outMIDI[local.stepSequencer.markTrack];
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                
    // set the mark and edit positions to 0 temporarily so we can copy the note data properly
    uint8_t backupMarkPosition = local.stepSequencer.markPosition;
    uint8_t backupEditPosition = local.stepSequencer.currentEditPosition;
    local.stepSequencer.markPosition = 0;
    local.stepSequencer.currentEditPosition = 0;
    stateStepSequencerMenuEditCopy(false, false);  // we copy rather than splat to save some time
        
    // reset the mark and edit positions
    local.stepSequencer.markPosition = backupMarkPosition;
    local.stepSequencer.currentEditPosition = backupEditPosition;
        
    goUpState(immediateReturnState);
    }



//// FIXME: Eventually we'll have to get rid of mute for MONO MODE?
void advanceMute(uint8_t track)
    {
    switch(local.stepSequencer.muted[track])
        {
        case STEP_SEQUENCER_NOT_MUTED:
            {
            local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTE_ON_SCHEDULED_ONCE;
            break;
            }
        case STEP_SEQUENCER_MUTED:
            {
            local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE;
            break;
            }
        case STEP_SEQUENCER_MUTE_ON_SCHEDULED:
            {
            local.stepSequencer.muted[track] = STEP_SEQUENCER_NOT_MUTED;
            break;
            }
        case STEP_SEQUENCER_MUTE_OFF_SCHEDULED:
            {
            local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTED;
            break;
            }
        case STEP_SEQUENCER_MUTE_ON_SCHEDULED_ONCE:
            {
            local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTE_ON_SCHEDULED;
            break;
            }
        case STEP_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE:
            {
            local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTE_OFF_SCHEDULED;
            break;
            }
        }
    }

//// FIXME: Eventually we'll have to get rid of solo for MONO MODE?
void advanceSolo()
    {
    switch(local.stepSequencer.solo)
        {
        case STEP_SEQUENCER_NO_SOLO:
            {
            local.stepSequencer.solo = STEP_SEQUENCER_SOLO_ON_SCHEDULED;
            break;
            }
        case STEP_SEQUENCER_SOLO:
            {
            local.stepSequencer.solo = STEP_SEQUENCER_SOLO_OFF_SCHEDULED;
            break;
            }
        case STEP_SEQUENCER_SOLO_ON_SCHEDULED:
            {
            local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;
            break;
            }
        case STEP_SEQUENCER_SOLO_OFF_SCHEDULED:
            {
            local.stepSequencer.solo = STEP_SEQUENCER_SOLO;
            break;
            }
        }
    }



/// This is the mono version which replaces patterns with repeats
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
void stateStepSequencerMenuPatternDoRepeats()    
    {
    const char* menuItems[16] = { PSTR("LOOP"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("7"), PSTR("8"), 
                                  PSTR("9"), PSTR("12"), PSTR("16"), PSTR("24"), PSTR("32"), PSTR("64"), PSTR("END") };
    if (entry)
        {
        defaultMenuValue = local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.currentTrack)];
        }
    
    // we don't allow the first track to be END
    uint8_t result = doMenuDisplay(menuItems, local.stepSequencer.currentTrack == 0 ? 15 : 16, STATE_NONE, 0, 1);
    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.currentTrack)] = currentDisplay;
            resetStepSequencerCountdown();
            goUpState(STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        }
    }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER



void stateStepSequencerMenuPatternDoPattern()    
    {
    const char* menuItems[16] =     { PSTR("OOOO"), PSTR("OOO-"), PSTR("---O"), PSTR("OO-O"), PSTR("--O-"), PSTR("O---"), PSTR("-O--"), PSTR("OO--"), PSTR("--OO"), PSTR("O-O-"), PSTR("-O-O"), 
                                      PSTR("R1/8"),                      PSTR("R1/4"),                      PSTR("R1/2"),                      PSTR("---X"),                      PSTR("XXXX") };
    const uint8_t menuIndices[16] = { P1111,        P1110,        P0001,        P1101,        P0010,        P1000,        P0100,        P1100,        P0011,        P1010,        P0101,                    
                                      STEP_SEQUENCER_PATTERN_RANDOM_1_8, STEP_SEQUENCER_PATTERN_RANDOM_1_4, STEP_SEQUENCER_PATTERN_RANDOM_1_2, STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL, STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE };
    if (entry)
        {
        // find the pattern
        for(uint8_t i = 0 ; i < 16; i++)
            {
            if (menuIndices[i] == local.stepSequencer.pattern[local.stepSequencer.currentTrack])
                {
                defaultMenuValue = i;
                break;
                }
            }
        setAutoReturnTime();
        }
        
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            local.stepSequencer.pattern[local.stepSequencer.currentTrack] = menuIndices[currentDisplay];
            goUpState(immediateReturnState);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_STEP_SEQUENCER_MENU);
            }
        break;
        }
    }

void stateStepSequencerMenuPattern()
    {
// If (IS_MONO() || IS_DUO()), we do repeats here rather than patterns
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    if (IS_MONO() || IS_DUO())
        {
        stateStepSequencerMenuPatternDoRepeats();
        }
    else
        {
        stateStepSequencerMenuPatternDoPattern();
        }
#else
    stateStepSequencerMenuPatternDoPattern();
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
    }
        

//// NOTE: The IMMEDIATE_RETURN feature removed from the next three functions
//// because I've found it VERY ANNOYING -- typically you need to set multiple
//// items, and so should go back into the menu.
void stateStepSequencerMenuPerformanceKeyboard()
    {
    uint8_t result = doNumericalDisplay(CHANNEL_ADD_TO_STEP_SEQUENCER, CHANNEL_TRANSPOSE, options.stepSequencerPlayAlongChannel, true, GLYPH_TRANSPOSE);
    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            options.stepSequencerPlayAlongChannel = currentDisplay;
            saveOptions();
            sendAllSoundsOff();
            // get rid of any residual select button calls, so we don't stop when exiting here
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
        
void stateStepSequencerMenuPerformanceRepeat()  
    {
    // This is forever, 1 time, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 times 
    const char* menuItems[16] = {  PSTR("LOOP"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("8"), PSTR("9"), PSTR("12"), PSTR("16"), PSTR("18"), PSTR("24"), PSTR("32"), PSTR("64"), PSTR("128") };
    if (entry) 
        {
        defaultMenuValue = data.slot.data.stepSequencer.repeat & 0x0F;
        }
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
                
    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.data.stepSequencer.repeat = ((data.slot.data.stepSequencer.repeat & 0xF0) | (currentDisplay & 0x0F));
            resetStepSequencerCountdown();
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

        
void stateStepSequencerMenuPerformanceNext()
    {
    // The values are OFF, 0, 1, ..., 8
    // These correspond with stored values (in the high 4 bits of repeat) of 0...9
    uint8_t result = doNumericalDisplay(-1, 8, ((int16_t)(data.slot.data.stepSequencer.repeat >> 4)) - 1, true, GLYPH_NONE);
    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.data.stepSequencer.repeat = ((data.slot.data.stepSequencer.repeat & 0x0F) | ((currentDisplay + 1) << 4));
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



void stateStepSequencerMenuPerformanceStop()
    {
    options.stepSequencerStop = !options.stepSequencerStop;
    saveOptions();
    goUpState(immediateReturnState);
    playStepSequencer();
    }



void stateStepSequencerMenuLength()
    {
    // The values are OFF, 1, ..., 31
    // or OFF, 33, ..., 63 for 64-step sequences
    // or OFF, 65, ..., 95 for 96-step sequences
    uint8_t result = doNumericalDisplay(
        GET_MINIMUM_CUSTOM_LENGTH() - 1,                // either 0 or 32 or 64, which will be displayed at "----"
        GET_TRACK_FULL_LENGTH() - 1,                    // 16, 24, 32, 48, 64, or 96
        GET_TRACK_CUSTOM_LENGTH() + GET_MINIMUM_CUSTOM_LENGTH() - 1,            // The current custom length (0...32) plus either 0 or 16 or 32 or 64
        true,                                                                   // display the 0 or 32 or 64 as "----"
        GLYPH_NONE);
    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            // Now we change the length
            data.slot.data.stepSequencer.format = GET_TRACK_FORMAT() | ((currentDisplay - (GET_MINIMUM_CUSTOM_LENGTH() - 1)) << 3);
            // Now we reset it again to put the cursor in the right place
            resetStepSequencer();
            goUpState(immediateReturn ? immediateReturnState : STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        }
    }


void resetStepSequencerCountdown()
    {
    // This is forever, 1 time, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 times 
    uint8_t val = data.slot.data.stepSequencer.repeat & 15;
    switch (val)
        {
        case 0: local.stepSequencer.countdown = 255; break;
        case 1: local.stepSequencer.countdown = 1; break;
        case 2: local.stepSequencer.countdown = 2; break;
        case 3: local.stepSequencer.countdown = 3; break;
        case 4: local.stepSequencer.countdown = 4; break;
        case 5: local.stepSequencer.countdown = 5; break;
        case 6: local.stepSequencer.countdown = 6; break;
        case 7: local.stepSequencer.countdown = 8; break;
        case 8: local.stepSequencer.countdown = 9; break;
        case 9: local.stepSequencer.countdown = 12; break;
        case 10: local.stepSequencer.countdown = 16; break;
        case 11: local.stepSequencer.countdown = 18; break;
        case 12: local.stepSequencer.countdown = 24; break;
        case 13: local.stepSequencer.countdown = 32; break;
        case 14: local.stepSequencer.countdown = 64; break;
        case 15: local.stepSequencer.countdown = 128; break;
        }
    local.stepSequencer.countup = 255;
    }
    

// clears a note on a track no matter what
void clearNoteOnTrack(uint8_t track)
    {
    if (local.stepSequencer.noteOff[track] < NO_NOTE) 
        {
        uint8_t out = (outMIDI(track) == MIDI_OUT_DEFAULT ? options.channelOut : outMIDI(track));
        if (out != NO_MIDI_OUT)
            {
            sendNoteOff(local.stepSequencer.noteOff[track], 127, out);
            }
        local.stepSequencer.noteOff[track] = NO_NOTE;
        }
    }

// This is a slightly modified version of the code in stateLoad()... I'd like to merge them but it'll
// have an impact on the Uno.  So for NOW...
uint8_t loadSequence(uint8_t slot)
    {
    uint8_t num = GET_NUM_TRACKS();
    for(uint8_t i = 0; i < num; i++)
        clearNoteOnTrack(i);

    if (getSlotType(slot) != slotTypeForApplication(STATE_STEP_SEQUENCER))
        {
        return 0;
        }
    else
        {
        loadSlot(slot);

        // FIXME: did I fix the issue of synchronizing the beats with the sequencer notes?
        //local.stepSequencer.currentPlayPosition = 
        //      div12((24 - beatCountdown) * notePulseRate) >> 1;   // get in sync with beats

        uint8_t len = GET_TRACK_FULL_LENGTH();
        uint8_t num = GET_NUM_TRACKS();
                                
        // unpack the high-bit info
        for(uint8_t i = 0; i < num; i++)
            {
            uint16_t pos = i * ((uint16_t)len) * 2;

            //// 1 bit type of data
            if (gatherByte(pos) >> 7 == 0)  // It's a note
                {
                //// 1 bit mute
                //// 5 bits MIDI out channel (including "use default")
                //// 7 bits length
                //// 7 bits velocity (including "use per-note velocity")
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

#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
/// FIXME: controlParameter and lastControlValue are MISSING.  This is wrong?

// In MONO mode the play track is used to keep track of which track is playing during performance.  We set it to 0 here.
        local.stepSequencer.playTrack = 0;
        if (IS_MONO() || IS_DUO())
            local.stepSequencer.NEXT_PLAY_TRACK = NO_TRACK;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
        local.stepSequencer.scheduleStop = 0;
        local.stepSequencer.goNextSequence = 0;
        local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;
        local.stepSequencer.currentTrack = 0;
        local.stepSequencer.currentEditPosition = 0;
        local.stepSequencer.markTrack = 0;
        local.stepSequencer.markPosition = 0;
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
        local.stepSequencer.lastExclusiveTrack = NO_TRACK;
        resetStepSequencerCountdown();
        // this will set the countdown one higher than it should be so...
        if (local.stepSequencer.countdown != 255)
            local.stepSequencer.countdown--;
        return 1;
        }
    }
           




void resetTrack(uint8_t track)
    {
    uint8_t trackLen = GET_TRACK_FULL_LENGTH();
    memset(data.slot.data.stepSequencer.buffer + ((uint16_t)trackLen) * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
    local.stepSequencer.data[track] = STEP_SEQUENCER_DATA_NOTE;


/// Advanced: control value must be cleared as well
/// Also if IS_MONO we need to handle MIDI channel and repeats properly
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    local.stepSequencer.lastControlValue[track] = 0;
    
    if (IS_MONO() || IS_DUO())
        {
        // don't screw with the MIDI channel
        local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(track)] = STEP_SEQUENCER_MONO_REPEATS_ONCE;
        }
    else
        {
        local.stepSequencer.pattern[track] = STEP_SEQUENCER_PATTERN_ALL;
        local.stepSequencer.outMIDI[track] = MIDI_OUT_DEFAULT;  // default
        }
#else   
    local.stepSequencer.pattern[track] = STEP_SEQUENCER_PATTERN_ALL;
    local.stepSequencer.outMIDI[track] = (track == 0 ? MIDI_OUT_DEFAULT : track + 1);
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    local.stepSequencer.transposable[track] = 1;
    local.stepSequencer.noteLength[track] = PLAY_LENGTH_USE_DEFAULT;
    local.stepSequencer.muted[track] = STEP_SEQUENCER_NOT_MUTED;
    local.stepSequencer.velocity[track] = STEP_SEQUENCER_NO_OVERRIDE_VELOCITY;
    local.stepSequencer.fader[track] = FADER_IDENTITY_VALUE;
    local.stepSequencer.offTime[track] = 0;
    local.stepSequencer.noteOff[track] = NO_NOTE;
    }


// If the point holds a cursor, blinks it, else sets it.  Used to simplify
// and reduce code size
void blinkOrSetPoint(unsigned char* led, uint8_t x, uint8_t y, uint8_t isCursor)
    {
    if (isCursor)
        blinkPoint(led, x, y);
    else
        setPoint(led, x, y);
    }
  
uint8_t shouldMuteTrack(uint8_t track)
    {
    uint8_t solo = (local.stepSequencer.solo == STEP_SEQUENCER_SOLO || local.stepSequencer.solo == STEP_SEQUENCER_SOLO_OFF_SCHEDULED);
    uint8_t muted = (local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTED || local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTE_OFF_SCHEDULED || local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE);
    return 
        // We're muted if solo is on and we're not the current track
        (solo && track != local.stepSequencer.currentTrack) ||
        // We're muted if solo is NOT on and OUR mute is on
        (!solo && muted);
    }


void drawStepSequencerNotePitchAndVelocity(uint8_t trackLen)
	{
#ifdef TWO_SCREENS_VERTICAL
	uint16_t pos = ((uint16_t)trackLen) * local.stepSequencer.currentTrack;
	if (local.stepSequencer.currentEditPosition >= 0)
		{
		pos = pos + local.stepSequencer.currentEditPosition;
		}
	else
		{
		pos = pos + local.stepSequencer.currentPlayPosition;
		}
	uint8_t note = data.slot.data.stepSequencer.buffer[pos * 2];
	uint8_t velocity = data.slot.data.stepSequencer.buffer[pos * 2 + 1];
	uint8_t type = local.stepSequencer.data[local.stepSequencer.currentTrack];
	if (type == STEP_SEQUENCER_DATA_NOTE)
		{
		if (velocity == 0)
			{
			if (note > 0)	// it's a tie
				{
				write3x5Glyph(led4, GLYPH_3x5_T, 0);
				write3x5Glyph(led4, GLYPH_3x5_I, 4);
				write3x5Glyph(led3, GLYPH_3x5_E, 0);
				}
			else			// it's a rest
				{
				// do nothing
				}
			}
		else 
			{
			writeNotePitch(led4, note);
			writeShortNumber(led3, velocity, false);
			}
		}
	else if ((note << 7 | velocity) == CONTROL_VALUE_EMPTY)		// it's an empty data value
		{
		// write nothing
		}
	else if (type == STEP_SEQUENCER_DATA_CC)
		{
		writeShortNumber(led3, note - 1, false);
		write3x5Glyph(led4, GLYPH_3x5_C, 0);
		write3x5Glyph(led4, GLYPH_3x5_C, 4);
		}
	else if (type == STEP_SEQUENCER_DATA_14_BIT_CC || type == STEP_SEQUENCER_DATA_NRPN || type == STEP_SEQUENCER_DATA_RPN)
		{
	 	writeNumber(led3, led4, (note << 7) | velocity);
		}
	else if (type == STEP_SEQUENCER_DATA_PC)
		{
		writeShortNumber(led3, note - 1, false);
		write3x5Glyph(led4, GLYPH_3x5_P, 0);
		write3x5Glyph(led4, GLYPH_3x5_C, 4);
		}
	else if (type == STEP_SEQUENCER_DATA_BEND)
		{
	 	writeNumber(led3, led4, ((int16_t)((note << 7) | velocity)) - 8192);
		}
	else if (type == STEP_SEQUENCER_DATA_AFTERTOUCH)
		{
		writeShortNumber(led3, note - 1, false);
		write3x5Glyph(led4, GLYPH_3x5_A, 0);
		write3x5Glyph(led4, GLYPH_3x5_T, 4);
		}
	else	// uh...
		{
		// do nothing
		}
#else
	// do nothing
#endif TWO_SCREENS_VERTICAL
	}

// Draws the sequence with the given track length, number of tracks, and skip size
void drawStepSequencer(uint8_t trackLen, uint8_t fullLen, uint8_t numTracks)
    {
    // this little function correctly maps:
    // 8 -> 1
    // 12 -> 1
    // 16 -> 1
    // 24 -> 2
    // 32 -> 2
    // 48 -> 3
    // 64 -> 4    
    // 96 -> 6    
    uint8_t skip = ((fullLen + 15) >> 4);      // that is, trackLen / 16
        
    clearScreen();
    
    uint8_t firstTrack = local.stepSequencer.currentTrack;
    uint8_t lastTrack = numTracks;          // lastTrack is 1+ the final track we'll be drawing
    
#ifdef TWO_SCREENS_VERTICAL
	if (options.stepSequencerShowNote)
		{
		// draw the top screen
		drawStepSequencerNotePitchAndVelocity(trackLen);
		}
		
	if (!options.stepSequencerShowNote)
		{
	    firstTrack = 0;                         // there is always enough space to fit the entire sequence on screen
	    }
	else
#endif TWO_SCREENS_VERTICAL
    if (fullLen == 96)
        {
        // handle 96 specially since it takes up the whole screen
        lastTrack = firstTrack + 1;
        }
    else
        {
        // this code is designed to allow the user to move down to about the middle of the screen,
        // at which point the cursor stays there and the screen scrolls instead.
        uint8_t fourskip =  4 / skip;
        if (firstTrack < fourskip)  
            firstTrack = 0;
        else firstTrack = firstTrack - fourskip + 1;   
        uint8_t sixskip = 6 / skip;
        lastTrack = bound(lastTrack, 0, firstTrack + sixskip);
        if (lastTrack == numTracks)
            {
            if (lastTrack >= sixskip) 
                firstTrack = lastTrack - sixskip;
            }
        }

    // Now we start drawing each of the tracks.  We will make blinky lights for beats or for the cursor
    // and will have solid lights or nothing for the notes or their absence.
        
#ifdef TWO_SCREENS_VERTICAL
    uint8_t y = 15;
    if (options.stepSequencerShowNote)
    	{
    	y = 7;
    	}
#else
    uint8_t y = 7;              // we can go negative if we have two vertical screens
#endif TWO_SCREENS_VERTICAL
    for(uint8_t t = firstTrack; t < lastTrack; t++)  // for each track from top to bottom
        {
        // data is stored per-track as
        // NOTE VEL
        // We need to strip off the high bit because it's used for other packing
        // for each note in the track
        for (uint8_t d = 0; d < trackLen; d++)
            {
            uint8_t shouldDrawMuted = shouldMuteTrack(t);
            uint16_t pos = (t * (uint16_t) fullLen + d) * 2;
            uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
            // check for tie
            if ((vel == 0) && (data.slot.data.stepSequencer.buffer[pos] == 1)
                && (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NOTE))
                vel = 1;  // so we draw it
                
            else if ((local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE) &&
                ((vel & 127) | ((data.slot.data.stepSequencer.buffer[pos] & 127) << 7)) != 0)
                vel = 1;  // so we draw it

            if (shouldDrawMuted)
                vel = 0;
            uint8_t xpos = d - ((d >> 4) * 16);  // x position on screen
            uint8_t blink = (
                // draw play position cursor if we're not stopped and we're in edit cursor mode
                    ((local.stepSequencer.playState != PLAY_STATE_STOPPED) && (d == local.stepSequencer.currentPlayPosition)
                    && !local.stepSequencer.performanceMode) ||   // main cursor
                // draw play position cursor, plus the crosshatch, always if we're in play position mode
                    ((local.stepSequencer.currentEditPosition < 0 
                        || local.stepSequencer.performanceMode) && 
                    ((d == local.stepSequencer.currentPlayPosition) ||  ((t == local.stepSequencer.currentTrack) && (abs(d - local.stepSequencer.currentPlayPosition) == 2)))) ||  // crosshatch
                // draw edit cursor
                ((t == local.stepSequencer.currentTrack) && (d == local.stepSequencer.currentEditPosition) && !local.stepSequencer.performanceMode) ||
                
                // draw mute or solo indicator.  Solo overrides mute.
                // So draw if solo is on but we're not it, OR if solo is turned off and we're muted
                ((xpos == 0 || xpos == 15) && shouldDrawMuted)
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                || (local.stepSequencer.performanceMode && (IS_MONO() || IS_DUO()) && d == 0 && GET_PRIMARY_TRACK(t) == local.stepSequencer.playTrack)
                || ((!local.stepSequencer.performanceMode) && IS_DUO() && d == 0 && t == GET_PRIMARY_TRACK(local.stepSequencer.currentTrack) && t != local.stepSequencer.currentTrack)
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                );

            if (vel || blink)
                {       
                uint8_t pointx = d - ((d >> 3) * 8);            //  d - (d / 8) * 8
                uint8_t isled2 = (((d >> 3) & 0x1) == 0x0);             // (d / 8) is even
                uint8_t pointy = y - (d >> 4);                    // (y - (d / 16)
#ifdef TWO_SCREENS_VERTICAL
                if (pointy > 7 && !options.stepSequencerShowNote)
                    {
                    blinkOrSetPoint(isled2 ? led4 : led3, pointx, pointy - 8, blink);
                    }
                else
                    {
                    blinkOrSetPoint(isled2 ? led2 : led, pointx, pointy, blink);
                    }
#else
                blinkOrSetPoint(isled2 ? led2 : led, pointx, pointy, blink);
#endif TWO_SCREENS_VERTICAL
                }
            }
        y -= skip;
        }
        
    // Next draw the track number
    drawRange(led2, 0, 1, 12, local.stepSequencer.currentTrack);


    // If MONO we draw the play track rather than the MIDI channel
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    if (IS_MONO() || IS_DUO())
        {
        if (local.stepSequencer.performanceMode && local.stepSequencer.playTrack < GET_NUM_TRACKS())
            {
            drawRange(led2, 0, 0, 12, local.stepSequencer.playTrack);
            }
        }
    else
        {
        drawMIDIChannel(
            (outMIDI(local.stepSequencer.currentTrack) == MIDI_OUT_DEFAULT) ?
            options.channelOut : outMIDI(local.stepSequencer.currentTrack));
        }
        

#else
    drawMIDIChannel(
        (outMIDI(local.stepSequencer.currentTrack) == MIDI_OUT_DEFAULT) ?
        options.channelOut : outMIDI(local.stepSequencer.currentTrack));
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    // Are we in performance mode?
    if (local.stepSequencer.performanceMode)
        {
        blinkPoint(led, 2, 1);
        // are we going to the next sequence?
        if (local.stepSequencer.goNextSequence || local.stepSequencer.scheduleStop)
            setPoint(led, 3, 1);
        }       
    // is our track scheduled to play?
    if (local.stepSequencer.shouldPlay[local.stepSequencer.currentTrack])
        setPoint(led, 4, 1);
                
    // If MONO we want to blink if we're trying to edit (or jump to) an END track
    // Otherwise we use those two spots for the sequence iteration
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    if (IS_MONO() || IS_DUO())
        {
        if (local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.currentTrack)] == STEP_SEQUENCER_MONO_REPEATS_END)
            {
            blinkPoint(led, 0, 1);
            blinkPoint(led, 1, 1);
            }
        }
    else
        {
        drawRange(led, 0, 1, 4, local.stepSequencer.countup & 3);
        }
#else
    drawRange(led, 0, 1, 4, local.stepSequencer.countup & 3);
        
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    // Do we have a fader value != FADER_IDENDITY_VALUE ?
    if (local.stepSequencer.fader[local.stepSequencer.currentTrack] != FADER_IDENTITY_VALUE)
        setPoint(led, 5, 1);
    
    // Are we overriding velocity?
    if (local.stepSequencer.velocity[local.stepSequencer.currentTrack] != STEP_SEQUENCER_NO_OVERRIDE_VELOCITY)
        setPoint(led, 6, 1);

    // Are we stopped?
    if (local.stepSequencer.playState != PLAY_STATE_PLAYING)
        setPoint(led, 7, 1);
    }





// Reformats the sequence as requested by the user
void stateStepSequencerFormat()
    {
/// The advanced step sequencer has 6 additional MONO modes (for a total of 12) and possibly in the future some DUO or TRIO modes
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    const char* menuItems[16] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES"), PSTR("48 NOTES"), PSTR("64 NOTES"), PSTR("96 NOTES"), 
                                   PSTR("16 MONO"), PSTR("24 MONO"), PSTR("32 MONO"), PSTR("48 MONO"), PSTR("64 MONO"), PSTR("96 MONO"),
                                   PSTR("16 DUO"), PSTR("24 DUO"), PSTR("32 DUO"), PSTR("48 DUO") };               // No PSTR("96 DUO") because we want two tracks per screen
    // PSTR("16 TRIO"), PSTR("32 TRIO"), PSTR("64 TRIO") };
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
#else
    const char* menuItems[6] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES"), PSTR("48 NOTES"), PSTR("64 NOTES"), PSTR("96 NOTES") };
    uint8_t result = doMenuDisplay(menuItems, 6, STATE_NONE, 0, 1);
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.type = SLOT_TYPE_STEP_SEQUENCER;

/// The advanced step sequencer has 6 additional MONO modes (for a total of 12) and possibly in the future some DUO or TRIO modes
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            const uint8_t formats[16] = { STEP_SEQUENCER_FORMAT_16x12, STEP_SEQUENCER_FORMAT_24x8, STEP_SEQUENCER_FORMAT_32x6, STEP_SEQUENCER_FORMAT_48x4, STEP_SEQUENCER_FORMAT_64x3, STEP_SEQUENCER_FORMAT_96x2,
                                          STEP_SEQUENCER_FORMAT_16x12, STEP_SEQUENCER_FORMAT_24x8, STEP_SEQUENCER_FORMAT_32x6, STEP_SEQUENCER_FORMAT_48x4, STEP_SEQUENCER_FORMAT_64x3, STEP_SEQUENCER_FORMAT_96x2 };
            STEP_SEQUENCER_FORMAT_16x12, STEP_SEQUENCER_FORMAT_24x8, STEP_SEQUENCER_FORMAT_32x6, STEP_SEQUENCER_FORMAT_48x4, // STEP_SEQUENCER_FORMAT_96x2,
                // STEP_SEQUENCER_FORMAT_16x12, STEP_SEQUENCER_FORMAT_32x6, STEP_SEQUENCER_FORMAT_64x3 };
/// For now MONO mode sets mono to 1: maybe later we'll set to 2 or 3 for duo or trio mode                                                               
                data.slot.data.stepSequencer.mono = (currentDisplay < 6 ? 0 : (currentDisplay < 12 ? 1 : 2));               // (currentDisplay < 17 ? 2 : 3)));
#else
            const uint8_t formats[6] = { STEP_SEQUENCER_FORMAT_16x12, STEP_SEQUENCER_FORMAT_24x8, STEP_SEQUENCER_FORMAT_32x6, STEP_SEQUENCER_FORMAT_48x4, STEP_SEQUENCER_FORMAT_64x3, STEP_SEQUENCER_FORMAT_96x2 };
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

            data.slot.data.stepSequencer.format = formats[currentDisplay];
            setParseRawCC(false);
            memset(data.slot.data.stepSequencer.buffer, 0, STEP_SEQUENCER_BUFFER_SIZE);
            for(uint8_t i = 0; i < GET_NUM_TRACKS(); i++)
                {
                resetTrack(i);
                }

/// MONO mode only uses track 0.  resetTrack() above no longer clears midi in mono mode, so we need to reset it here
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            if (IS_MONO() || IS_DUO())
                {
                local.stepSequencer.outMIDI[0] = MIDI_OUT_DEFAULT;
                }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

            stopStepSequencer();
            // stopStepSequencer() just set the countdown to something insane.  Set it to infinity
            local.stepSequencer.countdown = 255;
            local.stepSequencer.countup = 255;
            data.slot.data.stepSequencer.repeat = 0;  // forever
            local.stepSequencer.transpose = 0;
            local.stepSequencer.performanceMode = 0;
            local.stepSequencer.goNextSequence = 0;
            
/// MONO mode would reset the play track (unused otherwise)
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            local.stepSequencer.playTrack = 0;
            if (IS_MONO() || IS_DUO())
                local.stepSequencer.NEXT_PLAY_TRACK = NO_TRACK;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

            local.stepSequencer.scheduleStop = 0;
            local.stepSequencer.solo = 0;
            local.stepSequencer.currentTrack = 0;
            local.stepSequencer.currentEditPosition = 0;
            local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
            local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
            goDownState(STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        case MENU_CANCELLED:
            {
            goDownState(STATE_STEP_SEQUENCER);
            }
        break;
        }
    }


// Deletes any stray ties AFTER position p, replacing them with rests
void removeSuccessiveTies(uint8_t p, uint8_t trackLen)
    {
    if (local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE)
        return;

    p = incrementAndWrap(p, trackLen);
    uint16_t v = (((uint16_t)trackLen) * local.stepSequencer.currentTrack + p) * 2 ;
    uint8_t removed = false;
    while((data.slot.data.stepSequencer.buffer[v + 1]== 0) &&
        data.slot.data.stepSequencer.buffer[v] == 1)
        {
        removed = true;
        data.slot.data.stepSequencer.buffer[v] = 0;  // make it a rest
        p = incrementAndWrap(p, trackLen);
        v = (((uint16_t)trackLen) * local.stepSequencer.currentTrack + p) * 2 ;
        }
      
    if (removed)
        {                  
        // we gotta do this because we just deleted some notes :-(
        uint8_t out = (outMIDI(local.stepSequencer.currentTrack) == MIDI_OUT_DEFAULT ? 
            options.channelOut : outMIDI(local.stepSequencer.currentTrack));
        if (out != NO_MIDI_OUT)
            {
            sendAllSoundsOff(out);
            }
        }
    }
                                                

// Sends either a Note ON (if note is 0...127) or Note OFF (if note is 128...255)
// with the given velocity and the given track.  Computes the proper MIDI channel.
void sendTrackNote(uint8_t note, uint8_t velocity, uint8_t track)
    {
    uint8_t out = outMIDI(track);
    if (out == MIDI_OUT_DEFAULT) 
        out = options.channelOut;
    if (out != NO_MIDI_OUT)
        {
        if (note < 128) sendNoteOn(note, velocity, out);
        else sendNoteOff(note - 128, velocity, out);
        }
    }

void loadBuffer(uint16_t position, uint8_t note, uint8_t velocity)
    {
    data.slot.data.stepSequencer.buffer[position * 2] = note;
    data.slot.data.stepSequencer.buffer[position * 2 + 1] = velocity;
    }



/// Adds ties from local.stepSequencer.lastNotePos + 1 to just before pos, wrapping around,
/// unless local.stepSequencer.lastNotePos was NO_SEQUENCER_POS or was pos itself (a note played and immediately released)
void tieNote(uint8_t pos)
    {
    uint8_t trackLen = GET_TRACK_LENGTH();
    if (local.stepSequencer.lastNotePos != NO_SEQUENCER_POS && 
        local.stepSequencer.lastNotePos != pos)
        {
        uint8_t p = incrementAndWrap(local.stepSequencer.lastNotePos, trackLen);
        while(p != pos)
            {
            loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + p, 1, 0);
            p = incrementAndWrap(p, trackLen); 
            }
        }
    local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
    local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
    }




// I'd prefer the following code as it creates a bit of a buffer so scrolling to position 0 doesn't go straight
// into play position mode.  Or perhaps we should change things so that you scroll to the far RIGHT edge, dunno.
// But anyway we can't do this because adding just a little bit here radically increases our memory footprint. :-(
#define CURSOR_LEFT_MARGIN (4)

int16_t getNewCursorXPos(uint8_t trackLen)
    {
    int16_t val = ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + CURSOR_LEFT_MARGIN)) >> 9)) - CURSOR_LEFT_MARGIN;
    if ((val < 0) && (val > -CURSOR_LEFT_MARGIN))
        val = 0;
    return val;
    }


void resetStepSequencer()
    {
    local.stepSequencer.currentPlayPosition = GET_TRACK_LENGTH() - 1;
    resetStepSequencerCountdown();
    }
        
void stopStepSequencer()
    {
    stopClock(true);
    resetStepSequencer();
    local.stepSequencer.playState = PLAY_STATE_STOPPED;
    local.stepSequencer.scheduleStop = false;
    local.stepSequencer.lastExclusiveTrack = NO_TRACK;
    
/// MONO mode would reset the play track (unused otherwise)
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    local.stepSequencer.playTrack = 0;
    if (IS_MONO() || IS_DUO())
        local.stepSequencer.NEXT_PLAY_TRACK = NO_TRACK;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    sendAllSoundsOff();
    }

void addTie(uint8_t trackLen)
    {
    // We only permit ties if (1) the note before was NOT a rest and
    // (2) the note AFTER is NOT another tie (to prevent us from making a full line of ties)
    // These two positions (before and after) are p and p2 
    uint8_t p = local.stepSequencer.currentEditPosition - 1;
    uint8_t p2 = p + 2;
    if (p == 255) p = trackLen - 1;             // we wrapped around from 0
    if (p2 >= trackLen) p2 = 0;                                 // we wrapped around from tracklen - 1

    uint16_t v = (((uint16_t)trackLen) * local.stepSequencer.currentTrack + p) * 2 ;            // these values can easily go beyond uint8_t
    uint16_t v2 = (((uint16_t)trackLen) * local.stepSequencer.currentTrack + p2) * 2 ;
    if (local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE)
        {
        // erase data
        uint8_t msb = 0;
        uint8_t lsb = 0;
        loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, msb, lsb);
        local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
        local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
        }
    // don't add if a rest precedes it or a tie is after it
    else if (((data.slot.data.stepSequencer.buffer[v + 1] == 0) &&           // rest before
            (data.slot.data.stepSequencer.buffer[v] == 0)) ||
            ((data.slot.data.stepSequencer.buffer[v2 + 1] == 1) &&          // tie after
            (data.slot.data.stepSequencer.buffer[v2] == 0)))
        {
        // do nothing
        }
    else
        {
        loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 1, 0);
        local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
        local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
        }
    }

// Plays and records the sequence
void stateStepSequencerPlay()
    {
    // first we:
    // compute TRACKLEN, the length of the track
    // compute SKIP, the number of lines on the screen the track takes up
    uint8_t trackLen = GET_TRACK_LENGTH();
    uint8_t numTracks = GET_NUM_TRACKS();
    
    if (entry)
        {
        entry = false;
        local.stepSequencer.currentRightPot = -1;
        local.stepSequencer.pots[LEFT_POT] = pot[LEFT_POT];
        local.stepSequencer.pots[RIGHT_POT] = pot[RIGHT_POT];
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        local.stepSequencer.lastCurrentTrack = NO_TRACK;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
        }

    immediateReturn = false;

    // always do this
    leftPotParameterEquivalent = false;

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        if (local.stepSequencer.performanceMode)
            {
            //// EXIT PERFORMANCE MODE
            local.stepSequencer.performanceMode = false;
            setParseRawCC(local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC);
            }
        else
            {
            //// EXIT SEQUENCER
            setParseRawCC(false);
            goUpState(STATE_STEP_SEQUENCER_EXIT);
            }
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        if (local.stepSequencer.performanceMode)
            {
/// We handle non-note data here
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            if (IS_MONO())
                {
                if (local.stepSequencer.playTrack + 1 < numTracks)
                    {
                    if (local.stepSequencer.NEXT_PLAY_TRACK == NO_TRACK)
                        {
                        local.stepSequencer.NEXT_PLAY_TRACK = local.stepSequencer.playTrack + 1;
                        }
                    else
                        {
                        local.stepSequencer.NEXT_PLAY_TRACK = NO_TRACK;
                        }
                    }
                }
            else if (IS_DUO())
                {
                if (GET_PRIMARY_TRACK(local.stepSequencer.playTrack + 2) < numTracks)
                    {
                    if (local.stepSequencer.NEXT_PLAY_TRACK == NO_TRACK)
                        {
                        local.stepSequencer.NEXT_PLAY_TRACK = GET_PRIMARY_TRACK(local.stepSequencer.playTrack + 2);
                        }
                    else
                        {
                        local.stepSequencer.NEXT_PLAY_TRACK = NO_TRACK;
                        }
                    }
                }
            else
                {
                advanceMute(local.stepSequencer.currentTrack);
                }
#else
            //// SCHEDULE MUTE
            advanceMute(local.stepSequencer.currentTrack);
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
            }
        else if (local.stepSequencer.currentEditPosition >= 0)
            {
            //// ADD REST OR DATA

/// We handle non-note data here
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            if (local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE)
                {
                // enter data
                uint8_t msb = (uint8_t) (local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] >> 7);
                uint8_t lsb = (uint8_t) (local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] & 127);
                loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, msb, lsb);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            else
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                {
                local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
                local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
                // add a rest
                loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 0, 0);
                removeSuccessiveTies(local.stepSequencer.currentEditPosition, trackLen);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            }
        else
            {
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            if (IS_MONO() || IS_DUO())
                {
                /// Advance to next track
                uint8_t oldTrack = local.stepSequencer.currentTrack;
                local.stepSequencer.currentTrack++;
                if (local.stepSequencer.currentTrack >= numTracks)
                    local.stepSequencer.currentTrack = 0;
                if (GET_PRIMARY_TRACK(oldTrack) != GET_PRIMARY_TRACK(local.stepSequencer.currentTrack))
                    {
                    clearNotesOnTracks(true);
                    }
                }
            else 
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                {
                //// TOGGLE MUTE
                local.stepSequencer.muted[local.stepSequencer.currentTrack] = !local.stepSequencer.muted[local.stepSequencer.currentTrack];
                } 
            }    
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        if (button[SELECT_BUTTON])
            {
            isUpdated(SELECT_BUTTON, PRESSED);  // kill the long release on the select button
            if (local.stepSequencer.performanceMode)
                {
                //// SCHEDULE NEXT SEQUENCE
                local.stepSequencer.goNextSequence = !local.stepSequencer.goNextSequence;
                }
            else
                {
                //// ENTER PERFORMANCE MODE
                local.stepSequencer.performanceMode = true;
                local.stepSequencer.goNextSequence = false;
                resetStepSequencerCountdown();  // otherwise we'll miss jumps to other sequences
                setParseRawCC(true);
                }
            }
        else
            {
            if (local.stepSequencer.performanceMode)
                {
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                if (IS_MONO() || IS_DUO())
                    {
                    local.stepSequencer.NEXT_PLAY_TRACK = GET_PRIMARY_TRACK(local.stepSequencer.currentTrack);
                    }
                else
                    {
                    advanceSolo();
                    }
#else
                //// SCHEDULE SOLO
                advanceSolo();
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                }
            else if (local.stepSequencer.currentEditPosition >= 0)
                {
                //// ADD A TIE
                addTie(trackLen);
                }
            else 
                {
                //// CLEAR TRACK
                    
                // do a "light" clear, not a full reset
                memset(data.slot.data.stepSequencer.buffer + ((uint16_t)trackLen) * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
                }
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        if (local.stepSequencer.performanceMode && local.stepSequencer.playState == PLAY_STATE_PLAYING && !local.stepSequencer.scheduleStop)
            {
            local.stepSequencer.scheduleStop = true;
            }
        else
            {       
            switch(local.stepSequencer.playState)
                {
                case PLAY_STATE_STOPPED:
                    {
                    resetStepSequencer();
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                    local.stepSequencer.playTrack = 0;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                    local.stepSequencer.scheduleStop = false;
                    local.stepSequencer.goNextSequence = false;
                    local.stepSequencer.playState = PLAY_STATE_WAITING;
                    // we always stop the clock just in case, even if we're immediately restarting it
                    stopClock(true);
                    // Possible bug condition:
                    // The MIDI spec says that there "should" be at least 1 ms between
                    // starting the clock and the first clock pulse.  I don't know if that
                    // will happen here consistently.
                    startClock(true);
                    }
                break;
                case PLAY_STATE_WAITING:
                    // Fall Thru
                case PLAY_STATE_PLAYING:
                    {
                    stopStepSequencer();
                    return;
                    }
                break;
                }
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        if (button[MIDDLE_BUTTON])
            {
            isUpdated(MIDDLE_BUTTON, PRESSED);  // kill the long release on the middle button
            if (local.stepSequencer.performanceMode)
                {
                //// SCHEDULE NEXT SEQUENCE
                local.stepSequencer.goNextSequence = !local.stepSequencer.goNextSequence;
                }
            else
                {
                //// ENTER PERFORMANCE MODE
                local.stepSequencer.performanceMode = true;
                local.stepSequencer.goNextSequence = false;
                resetStepSequencerCountdown();    // otherwise we'll miss jumps to other sequences
                setParseRawCC(true);
                }
            }
        else
            {
            //// ENTER MENU
            goDownState(STATE_STEP_SEQUENCER_MENU);
            }
        }
    else if (potUpdated[LEFT_POT])
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        //// CHANGE TRACK
        
        uint8_t newTrack = ((pot[LEFT_POT] * numTracks) >> 10);         //  / 1024;
        newTrack = bound(newTrack, 0, numTracks);
// If MONO mode we need to clear notes if we're going to a new track
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (newTrack != local.stepSequencer.lastCurrentTrack)           // we only update if it's different so we can ALSO change the current track using the middle button
            {
            if (!local.stepSequencer.performanceMode && (GET_PRIMARY_TRACK(newTrack) != GET_PRIMARY_TRACK(local.stepSequencer.currentTrack)) && (IS_MONO() || IS_DUO()))    
                {
                clearNotesOnTracks(true);
                }
            local.stepSequencer.lastCurrentTrack = newTrack;                        
            local.stepSequencer.currentTrack = newTrack;
            setParseRawCC(local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC);
            }
#else
        local.stepSequencer.currentTrack = newTrack;
        setParseRawCC(local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC);
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
        }
    else if (potUpdated[RIGHT_POT])
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        if (local.stepSequencer.performanceMode)
            {
            //// CHANGE TEMPO

#define BIG_POT_UPDATE (32)
            if (potChangedBy(local.stepSequencer.pots, RIGHT_POT, BIG_POT_UPDATE))
                {
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_TEMPO);
                }
            }
        else
            {
            //// CHANGE NOTE POSITION
            
            int16_t newPos = getNewCursorXPos(trackLen);
            if (lockoutPots ||      // using an external NRPN device, which is likely accurate
                local.stepSequencer.currentRightPot == -1 ||   // nobody's been entering data
                local.stepSequencer.currentRightPot >= newPos && local.stepSequencer.currentRightPot - newPos >= 2 ||
                local.stepSequencer.currentRightPot < newPos && newPos - local.stepSequencer.currentRightPot >= 2)
                {
                local.stepSequencer.currentEditPosition = newPos;
                                
                if (local.stepSequencer.currentEditPosition >= trackLen)        
                    local.stepSequencer.currentEditPosition = trackLen - 1;
                local.stepSequencer.currentRightPot = -1;
                }
            }
        }
        
        
        
    ///// INCOMING MIDI DATA
  
    if (bypass)
        {
        // do nothing
        }
    
    // rerouting to new channel
    if (newItem && 
        itemType != MIDI_CUSTOM_CONTROLLER && 
        local.stepSequencer.performanceMode && 
        options.stepSequencerPlayAlongChannel != CHANNEL_TRANSPOSE && 
        options.stepSequencerPlayAlongChannel != CHANNEL_ADD_TO_STEP_SEQUENCER)
        {
        TOGGLE_IN_LED();
        // figure out what the channel should be
        uint8_t channelOut = options.stepSequencerPlayAlongChannel;
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
    // transposition
    else if (newItem && 
        itemType == MIDI_NOTE_ON &&
        local.stepSequencer.performanceMode && 
        options.stepSequencerPlayAlongChannel == CHANNEL_TRANSPOSE)
        {
        TOGGLE_IN_LED();

#define MIDDLE_C (60)

        local.stepSequencer.transpose = ((int8_t)itemNumber) - (int8_t) MIDDLE_C;  // this can only range -60 ... 67
        }


    else if (newItem && (itemType == MIDI_NOTE_ON)  //// there is a note played
        && local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NOTE)
        {
        TOGGLE_IN_LED();
        uint8_t note = itemNumber;
        uint8_t velocity = itemValue;
        
        if ((local.stepSequencer.muted[local.stepSequencer.currentTrack] && !options.stepSequencerNoEcho)
            || (local.stepSequencer.performanceMode && options.stepSequencerPlayAlongChannel == CHANNEL_ADD_TO_STEP_SEQUENCER ))
            {
            // play the note
            sendTrackNote(note, velocity, local.stepSequencer.currentTrack);
            }
        else
            {
            // here we're trying to provide some slop so the user can press the note early.
            // we basically are rounding up or down to the nearest note
            uint8_t pos = (local.stepSequencer.currentEditPosition < 0  || (local.stepSequencer.performanceMode)) ? 
                local.stepSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0) :
                local.stepSequencer.currentEditPosition;
            if (pos >= trackLen) pos = 0;
            // If there's a previous note which has not finished, tie it up to pos
            if (local.stepSequencer.currentEditPosition < 0)
                tieNote(pos);


#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
/// "Tie Notes" are only with the advanced sequencer
            if (note == options.stepSequencerTieNote)   // aha!  Add a tie
                {
                removeSuccessiveTies(pos, trackLen);
                addTie(trackLen);
                local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
                local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                }
            else
                {            
/// "Rest Notes" are only with the advanced sequencer
                if (note == options.stepSequencerRestNote)  // aha!  Add a rest
                    {
                    note = 0;
                    velocity = 0;
                    }
                loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + pos, note, velocity);
                removeSuccessiveTies(pos, trackLen);
                local.stepSequencer.lastNote = note;
                local.stepSequencer.lastNotePos = pos;
                }
#else
            loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + pos, note, velocity);
            removeSuccessiveTies(pos, trackLen);
            local.stepSequencer.lastNote = note;
            local.stepSequencer.lastNotePos = pos;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

            
            if (local.stepSequencer.currentEditPosition >= 0 && !(local.stepSequencer.performanceMode)
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                && note != options.stepSequencerTieNote
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                )
                {
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
                }
            else 
                {
                local.stepSequencer.dontPlay[local.stepSequencer.currentTrack] = 1;

// We have to check for rest notes here and not echo them
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                if (!options.stepSequencerNoEcho && note != options.stepSequencerRestNote && note != options.stepSequencerTieNote)
#else
                    if (!options.stepSequencerNoEcho)               // only play if we're echoing
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                        {
                        sendTrackNote(note, velocity, local.stepSequencer.currentTrack);
                        }
                }
                        
            local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
            }
        }
    else if (newItem && (itemType == MIDI_NOTE_OFF)
        && (!(local.stepSequencer.performanceMode) || options.stepSequencerPlayAlongChannel == CHANNEL_ADD_TO_STEP_SEQUENCER )
        && local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NOTE)
        {
        if (local.stepSequencer.lastNote == itemNumber && local.stepSequencer.currentEditPosition < 0)
            {
            // here we're trying to provide some slop so the user can press the note early.
            // we basically are rounding up or down to the nearest note
            uint8_t pos = (local.stepSequencer.currentEditPosition < 0 || (local.stepSequencer.performanceMode)) ? 
                local.stepSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0) :
                local.stepSequencer.currentEditPosition;
            if (pos >= trackLen) pos = 0;

            // If we just finished the note, tie it up to pos
            tieNote(pos);
            }
        sendTrackNote(itemNumber + 128, itemValue, local.stepSequencer.currentTrack);
        }
    else if (newItem && (itemType == MIDI_AFTERTOUCH || itemType == MIDI_AFTERTOUCH_POLY))
        {
        
// Aftertouch is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_AFTERTOUCH)
            {
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = (itemValue << 7) + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
           
        // pass through -- hope this is right
        // Note that we're always converting to channel aftertouch
         
        sendControllerCommand(CONTROL_TYPE_AFTERTOUCH, 0, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_PROGRAM_CHANGE))
        {

// PC is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_PC)
            {
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = (itemNumber << 7) + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_PC, 0, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_PITCH_BEND))
        {
// Pitch Bend is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_BEND)
            {
            uint16_t v = itemValue;
            if (v == CONTROL_VALUE_EMPTY)
                v = MAX_CONTROL_VALUE;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_PITCH_BEND, 0, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_CC_7_BIT))
        {

// 7-bit CC is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if ((local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC ||
                local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_14_BIT_CC) &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = (itemValue << 7) + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
        
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_CC, itemNumber, itemValue << 7, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_CC_14_BIT))
        {

// 14-bit CC is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_14_BIT_CC &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            uint16_t v = itemValue;
            if (v == CONTROL_VALUE_EMPTY)
                v = MAX_CONTROL_VALUE;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
 
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_CC, itemNumber, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {

// NRPN is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NRPN &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            uint16_t v = itemValue;
            if (v == CONTROL_VALUE_EMPTY)
                v = MAX_CONTROL_VALUE;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_NRPN, itemNumber, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_RPN_14_BIT))
        {
// RPN is entered into non-note tracks
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_RPN &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            uint16_t v = itemValue;
            if (v == CONTROL_VALUE_EMPTY)
                v = MAX_CONTROL_VALUE;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_RPN, itemNumber, itemValue, options.channelOut);
        }

// The custom controller is only available for the advanced step sequencer
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    else if (newItem && (itemType == MIDI_CUSTOM_CONTROLLER))
        {
        local.stepSequencer.lastNote = NO_SEQUENCER_NOTE;
        local.stepSequencer.lastNotePos = NO_SEQUENCER_POS;
                
        switch (itemNumber)
            {
            case CC_EXTRA_PARAMETER_A:
            case CC_EXTRA_PARAMETER_B:
            case CC_EXTRA_PARAMETER_C:
            case CC_EXTRA_PARAMETER_D:
            case CC_EXTRA_PARAMETER_E:
            case CC_EXTRA_PARAMETER_F:
            case CC_EXTRA_PARAMETER_G:
            case CC_EXTRA_PARAMETER_H:
            case CC_EXTRA_PARAMETER_I:
            case CC_EXTRA_PARAMETER_J:
            case CC_EXTRA_PARAMETER_K:
            case CC_EXTRA_PARAMETER_L:
                {
                if (local.stepSequencer.performanceMode)
                    {
                    advanceMute(itemNumber - CC_EXTRA_PARAMETER_A);
                    }
                else
                    {
#ifdef INCLUDE_STEP_SEQUENCER_CC_MUTE_TOGGLES
                    local.stepSequencer.muted[itemNumber - CC_EXTRA_PARAMETER_A] = itemValue;
#else
                    local.stepSequencer.muted[itemNumber - CC_EXTRA_PARAMETER_A] = !local.stepSequencer.muted[itemNumber - CC_EXTRA_PARAMETER_A];
#endif
                    }
                break;
                }
            case CC_EXTRA_PARAMETER_M:
            case CC_EXTRA_PARAMETER_N:
            case CC_EXTRA_PARAMETER_O:
            case CC_EXTRA_PARAMETER_P:
            case CC_EXTRA_PARAMETER_Q:
            case CC_EXTRA_PARAMETER_R:
            case CC_EXTRA_PARAMETER_S:
            case CC_EXTRA_PARAMETER_T:
            case CC_EXTRA_PARAMETER_U:
            case CC_EXTRA_PARAMETER_V:
            case CC_EXTRA_PARAMETER_W:
            case CC_EXTRA_PARAMETER_X:
                {
                if (itemNumber - (CC_EXTRA_PARAMETER_A + 12) < GET_NUM_TRACKS() )  // track is valid
                    {
                    local.stepSequencer.currentTrack = itemNumber - (CC_EXTRA_PARAMETER_A + 12);
                    }
                break;
                }
            case CC_EXTRA_PARAMETER_Y:
                {
                if (local.stepSequencer.performanceMode)
                    advanceSolo();
                else
                    local.stepSequencer.solo = !local.stepSequencer.solo;
                break;
                }
            case CC_EXTRA_PARAMETER_Z:
                {
                // transposable
                local.stepSequencer.transposable[local.stepSequencer.currentTrack] = !local.stepSequencer.transposable[local.stepSequencer.currentTrack];
                break;
                }
            case CC_EXTRA_PARAMETER_1:
                {
                // do a "light" clear, not a full reset
                memset(data.slot.data.stepSequencer.buffer + ((uint16_t)trackLen) * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
                break;
                }
            case CC_EXTRA_PARAMETER_2:
                {
                // toggle schedule next sequence
                if (local.stepSequencer.performanceMode)
                    {
                    local.stepSequencer.goNextSequence = !local.stepSequencer.goNextSequence;
                    }
                break;
                }
            case CC_EXTRA_PARAMETER_3:
                {
                IMMEDIATE_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_CLICK);
                break;
                }
            case CC_EXTRA_PARAMETER_4:
                {
                IMMEDIATE_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_EDIT_MARK);
                break;
                }
            case CC_EXTRA_PARAMETER_5:
                {
                IMMEDIATE_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_EDIT_COPY);
                break;
                }
            case CC_EXTRA_PARAMETER_6:
                {
                IMMEDIATE_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_EDIT_SPLAT);
                break;
                }
            case CC_EXTRA_PARAMETER_7:
                {
                IMMEDIATE_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_EDIT_MOVE);
                break;
                }
            case CC_EXTRA_PARAMETER_8:
                {
                IMMEDIATE_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_EDIT_DUPLICATE);
                break;
                }
                        
                        
            // this is a discontinuity, hope compiler can handle it
                        
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_1:
                {
                // length
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_STEP_SEQUENCER_NOTE_LENGTH);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_2:
                {
                // midi out
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_3:
                {
                // velocity
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_STEP_SEQUENCER_VELOCITY);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_4:
                {
                // fader
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_STEP_SEQUENCER_FADER);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_5:
                {
                // pattern
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_PATTERN);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_6:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_TEMPO);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_7:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_TRANSPOSE);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_8:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_VOLUME);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_9:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_NOTE_SPEED);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_10:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_PLAY_LENGTH);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_11:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_SWING);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_12:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_PERFORMANCE_KEYBOARD);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_13:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_PERFORMANCE_REPEAT);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_14:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_PERFORMANCE_NEXT);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_15:
                {
                // Select Length
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_STEP_SEQUENCER_MENU_LENGTH);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_6_LSB:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_STEP_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_TEMPO);
                break;
                }
            }
        }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    playStepSequencer();
    if (updateDisplay)
        {
        drawStepSequencer(trackLen, GET_TRACK_FULL_LENGTH(), numTracks);
        }
    }


// Various choices in the menu
#define STEP_SEQUENCER_MENU_SOLO 0
#define STEP_SEQUENCER_MENU_RESET 1
#define STEP_SEQUENCER_MENU_NOTE_LENGTH 2
#define STEP_SEQUENCER_MENU_MIDI_OUT 3
#define STEP_SEQUENCER_MENU_VELOCITY 4
#define STEP_SEQUENCER_MENU_FADER 5






// Advanced step sequencer has two more menu options: type (note, non-note) and rest notes
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER

#define STEP_SEQUENCER_MENU_TYPE 6
#define STEP_SEQUENCER_MENU_PATTERN 7
#define STEP_SEQUENCER_MENU_TRANSPOSABLE 8
#define STEP_SEQUENCER_MENU_EDIT 9
#define STEP_SEQUENCER_MENU_NO_ECHO 10
#define STEP_SEQUENCER_MENU_REST 11
#define STEP_SEQUENCER_MENU_TIE 12
#define STEP_SEQUENCER_MENU_SHOW_NOTE 13
#define STEP_SEQUENCER_MENU_LENGTH 14
#define STEP_SEQUENCER_MENU_PERFORMANCE 15
#define STEP_SEQUENCER_MENU_SAVE 16
#define STEP_SEQUENCER_MENU_OPTIONS 17

#else


///// FIXME: I have temporarily removed NoEcho as an option for the standard step sequencer in order
///// to get it to fit.  Not sure what I had done which had bloated it.  Trying to squeeze it back down
///// again.  At any rate, the noEcho checks are still in the standard step sequencer code, I've not
///// ifdef'ed them out (nor in Options).

#define STEP_SEQUENCER_MENU_PATTERN 6
#define STEP_SEQUENCER_MENU_TRANSPOSABLE 7
#define STEP_SEQUENCER_MENU_EDIT 8
#define STEP_SEQUENCER_MENU_NO_ECHO 9
#define STEP_SEQUENCER_MENU_LENGTH 10
#define STEP_SEQUENCER_MENU_PERFORMANCE 11
#define STEP_SEQUENCER_MENU_SAVE 12
#define STEP_SEQUENCER_MENU_OPTIONS 13

#endif INCLUDE_ADVANCED_STEP_SEQUENCER






// Gives other options
void stateStepSequencerMenu()
    {
    uint8_t result;
    
// Advanced step sequencer has two more menu options: type (note, non-note) and rest notes
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    const char* menuItems[18] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("RESET TRACK"),
        PSTR("NOTE LENGTH (TRACK)"),
        (IS_MONO() || IS_DUO()) ? PSTR("OUT MIDI (TRACK)") : PSTR("OUT MIDI") ,
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("TYPE (TRACK)"),
        ((IS_MONO() || IS_DUO()) ? PSTR("REPEAT (TRACK)") : PSTR("PATTERN (TRACK)")),
        local.stepSequencer.transposable[local.stepSequencer.currentTrack] ? PSTR("NO TRANSPOSE (TRACK)") : PSTR("TRANSPOSE (TRACK)"),
        PSTR("EDIT"),
        options.stepSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        PSTR("REST NOTE"),
        PSTR("TIE NOTE"),
        options.stepSequencerShowNote ? PSTR("NO SHOW NOTE") : PSTR("SHOW NOTE"),
        PSTR("LENGTH"),
        PSTR("PERFORMANCE"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 18, STATE_NONE, STATE_NONE, 1);
#else
    const char* menuItems[13] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("RESET TRACK"),
        PSTR("NOTE LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("PATTERN (TRACK)"),
        local.stepSequencer.transposable[local.stepSequencer.currentTrack] ? PSTR("NO TRANSPOSE (TRACK)") : PSTR("TRANSPOSE (TRACK)"),
        PSTR("EDIT"),
//        options.stepSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        PSTR("LENGTH"),
        PSTR("PERFORMANCE"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 13, STATE_NONE, STATE_NONE, 1);
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            goUpState(STATE_STEP_SEQUENCER_PLAY);
            switch(currentDisplay)
                {
                case STEP_SEQUENCER_MENU_SOLO:
                    {
                    local.stepSequencer.solo = !local.stepSequencer.solo;
                    }
                break;
                case STEP_SEQUENCER_MENU_RESET:
                    {
                    resetTrack(local.stepSequencer.currentTrack);
                    break;
                    }
                case STEP_SEQUENCER_MENU_NOTE_LENGTH:
                    {
                    goDownState(STATE_STEP_SEQUENCER_NOTE_LENGTH);                            
                    }
                break;
                case STEP_SEQUENCER_MENU_MIDI_OUT:
                    {
                    local.stepSequencer.backup = outMIDI(local.stepSequencer.currentTrack);
                    goDownState(STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT);
                    }
                break;
                case STEP_SEQUENCER_MENU_VELOCITY:
                    {
                    goDownState(STATE_STEP_SEQUENCER_VELOCITY);
                    }
                break;
                case STEP_SEQUENCER_MENU_FADER:
                    {
                    goDownState(STATE_STEP_SEQUENCER_FADER);
                    }
                break;

// Advanced step sequencer has two more menu options: type (note, non-note) and rest notes
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                case STEP_SEQUENCER_MENU_TYPE:
                    {
                    goDownState(STATE_STEP_SEQUENCER_MENU_TYPE);
                    }
                break;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

                case STEP_SEQUENCER_MENU_PATTERN:
                    {
                    immediateReturnState = STATE_STEP_SEQUENCER_PLAY;
                    goDownState(STATE_STEP_SEQUENCER_MENU_PATTERN);
                    }
                break;
                case STEP_SEQUENCER_MENU_TRANSPOSABLE:
                    {
                    local.stepSequencer.transposable[local.stepSequencer.currentTrack] = !local.stepSequencer.transposable[local.stepSequencer.currentTrack];
                    }
                break;
                case STEP_SEQUENCER_MENU_EDIT:
                    {
                    goDownState(STATE_STEP_SEQUENCER_MENU_EDIT);
                    }
                break;
                case STEP_SEQUENCER_MENU_NO_ECHO:
                    {
                    options.stepSequencerNoEcho = !options.stepSequencerNoEcho;
                    saveOptions();
                    }
                break;

// Advanced step sequencer has two more menu options: type (note, non-note) and rest notes
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                case STEP_SEQUENCER_MENU_REST:
                    {
                    goDownState(STATE_STEP_SEQUENCER_MENU_REST);
                    }
                break;
                case STEP_SEQUENCER_MENU_TIE:
                    {
                    goDownState(STATE_STEP_SEQUENCER_MENU_TIE);
                    }
                break;
                case STEP_SEQUENCER_MENU_SHOW_NOTE:
                    {
                    options.stepSequencerShowNote = !options.stepSequencerShowNote;
                    saveOptions();
                    }
                break;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

                case STEP_SEQUENCER_MENU_LENGTH:
                    {
                    immediateReturnState = STATE_STEP_SEQUENCER_PLAY;
                    goDownState(STATE_STEP_SEQUENCER_MENU_LENGTH);
                    }
                break;
                case STEP_SEQUENCER_MENU_PERFORMANCE:
                    {
                    immediateReturnState = STATE_STEP_SEQUENCER_MENU;
                    goDownState(STATE_STEP_SEQUENCER_MENU_PERFORMANCE);
                    }
                break;
                case STEP_SEQUENCER_MENU_SAVE:
                    {
                    goDownState(STATE_STEP_SEQUENCER_SAVE);
                    }
                break;
                case STEP_SEQUENCER_MENU_OPTIONS:
                    {
                    immediateReturnState = STATE_STEP_SEQUENCER_MENU;
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
            goUpState(STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        }
        
    }
    
    
// Advanced step sequencer has non-note data
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
// Gives other options
void stateStepSequencerMenuType()
    {
    uint8_t result;
    
    const char* menuItems[8] = { PSTR("NOTE"), PSTR("CC"), PSTR("14 BIT CC"), PSTR("NRPN"), PSTR("RPN"), 
                                 PSTR("PC"), PSTR("BEND"), PSTR("AFTERTOUCH") };
    result = doMenuDisplay(menuItems, 8, STATE_OPTIONS_TEMPO, immediateReturnState, 1);

    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            if (currentDisplay == STEP_SEQUENCER_DATA_CC ||
                currentDisplay == STEP_SEQUENCER_DATA_14_BIT_CC ||
                currentDisplay == STEP_SEQUENCER_DATA_NRPN ||
                currentDisplay == STEP_SEQUENCER_DATA_RPN)
                {
                local.stepSequencer.newData = currentDisplay;
                goDownState(STATE_STEP_SEQUENCER_MENU_TYPE_PARAMETER);
                }
            else
                {
                resetTrack(local.stepSequencer.currentTrack);
                local.stepSequencer.data[local.stepSequencer.currentTrack] = currentDisplay;  
                // get rid of any residual select button calls, so we don't stop when exiting here
                isUpdated(SELECT_BUTTON, RELEASED);
                goDownState(STATE_STEP_SEQUENCER_PLAY);
                }
            }
        break;
        case MENU_CANCELLED:
            {
            // get rid of any residual select button calls, so we don't stop when exiting here
            isUpdated(SELECT_BUTTON, RELEASED);
            goUpState(STATE_STEP_SEQUENCER_MENU);
            }
        break;
        }
    }
    
// Gives other options
void stateStepSequencerMenuTypeParameter()
    {
    uint16_t max = (local.stepSequencer.newData == STEP_SEQUENCER_DATA_CC ? 127 : 
        (local.stepSequencer.newData == STEP_SEQUENCER_DATA_14_BIT_CC ? 31 : 16383));
    uint8_t result = doNumericalDisplay(0, max, local.stepSequencer.controlParameter[local.stepSequencer.currentTrack], 0, GLYPH_NONE);
    playStepSequencer();
    
    switch (result)
        {
        case NO_MENU_SELECTED:
            // nothing
            break;
        case MENU_SELECTED:
            resetTrack(local.stepSequencer.currentTrack);
            local.stepSequencer.data[local.stepSequencer.currentTrack] = local.stepSequencer.newData;  
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] = currentDisplay;
            goDownState(STATE_STEP_SEQUENCER_PLAY);
            break;
        case MENU_CANCELLED:
            goUpState(STATE_STEP_SEQUENCER_MENU);
            break;
        }
    }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER






// Turns off all notes
void clearNotesOnTracks(uint8_t clearEvenIfNoteNotFinished)
    {
    uint8_t numTracks = GET_NUM_TRACKS();
    uint8_t trackLen = GET_TRACK_LENGTH();
    for(uint8_t track = 0; track < numTracks; track++)
        {
        // clearNotesOnTracks is called BEFORE the current play position is incremented.
        // So we need to check the NEXT note to determine if it's a tie.  If it is,
        // then we don't want to stop playing

        uint8_t currentPos = local.stepSequencer.currentPlayPosition + 1;       // next position
        if (currentPos >= trackLen) currentPos = 0;                                                     // wrap around
        uint16_t pos = (track * (uint16_t) trackLen + currentPos) * 2;
        uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
        uint8_t note = data.slot.data.stepSequencer.buffer[pos];
        
        if (local.stepSequencer.noteOff[track] < NO_NOTE &&             // there's something to turn off
            //(clearEvenIfNoteNotFinished || (currentTime >= local.stepSequencer.offTime[track])) &&  // it's time to clear the note
            (clearEvenIfNoteNotFinished || (TIME_GREATER_THAN_OR_EQUAL(currentTime, local.stepSequencer.offTime[track]))) &&  // it's time to clear the note
            (!((vel == 0) && (note == 1))))                                     // not a tie
            {
            uint8_t out = (outMIDI(track) == MIDI_OUT_DEFAULT ? options.channelOut : outMIDI(track));
            if (out != NO_MIDI_OUT)
                {
                sendNoteOff(local.stepSequencer.noteOff[track], 127, out);
                }
            local.stepSequencer.noteOff[track] = NO_NOTE;
            }
        }
    }



// Advanced step sequencer has non-note data
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER  
// this method exists to avoid storing this data in RAM
uint8_t getMonoRepeats(uint8_t val)
    {
    // The last one (END) is 0 because, well, I dunno why...
    const uint8_t _monoRepeats[16] = { 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 15, 23, 31, 63, 0 };
    return _monoRepeats[val];
    }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER  



// Plays the current sequence
void playStepSequencer()
    {
    // we redo this rather than take it from stateStepSequencerPlay because we may be 
    // called from other methods as well 
        
    uint8_t trackLen = GET_TRACK_LENGTH();
    uint8_t numTracks = GET_NUM_TRACKS();
        
    // Clear notes if necessary
    clearNotesOnTracks(false);
        
    if ((local.stepSequencer.playState == PLAY_STATE_WAITING) && beat)
        local.stepSequencer.playState = PLAY_STATE_PLAYING;
        
    if (notePulse && (local.stepSequencer.playState == PLAY_STATE_PLAYING))
        {
        // definitely clear everything
        clearNotesOnTracks(true);
                
        // remember and update the current play position to the next step
        uint8_t oldPlayPosition = local.stepSequencer.currentPlayPosition;
        local.stepSequencer.currentPlayPosition = incrementAndWrap(local.stepSequencer.currentPlayPosition, trackLen);
        
        // If we're in performance mode, and a stop is scheduled, and we're at the right place, stop
        if (local.stepSequencer.performanceMode && local.stepSequencer.scheduleStop && local.stepSequencer.currentPlayPosition == options.stepSequencerStop)
            {
            stopStepSequencer(); 
            local.stepSequencer.goNextSequence = false;     // totally reset
            return; 
            }                                       

        // If we're at the beginning of the bar, we need to potentially update a bunch of stuff                
        if (local.stepSequencer.currentPlayPosition == 0)
            {
            // update the pattern counter
            local.stepSequencer.countup++;

            // change scheduled mute?
            if (local.stepSequencer.performanceMode)
                {
                for(uint8_t track = 0; track < numTracks; track++)
                    {
                    if (local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTE_ON_SCHEDULED)
                        local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTED;
                    else if (local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTE_OFF_SCHEDULED)
                        local.stepSequencer.muted[track] = STEP_SEQUENCER_NOT_MUTED;
                    else if (local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTE_ON_SCHEDULED_ONCE)
                        local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTE_OFF_SCHEDULED;
                    else if (local.stepSequencer.muted[track] == STEP_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE)
                        local.stepSequencer.muted[track] = STEP_SEQUENCER_MUTE_ON_SCHEDULED;
                    }

// Mono Mode never solos
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER  
                if (IS_MONO() || IS_DUO())
                    local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;                                              // we never solo
                else if (local.stepSequencer.solo == STEP_SEQUENCER_SOLO_ON_SCHEDULED)
                    local.stepSequencer.solo = STEP_SEQUENCER_SOLO;
                else if (local.stepSequencer.solo == STEP_SEQUENCER_SOLO_OFF_SCHEDULED)
                    local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;
#else              
                if (local.stepSequencer.solo == STEP_SEQUENCER_SOLO_ON_SCHEDULED)
                    local.stepSequencer.solo = STEP_SEQUENCER_SOLO;
                else if (local.stepSequencer.solo == STEP_SEQUENCER_SOLO_OFF_SCHEDULED)
                    local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER



// MONO mode handles countup and countdown differently from the standard step sequencer
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                /// Mono uses countup to repeat tracks, and uses countdown to repeat the entire sequence.
                /// This is different from standard, which doesn't repeat tracks. 
                if (IS_MONO() || IS_DUO())
                    {
                    /// (In performance mode)
                    /// If we're scheduled to go the next sequence, OR
                    /// If we're at the end of the track and the countdown is 0 and either we're at the final track or the next track is END
                    /// then we need to terminate the sequence
                    if (local.stepSequencer.goNextSequence || 
                            (oldPlayPosition == trackLen - 1 && local.stepSequencer.countdown == 0 && 
                                (local.stepSequencer.playTrack >= GET_NUM_TRACKS() - 1 || 
                                local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.playTrack + (IS_MONO() ? 1 : 2))] == STEP_SEQUENCER_MONO_REPEATS_END)))
                        {
                        uint8_t nextSequence = (data.slot.data.stepSequencer.repeat >> 4);
                        if (nextSequence == 0) // STOP
                            {
                            stopStepSequencer(); 
                            return; 
                            }
                        else
                            {
                            if (!loadSequence(nextSequence - 1))  // maybe this will work out of the box?  hmmm 
                                {
                                stopStepSequencer();
                                return;
                                }
                            }
                        }
                    else if (local.stepSequencer.NEXT_PLAY_TRACK != NO_TRACK)
                        {
                        local.stepSequencer.countup = 0;                                                        // we've already incremented the countup, so we can't do 255
                        local.stepSequencer.playTrack = local.stepSequencer.NEXT_PLAY_TRACK;
                        if (local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.playTrack)] == STEP_SEQUENCER_MONO_REPEATS_END)
                            {
                            local.stepSequencer.playTrack = 0;
                            }
                        local.stepSequencer.NEXT_PLAY_TRACK = NO_TRACK;
                        }

                    /// (In performance mode)
                    /// Else if we're at the end of the track and the countup is equal the number of repeats, so the track is done...
                    /// then we need to advance the track
                    else if (oldPlayPosition == trackLen - 1 && 
                        local.stepSequencer.countup > getMonoRepeats(local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.playTrack)]))
                        {
                        local.stepSequencer.playTrack += (IS_DUO() ? 2 : 1);
                        if (local.stepSequencer.playTrack >= numTracks ||
                            local.stepSequencer.MONO_REPEATS[GET_PRIMARY_TRACK(local.stepSequencer.playTrack)] == STEP_SEQUENCER_MONO_REPEATS_END)
                            {
                            local.stepSequencer.playTrack = 0;
                            }
                                        
                        // now reset countup
                        local.stepSequencer.countup = 0;                                                        // we've already incremented the countup, so we can't do 255
                                        
                        // finally decrease countdown
                        if (local.stepSequencer.countdown != 255)  // not forever
                            {
                            local.stepSequencer.countdown--;
                            }                       
                        }
                    }
                else
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                    {
                    if (local.stepSequencer.goNextSequence || 
                        (oldPlayPosition == trackLen - 1 && local.stepSequencer.countdown == 0))
                        {
                        uint8_t nextSequence = (data.slot.data.stepSequencer.repeat >> 4);
                        if (nextSequence == 0) // STOP
                            {
                            stopStepSequencer(); 
                            return; 
                            }
                        else
                            {
                            if (!loadSequence(nextSequence - 1))  // maybe this will work out of the box?  hmmm 
                                {
                                stopStepSequencer();
                                return;
                                }
                            }
                        }
                    else if (oldPlayPosition == trackLen - 1 && local.stepSequencer.countdown != 255)  // not forever
                        {
                        local.stepSequencer.countdown--;
                        }                       
                    }
                                
                                
                                
                                
                                
// Patterns are not done in MONO
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                if (IS_STANDARD())
#endif INCLUDE_ADVANCED_STEP_SEQUENCER        
                    // pick an exclusive random track
                    {
                    // determine options
                    uint8_t exclusiveTracks[MAX_STEP_SEQUENCER_TRACKS];
                    uint8_t oneExclusive = NO_TRACK;
                    uint8_t numExclusiveTracks = 0;
                    for(uint8_t track = 0; track < numTracks; track++)
                        {
                        if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE || 
                            (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL && ((local.stepSequencer.countup & 3) == 3)))
                            {
                            oneExclusive = track;           // found something, might be the last one
                            if (track != local.stepSequencer.lastExclusiveTrack)
                                {
                                exclusiveTracks[numExclusiveTracks++] = track;          // found a track that's not the last one
                                }
                            }
                        }
                        
                    // pick a track
                    if (numExclusiveTracks == 0)                // this happens if there are NO X tracks or ONE X track which has already been used
                        {
                        if (oneExclusive != NO_TRACK)   // if NO_TRACK, we don't want to reset so we can keep the lastExclusiveTrack to the fourth measure for ---X
                            local.stepSequencer.lastExclusiveTrack = oneExclusive;
                        }
                    else
                        {
                        local.stepSequencer.lastExclusiveTrack = exclusiveTracks[random(numExclusiveTracks)];
                        }
                    }
                }
//// END PERFORMANCE MODE SECTION
            }
//// END START-OF-TRACK SECTION



         
        for(uint8_t track = 0; track < numTracks; track++)
            {
            // data is stored per-track as
            // NOTE VEL
            // We need to strip off the high bit because it's used for other packing
                                
            // for each note in the track
            uint16_t pos = (track * (uint16_t) trackLen + local.stepSequencer.currentPlayPosition) * 2;
            uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
            uint8_t note = data.slot.data.stepSequencer.buffer[pos];
            uint8_t noteLength = ((local.stepSequencer.noteLength[track] == PLAY_LENGTH_USE_DEFAULT) ? 
                options.noteLength : local.stepSequencer.noteLength[track] );
 
            if (local.stepSequencer.currentPlayPosition == 0 
/// MONO mode does not do patterns            
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                && IS_STANDARD()
#endif INCLUDE_ADVANCED_STEP_SEQUENCER                  
                )
                {
                // pick a random track                          
                if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE)
                    {
                    local.stepSequencer.shouldPlay[track] = (track == local.stepSequencer.lastExclusiveTrack);
                    }
                else if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL)
                    {
                    local.stepSequencer.shouldPlay[track] = (track == local.stepSequencer.lastExclusiveTrack && ((local.stepSequencer.countup & 3) == 3));
                    }
                else if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_1_2)
                    {
                    local.stepSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 2));
                    }
                else if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_1_4)
                    {
                    local.stepSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 4));
                    }
                else if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_1_8)
                    {
                    local.stepSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 8));
                    }
                else
                    {
                    local.stepSequencer.shouldPlay[track] = ((local.stepSequencer.pattern[track] >> (local.stepSequencer.countup & 3)) & 1);                        
                    }
                                        
                if (!local.stepSequencer.shouldPlay[track]) 
                    clearNoteOnTrack(track);
                }
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            else if (IS_MONO() || IS_DUO())                         // MONO MODE always plays a track if it's the current track -- note that this is called every timestep, not just timestep 0
                {
                if ((local.stepSequencer.performanceMode && GET_PRIMARY_TRACK(track) == GET_PRIMARY_TRACK(local.stepSequencer.playTrack)) ||
                    (!local.stepSequencer.performanceMode && GET_PRIMARY_TRACK(track) == GET_PRIMARY_TRACK(local.stepSequencer.currentTrack)))
                    {
                    local.stepSequencer.shouldPlay[track] = true;
                    }
                else
                    {
                    local.stepSequencer.shouldPlay[track] = false;
                    clearNoteOnTrack(track);                // just in case
                    }
                }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER                  
                              
            uint8_t shouldPlay = local.stepSequencer.shouldPlay[track] ;
                                
//// Advanced step sequencer has non-note (data tracks)
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
            if (local.stepSequencer.data[track] != STEP_SEQUENCER_DATA_NOTE && shouldPlay)
                {
                uint16_t value = ( ((note & 127) << 7) | (vel & 127) );
                if (value != 0)
                    {
                    value = value - 1;
                    // recall that 0 is "OFF" for sendControllerCommand
                    sendControllerCommand(local.stepSequencer.data[track],
                        local.stepSequencer.controlParameter[track],
                        value,
                        (outMIDI(track) == MIDI_OUT_DEFAULT ? options.channelOut : outMIDI(track)));
                    }
                }
            else 
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                if (vel == 0 && note == 1 && shouldPlay)  // tie
                    {
                    local.stepSequencer.offTime[track] = currentTime + (div100(notePulseRate * getMicrosecsPerPulse() * noteLength));
                    }
                else if (vel != 0 
                    && !local.stepSequencer.dontPlay[track]  // not a rest or tie
                    && local.stepSequencer.shouldPlay[track] 
                    //&& ((!local.stepSequencer.solo && local.stepSequencer.muted[track]) ||                 // If solo is off AND we're not muted......  OR...
                    //    (local.stepSequencer.solo && track == local.stepSequencer.currentTrack)))       // Solo is turned on we're the current track regardless of mute
                    && !shouldMuteTrack(track))
                    {
                    if (local.stepSequencer.velocity[track] != STEP_SEQUENCER_NO_OVERRIDE_VELOCITY)
                        vel = local.stepSequencer.velocity[track];

                    uint16_t newvel = (vel * (uint16_t)(local.stepSequencer.fader[track])) >> 4;    // >> 4 is / FADER_IDENTITY_VALUE, that is, / 16
                    if (newvel > 127) 
                        newvel = 127;
                    if (local.stepSequencer.performanceMode && local.stepSequencer.transposable[track])
                        {
                        // transpose can only go -60 ... 67, so
                        // newNote can only go -60 ... 67 + 127
                        int16_t newNote = local.stepSequencer.transpose + (int16_t) note;
                        if (newNote >= 0 && newNote <= 127)
                            {
                            note = (uint8_t) newNote;
                            }
                        }
                    sendTrackNote(note, (uint8_t)newvel, track);         
                        
                    local.stepSequencer.offTime[track] = currentTime + (div100(notePulseRate * getMicrosecsPerPulse() * noteLength));
                    local.stepSequencer.noteOff[track] = note;
                    }
                else //if (vel == 0 && note == 0) // rest or something weird
                    {
                    local.stepSequencer.noteOff[track] = NO_NOTE;
                    }
            }
        // clear the dontPlay flags
        memset(local.stepSequencer.dontPlay, 0, numTracks);
        }

    // click track
    doClick();
    }
    

//// Rest notes are only available with the advanced step sequencer
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
void stateStepSequencerMenuRest()
    {
    uint8_t note = stateEnterNote(STATE_STEP_SEQUENCER_MENU, true);
    if (note == NOTE_REMOVED)
        {
        options.stepSequencerRestNote = NO_SEQUENCER_NOTE;
        saveOptions();
        goDownState(STATE_STEP_SEQUENCER_MENU);
        }
    else if (note != NO_NOTE)
        {
        options.stepSequencerRestNote = note;
        saveOptions();
        goDownState(STATE_STEP_SEQUENCER_MENU);
        }
    }

void stateStepSequencerMenuTie()
    {
    uint8_t note = stateEnterNote(STATE_STEP_SEQUENCER_MENU, true);
    if (note == NOTE_REMOVED)
        {
        options.stepSequencerTieNote = NO_SEQUENCER_NOTE;
        saveOptions();
        goDownState(STATE_STEP_SEQUENCER_MENU);
        }
    else if (note != NO_NOTE)
        {
        options.stepSequencerTieNote = note;
        saveOptions();
        goDownState(STATE_STEP_SEQUENCER_MENU);
        }
    }
#endif INCLUDE_ADVANCED_STEP_SEQUENCER

#endif INCLUDE_STEP_SEQUENCER

