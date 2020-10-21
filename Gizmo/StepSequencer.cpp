////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#ifdef INCLUDE_STEP_SEQUENCER


// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
//GLOBAL uint8_t _trackLength[5] = { 16, 24, 32, 48, 64 };
GLOBAL uint8_t _trackLength[4] = { 16, 24, 32, 64 };
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
//GLOBAL uint8_t _numTracks[5] = { 12, 8, 6, 4, 3 };
GLOBAL uint8_t _numTracks[4] = { 12, 8, 6, 3 };




void stateStepSequencerMenuEditMark()
    {
    local.stepSequencer.markTrack = local.stepSequencer.currentTrack;
    local.stepSequencer.markPosition = local.stepSequencer.currentEditPosition;
    if (local.stepSequencer.markPosition < 0)
        local.stepSequencer.markPosition = 0;
    goUpState(immediateReturnState);
    }

void stateStepSequencerMenuEditDuplicate()
    {
    local.stepSequencer.data[local.stepSequencer.currentTrack] = local.stepSequencer.data[local.stepSequencer.markTrack];
    local.stepSequencer.outMIDI[local.stepSequencer.currentTrack] = local.stepSequencer.outMIDI[local.stepSequencer.markTrack];
    local.stepSequencer.noteLength[local.stepSequencer.currentTrack] = local.stepSequencer.noteLength[local.stepSequencer.markTrack];
    local.stepSequencer.muted[local.stepSequencer.currentTrack] = local.stepSequencer.muted[local.stepSequencer.markTrack];
    local.stepSequencer.velocity[local.stepSequencer.currentTrack] = local.stepSequencer.velocity[local.stepSequencer.markTrack];
    local.stepSequencer.fader[local.stepSequencer.currentTrack] = local.stepSequencer.fader[local.stepSequencer.markTrack];
    local.stepSequencer.offTime[local.stepSequencer.currentTrack] = local.stepSequencer.offTime[local.stepSequencer.markTrack];
    local.stepSequencer.noteOff[local.stepSequencer.currentTrack] = local.stepSequencer.noteOff[local.stepSequencer.markTrack];
    local.stepSequencer.shouldPlay[local.stepSequencer.currentTrack] = local.stepSequencer.shouldPlay[local.stepSequencer.markTrack];
    local.stepSequencer.transposable[local.stepSequencer.currentTrack] = local.stepSequencer.transposable[local.stepSequencer.markTrack];
    local.stepSequencer.pattern[local.stepSequencer.currentTrack] = local.stepSequencer.pattern[local.stepSequencer.markTrack];
    local.stepSequencer.dontPlay[local.stepSequencer.currentTrack] = local.stepSequencer.dontPlay[local.stepSequencer.markTrack];
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] = local.stepSequencer.controlParameter[local.stepSequencer.markTrack];
    local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = local.stepSequencer.lastControlValue[local.stepSequencer.markTrack];
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
        goDownState(STATE_STEP_SEQUENCER_MENU_NO);  // failed
        }
    }



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



static void setPots(uint16_t* potVals)
    {
    potVals[LEFT_POT] = pot[LEFT_POT];
    potVals[RIGHT_POT] = pot[RIGHT_POT];
    }
        
static uint8_t potChangedBy(uint16_t* potVals, uint8_t potNum, uint16_t amount)
    {
    int16_t val = potVals[potNum] - (int16_t)pot[potNum];
    if (val < 0) val = -val;
    return (val > amount);
    }


void stateStepSequencerMenuPattern()    
    {
    const char* menuItems[16] =     { PSTR("OOOO"), PSTR("OOO-"), PSTR("---O"), PSTR("OO-O"), PSTR("--O-"), PSTR("O---"), PSTR("-O--"), PSTR("OO--"), PSTR("--OO"), PSTR("O-O-"), PSTR("-O-O"), 
                                      PSTR("R1/8"),                      PSTR("R1/4"),                      PSTR("R1/2"),                      PSTR("R3/4"),                      PSTR("EXCL") };
    const uint8_t menuIndices[16] = { P1111,        P1110,        P0001,        P1101,        P0010,        P1000,        P0100,        P1100,        P0011,        P1010,        P0101,                    
                                      STEP_SEQUENCER_PATTERN_RANDOM_1_8, STEP_SEQUENCER_PATTERN_RANDOM_1_4, STEP_SEQUENCER_PATTERN_RANDOM_1_2, STEP_SEQUENCER_PATTERN_RANDOM_3_4, STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE };
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

void stateStepSequencerMenuLength()
    {
    // The values are OFF, 1, ..., 31
    // or OFF, 33, ..., 63 for 64-step sequences
    uint8_t result = doNumericalDisplay(
    	GET_MINIMUM_CUSTOM_LENGTH() - 1, 		// either 0 or 32, both of which will be displayed at "----"
    	GET_TRACK_FULL_LENGTH() - 1, 			// 16, 24, 32, or 64
    	GET_TRACK_CUSTOM_LENGTH() + GET_MINIMUM_CUSTOM_LENGTH() - 1, 		// The current custom length (0...32) plus either 0 or 32
    	true, 									// display the 0 or 32 as "----"
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
            data.slot.data.stepSequencer.format = GET_TRACK_FORMAT() | ((currentDisplay - (GET_MINIMUM_CUSTOM_LENGTH() - 1)) << 2);
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
    switch(data.slot.data.stepSequencer.repeat & 15)
        {
        case 0: local.stepSequencer.countdown = 255; break;
        case 1: local.stepSequencer.countdown = 0; break;
        case 2: local.stepSequencer.countdown = 1; break;
        case 3: local.stepSequencer.countdown = 2; break;
        case 4: local.stepSequencer.countdown = 3; break;
        case 5: local.stepSequencer.countdown = 4; break;
        case 6: local.stepSequencer.countdown = 5; break;
        case 7: local.stepSequencer.countdown = 7; break;
        case 8: local.stepSequencer.countdown = 8; break;
        case 9: local.stepSequencer.countdown = 11; break;
        case 10: local.stepSequencer.countdown = 15; break;
        case 11: local.stepSequencer.countdown = 17; break;
        case 12: local.stepSequencer.countdown = 23; break;
        case 13: local.stepSequencer.countdown = 31; break;
        case 14: local.stepSequencer.countdown = 63; break;
        case 15: local.stepSequencer.countdown = 127; break;
        }
    local.stepSequencer.countup = 255;
    }
    

// clears a note on a track no matter what
void clearNoteOnTrack(uint8_t track)
    {
    if (local.stepSequencer.noteOff[track] < NO_NOTE) 
        {
        uint8_t out = (local.stepSequencer.outMIDI[track] == MIDI_OUT_DEFAULT ? options.channelOut : local.stepSequencer.outMIDI[track]);
        if (out != NO_MIDI_OUT)
            {
            sendNoteOff(local.stepSequencer.noteOff[track], 127, out);
            }
        local.stepSequencer.noteOff[track] = NO_NOTE;
        }
    }

// This is a slightly modified version of the code in stateLoad()... I'd like to merge them but it'll
// have an impact on the Uno.  So for NOW...
void loadSequence(uint8_t slot)
    {
    uint8_t num = GET_NUM_TRACKS();
    for(uint8_t i = 0; i < num; i++)
        clearNoteOnTrack(i);

    if (getSlotType(slot) != slotTypeForApplication(STATE_STEP_SEQUENCER))
        {
        stopStepSequencer();
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

		local.stepSequencer.scheduleStop = 0;
        local.stepSequencer.goNextSequence = 0;
        local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;
        local.stepSequencer.currentTrack = 0;
        local.stepSequencer.currentEditPosition = 0;
        local.stepSequencer.markTrack = 0;
        local.stepSequencer.markPosition = 0;
                
        resetStepSequencerCountdown();
        }
    }
           




void resetTrack(uint8_t track)
    {
    uint8_t trackLen = GET_TRACK_FULL_LENGTH();
    memset(data.slot.data.stepSequencer.buffer + ((uint16_t)trackLen) * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
    local.stepSequencer.data[track] = STEP_SEQUENCER_DATA_NOTE;
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    local.stepSequencer.lastControlValue[track] = 0;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
    local.stepSequencer.pattern[track] = STEP_SEQUENCER_PATTERN_ALL;
    local.stepSequencer.transposable[track] = 1;
    local.stepSequencer.outMIDI[track] = MIDI_OUT_DEFAULT;  // default
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
    uint8_t skip = ((fullLen + 15) >> 4);      // that is, trackLen / 16

    clearScreen();
    
    // revise LASTTRACK to be just beyond the last track we'll draw
    //      (where TRACK is the first track we'll draw)     
        
    // this code is designed to allow the user to move down to about the middle of the screen,
    // at which point the cursor stays there and the screen scrolls instead.
    uint8_t firstTrack = local.stepSequencer.currentTrack;
    uint8_t fourskip =  4 / skip;
    if (firstTrack < fourskip)  
        firstTrack = 0;
    else firstTrack = firstTrack - fourskip + 1;
    
    uint8_t lastTrack = numTracks;          // lastTrack is 1+ the final track we'll be drawing
    uint8_t sixskip = 6 / skip;
    lastTrack = bound(lastTrack, 0, firstTrack + sixskip);

/*
	if (trackLen != fullLen)
		{
		debug(trackLen);
		debug(fullLen);
		debug(numTracks);
		debug(skip);
		debug(firstTrack);
		debug(lastTrack);
		debug(999);
		}
*/

    // Now we start drawing each of the tracks.  We will make blinky lights for beats or for the cursor
    // and will have solid lights or nothing for the notes or their absence.
        
    uint8_t y = 7;
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
                    ((t == local.stepSequencer.currentTrack) && (d == local.stepSequencer.currentEditPosition) 
                    && !local.stepSequencer.performanceMode) ||
                
                // draw mute or solo indicator.  Solo overrides mute.
                // So draw if solo is on but we're not it, OR if solo is turned off and we're muted
                ((xpos == 0 || xpos == 15) && shouldDrawMuted));

            if (vel || blink)
                {       
                // <8 <16 <24 <32 <40 <48 <56 <64
                if (d < 32)                             // for reasons only known to ATMEL, if I don't have this line on the UNO it's much bigger.  But it's irrelevant!
                    {
                    if (d < 16)
                        {
                        if (d < 8)
                            {
                            blinkOrSetPoint(led2, d, y, blink);
                            }
                        else // < 16
                            {
                            blinkOrSetPoint(led, d-8, y, blink);
                            }
                        }
                    else
                        {
                        if (d < 24)
                            {
                            blinkOrSetPoint(led2, d-16, y-1, blink);
                            }
                        else  // < 32
                            {
                            blinkOrSetPoint(led, d-24, y-1, blink);
                            }
                        }
                    }
                else
                    {
                    if (d < 48)
                        {
                        if (d < 40)
                            {
                            blinkOrSetPoint(led2, d - 32, y-2, blink);
                            }
                        else // < 48
                            {
                            blinkOrSetPoint(led, d-8 -32, y-2, blink);
                            }
                        }
                    else
                        {
                        if (d < 56)
                            {
                            blinkOrSetPoint(led2, d-16 - 32, y-3, blink);
                            }
                        else  // < 64
                            {
                            blinkOrSetPoint(led, d-24 - 32, y-3, blink);
                            }
                        }
                    }
                }
            }
        y -= skip;
        }
        
    // Next draw the track number
    drawRange(led2, 0, 1, 12, local.stepSequencer.currentTrack);

    // Next the MIDI channel
    drawMIDIChannel(
        (local.stepSequencer.outMIDI[local.stepSequencer.currentTrack] == MIDI_OUT_DEFAULT) ?
        options.channelOut : local.stepSequencer.outMIDI[local.stepSequencer.currentTrack]);

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
                
    // draw pattern position
    drawRange(led, 0, 1, 4, local.stepSequencer.countup & 3);

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
    uint8_t result;
    const char* menuItems[4] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES"), PSTR("64 NOTES")  };
    result = doMenuDisplay(menuItems, 4, STATE_NONE, 0, 1);
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
            data.slot.data.stepSequencer.format = currentDisplay;		// thus custom length will be 0
            setParseRawCC(false);
            memset(data.slot.data.stepSequencer.buffer, 0, STEP_SEQUENCER_BUFFER_SIZE);
            for(uint8_t i = 0; i < GET_NUM_TRACKS(); i++)
                {
                resetTrack(i);
                }
            stopStepSequencer();
            // stopStepSequencer() just set the countdown to something insane.  Set it to infinity
            local.stepSequencer.countdown = 255;
            local.stepSequencer.countup = 255;
            data.slot.data.stepSequencer.repeat = 0;  // forever
            local.stepSequencer.transpose = 0;
            local.stepSequencer.performanceMode = 0;
            local.stepSequencer.goNextSequence = 0;
            local.stepSequencer.scheduleStop = 0;
            //           local.stepSequencer.clearTrack = CLEAR_TRACK;
            local.stepSequencer.solo = 0;
            local.stepSequencer.currentTrack = 0;
            local.stepSequencer.currentEditPosition = 0;
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
        uint8_t out = (local.stepSequencer.outMIDI[local.stepSequencer.currentTrack] == MIDI_OUT_DEFAULT ? 
            options.channelOut : local.stepSequencer.outMIDI[local.stepSequencer.currentTrack]);
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
    uint8_t out = local.stepSequencer.outMIDI[track];
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
    sendAllSoundsOff();
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
        setPots(local.stepSequencer.pots);
        }

    immediateReturn = false;

    // always do this
    leftPotParameterEquivalent = false;

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
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
        if (local.stepSequencer.performanceMode)
            {
            //// SCHEDULE MUTE
            advanceMute(local.stepSequencer.currentTrack);
            }
        else if (local.stepSequencer.currentEditPosition >= 0)
            {
            //// ADD REST OR DATA
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
                // add a rest
                loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 0, 0);
                removeSuccessiveTies(local.stepSequencer.currentEditPosition, trackLen);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            }
        else
            {
            //// TOGGLE MUTE
            local.stepSequencer.muted[local.stepSequencer.currentTrack] = !local.stepSequencer.muted[local.stepSequencer.currentTrack];
            }       
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
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
                //// SCHEDULE SOLO
                advanceSolo();
                }
            else if (local.stepSequencer.currentEditPosition >= 0)
                {
                //// ADD A TIE

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
					}
				break;
				}
			}
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
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
        
            state = STATE_STEP_SEQUENCER_MENU;
            entry = true;
            }
        }
    else if (potUpdated[LEFT_POT])
        {
        //// CHANGE TRACK
        
        local.stepSequencer.currentTrack = ((pot[LEFT_POT] * numTracks) >> 10);         //  / 1024;
        local.stepSequencer.currentTrack = bound(local.stepSequencer.currentTrack, 0, numTracks);
        setParseRawCC(local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC);
//        local.stepSequencer.clearTrack = CLEAR_TRACK;
        }
    else if (potUpdated[RIGHT_POT])
        {
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
  
    else if (bypass)
        {
        // do nothing
        }
    


/*
// This is similar to the code later on which computes slop.
uint8_t sloppos = local.stepSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0);
if (sloppos >= trackLen) sloppos = 0;

if ((sloppos == 0) && (local.stepSequencer.clearTrack == DONT_CLEAR_TRACK))
{
local.stepSequencer.clearTrack = CLEAR_TRACK;
}
else if ((sloppos == 1) && (local.stepSequencer.clearTrack == DONT_CLEAR_TRACK_FIRST))
{
local.stepSequencer.clearTrack = DONT_CLEAR_TRACK;
}
*/

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
            uint8_t pos = (local.stepSequencer.currentEditPosition < 0 
                || (local.stepSequencer.performanceMode)
                ) ? 
                local.stepSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0) :
                local.stepSequencer.currentEditPosition;
            if (pos >= trackLen) pos = 0;
        
/*
  if ((local.stepSequencer.clearTrack == CLEAR_TRACK) && local.stepSequencer.performanceMode)
  {
  // clear track and notes
  memset(data.slot.data.stepSequencer.buffer + ((uint16_t)trackLen) * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
  clearNotesOnTracks(true);
  local.stepSequencer.clearTrack = DONT_CLEAR_TRACK;
  }
*/

/*      if (pos == 0)
        {
        local.stepSequencer.clearTrack = DONT_CLEAR_TRACK_FIRST;
        }
        else
        {
        local.stepSequencer.clearTrack = DONT_CLEAR_TRACK;
        }
*/
        
            // add a note
            loadBuffer(((uint16_t)trackLen) * local.stepSequencer.currentTrack + pos, note, velocity);
            removeSuccessiveTies(pos, trackLen);

            if (local.stepSequencer.currentEditPosition >= 0
                && !(local.stepSequencer.performanceMode))
                {
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
                }
            else 
                {
                local.stepSequencer.dontPlay[local.stepSequencer.currentTrack] = 1;
                if (!options.stepSequencerNoEcho)          // only play if we're echoing
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
        sendTrackNote(itemNumber + 128, itemValue, local.stepSequencer.currentTrack);
        }
    else if (newItem && (itemType == MIDI_AFTERTOUCH || itemType == MIDI_AFTERTOUCH_POLY))
        {
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
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    else if (newItem && (itemType == MIDI_CUSTOM_CONTROLLER))
        {
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

#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER

#define STEP_SEQUENCER_MENU_TYPE 6
#define STEP_SEQUENCER_MENU_PATTERN 7
#define STEP_SEQUENCER_MENU_TRANSPOSABLE 8
#define STEP_SEQUENCER_MENU_EDIT 9
//#define STEP_SEQUENCER_MENU_SEND_CLOCK 10
#define STEP_SEQUENCER_MENU_NO_ECHO 10
#define STEP_SEQUENCER_MENU_LENGTH 11
#define STEP_SEQUENCER_MENU_PERFORMANCE 12
#define STEP_SEQUENCER_MENU_SAVE 13
#define STEP_SEQUENCER_MENU_OPTIONS 14

#else

#define STEP_SEQUENCER_MENU_PATTERN 6
#define STEP_SEQUENCER_MENU_TRANSPOSABLE 7
#define STEP_SEQUENCER_MENU_EDIT 8
//#define STEP_SEQUENCER_MENU_SEND_CLOCK 9
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

#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
    const char* menuItems[15] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("RESET TRACK"),
        PSTR("NOTE LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("TYPE (TRACK)"),
        PSTR("PATTERN (TRACK)"),
        local.stepSequencer.transposable[local.stepSequencer.currentTrack] ? PSTR("NO TRANSPOSE (TRACK)") : PSTR("TRANSPOSE (TRACK)"),
        PSTR("EDIT"),
        options.stepSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        PSTR("LENGTH"),
        PSTR("PERFORMANCE"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 15, STATE_NONE, STATE_NONE, 1);
#else
    const char* menuItems[14] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("RESET TRACK"),
        PSTR("NOTE LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("PATTERN (TRACK)"),
        local.stepSequencer.transposable[local.stepSequencer.currentTrack] ? PSTR("NO TRANSPOSE (TRACK)") : PSTR("TRANSPOSE (TRACK)"),
        PSTR("EDIT"),
        options.stepSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        PSTR("LENGTH"),
        PSTR("PERFORMANCE"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 14, STATE_NONE, STATE_NONE, 1);
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
            state = STATE_STEP_SEQUENCER_PLAY;
            entry = true;
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
                    state = STATE_STEP_SEQUENCER_NOTE_LENGTH;                            
                    }
                break;
                case STEP_SEQUENCER_MENU_MIDI_OUT:
                    {
                    local.stepSequencer.backup = local.stepSequencer.outMIDI[local.stepSequencer.currentTrack];
                    state = STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT;
                    }
                break;
                case STEP_SEQUENCER_MENU_VELOCITY:
                    {
                    state = STATE_STEP_SEQUENCER_VELOCITY;
                    }
                break;
                case STEP_SEQUENCER_MENU_FADER:
                    {
                    state = STATE_STEP_SEQUENCER_FADER;
                    }
                break;
#ifdef INCLUDE_ADVANCED_STEP_SEQUENCER
                case STEP_SEQUENCER_MENU_TYPE:
                    {
                    state = STATE_STEP_SEQUENCER_MENU_TYPE;
                    }
                break;
#endif INCLUDE_ADVANCED_STEP_SEQUENCER
                case STEP_SEQUENCER_MENU_PATTERN:
                    {
                    immediateReturnState = STATE_STEP_SEQUENCER_PLAY;
                    state = STATE_STEP_SEQUENCER_MENU_PATTERN;
                    }
                break;
                case STEP_SEQUENCER_MENU_TRANSPOSABLE:
                    {
                    local.stepSequencer.transposable[local.stepSequencer.currentTrack] = !local.stepSequencer.transposable[local.stepSequencer.currentTrack];
                    }
                break;
                case STEP_SEQUENCER_MENU_EDIT:
                    {
                    state = STATE_STEP_SEQUENCER_MENU_EDIT;
                    }
                break;
                case STEP_SEQUENCER_MENU_NO_ECHO:
                    {
                    options.stepSequencerNoEcho = !options.stepSequencerNoEcho;
                    saveOptions();
                    }
                break;
                case STEP_SEQUENCER_MENU_LENGTH:
                    {
                    immediateReturnState = STATE_STEP_SEQUENCER_PLAY;
                    state = STATE_STEP_SEQUENCER_MENU_LENGTH;
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
                    state = STATE_STEP_SEQUENCER_SAVE;
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
            uint8_t out = (local.stepSequencer.outMIDI[track] == MIDI_OUT_DEFAULT ? options.channelOut : local.stepSequencer.outMIDI[track]);
            if (out != NO_MIDI_OUT)
                {
                sendNoteOff(local.stepSequencer.noteOff[track], 127, out);
                }
            local.stepSequencer.noteOff[track] = NO_NOTE;
            }
        }
    }

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

        uint8_t oldPlayPosition = local.stepSequencer.currentPlayPosition;
        local.stepSequencer.currentPlayPosition = incrementAndWrap(local.stepSequencer.currentPlayPosition, trackLen);
        
        // change scheduled mute?
        if (local.stepSequencer.performanceMode && local.stepSequencer.currentPlayPosition == 0)
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
            if (local.stepSequencer.solo == STEP_SEQUENCER_SOLO_ON_SCHEDULED)
                local.stepSequencer.solo = STEP_SEQUENCER_SOLO;
            else if (local.stepSequencer.solo == STEP_SEQUENCER_SOLO_OFF_SCHEDULED)
                local.stepSequencer.solo = STEP_SEQUENCER_NO_SOLO;

			if (local.stepSequencer.scheduleStop) 
				{
				stopStepSequencer(); 
				local.stepSequencer.goNextSequence = false;	// totally reset
				return; 
				}
					
            if (local.stepSequencer.goNextSequence || (oldPlayPosition != -1 && local.stepSequencer.countdown == 0))  // we're supposed to go
                {
                uint8_t nextSequence = (data.slot.data.stepSequencer.repeat >> 4);
                if (nextSequence == 0) // STOP
                    { 
					stopStepSequencer(); 
                    return; 
                    }
                else
                    { loadSequence(nextSequence - 1); }  // maybe this will work out of the box?  hmmm
                }
            else if (local.stepSequencer.countdown != 255)  // not forever
                {
                local.stepSequencer.countdown--;
                }                       
            }

        if (local.stepSequencer.currentPlayPosition == 0)
            {
            local.stepSequencer.countup++;
            }

// pick an exclusive random track
        uint8_t exclusiveTrack = 0;
        if (local.stepSequencer.currentPlayPosition == 0)
            {
            int trkcount = 0;
            for(uint8_t track = 0; track < numTracks; track++)
                {
                if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE)
                    {
                    if ((trkcount == 0) || (random() < RANDOM_MAX / (trkcount + 1)))  // this could work without the trakcount == 0 but I save a call to random() here 
                        {
                        exclusiveTrack = track;
                        }
                    trkcount++;
                    }
                }
            }
                        
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
 
            if (local.stepSequencer.currentPlayPosition == 0)
                {
                // pick a random track                          
                if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE)
                    {
                    local.stepSequencer.shouldPlay[track] = (track == exclusiveTrack);
                    }
                else if (local.stepSequencer.pattern[track] == STEP_SEQUENCER_PATTERN_RANDOM_3_4)
                    {
                    local.stepSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 4) * 3);
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
                                
            uint8_t shouldPlay = local.stepSequencer.shouldPlay[track] ;




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
                        (local.stepSequencer.outMIDI[track] == MIDI_OUT_DEFAULT ? options.channelOut : local.stepSequencer.outMIDI[track]));
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
    

#endif INCLUDE_STEP_SEQUENCER

