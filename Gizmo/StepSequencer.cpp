////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#ifdef INCLUDE_STEP_SEQUENCER



#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
GLOBAL uint8_t _trackLength[5] = {16, 24, 32, 48, 64};
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
GLOBAL uint8_t _numTracks[5] = {12, 8, 6, 4, 3};
#else
// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
GLOBAL uint8_t _trackLength[3] = {16, 24, 32};
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
GLOBAL uint8_t _numTracks[3] = {12, 8, 6};
#endif


void resetTrack(uint8_t track)
    {
    uint8_t trackLen = GET_TRACK_LENGTH();
    memset(data.slot.data.stepSequencer.buffer + trackLen * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
    local.stepSequencer.data[track] = STEP_SEQUENCER_DATA_NOTE;
    local.stepSequencer.lastControlValue[track] = 0;
#endif
    local.stepSequencer.outMIDI[track] = MIDI_OUT_DEFAULT;  // default
    local.stepSequencer.noteLength[track] = PLAY_LENGTH_USE_DEFAULT;
    local.stepSequencer.muted[track] = 0;
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
    return 
        (
        // We're muted if solo is on and we're not the current track
        (local.stepSequencer.solo && track != local.stepSequencer.currentTrack) ||
        // We're muted if solo is NOT on and our mute is on
        (!local.stepSequencer.solo && local.stepSequencer.muted[track]));
    }

// Draws the sequence with the given track length, number of tracks, and skip size
void drawStepSequencer(uint8_t trackLen, uint8_t numTracks, uint8_t skip)
    {
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
            uint16_t pos = (t * (uint16_t) trackLen + d) * 2;
            uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
            // check for tie
            if ((vel == 0) && (data.slot.data.stepSequencer.buffer[pos] == 1)
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
                && (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NOTE)
#endif
                )
                vel = 1;  // so we draw it
                
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
            else if ((local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE) &&
                ((vel & 127) | ((data.slot.data.stepSequencer.buffer[pos] & 127) << 7)) != 0)
                vel = 1;  // so we draw it
#endif

            if (shouldDrawMuted)
                vel = 0;
            uint8_t xpos = d - ((d >> 4) * 16);  // x position on screen
            uint8_t blink = (
                // draw play position cursor
                    ((local.stepSequencer.playState != PLAY_STATE_STOPPED) && 
                        ((d == local.stepSequencer.currentPlayPosition) ||   // main cursor
                        ((local.stepSequencer.currentEditPosition < 0 ) && (t == local.stepSequencer.currentTrack) && (abs(d - local.stepSequencer.currentPlayPosition) == 2)))) ||  // crosshatch
                // draw edit cursor
                ((t == local.stepSequencer.currentTrack) && (d == local.stepSequencer.currentEditPosition)) ||
                
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
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
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
#endif
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

#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
    // Are we locked?
    if (data.slot.data.stepSequencer.locked)
        setPoint(led, 4, 1);
#endif

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
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
    const char* menuItems[5] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES"), PSTR("48 NOTES"), PSTR("64 NOTES") };
    result = doMenuDisplay(menuItems, 5, STATE_NONE, 0, 1);
#else
    const char* menuItems[3] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES") };
    result = doMenuDisplay(menuItems, 3, STATE_NONE, 0, 1);
#endif
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
            data.slot.data.stepSequencer.format = currentDisplay;
            data.slot.data.stepSequencer.locked = 0;
            memset(data.slot.data.stepSequencer.buffer, 0, STEP_SEQUENCER_BUFFER_SIZE);
            for(uint8_t i = 0; i < GET_NUM_TRACKS(); i++)
                {
                resetTrack(i);
                }
            stopStepSequencer();
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
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
    if (local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE)
        return;
#endif

    p = incrementAndWrap(p, trackLen);
        
    uint8_t v = (trackLen * local.stepSequencer.currentTrack + p) * 2 ;
    while((data.slot.data.stepSequencer.buffer[v + 1]== 0) &&
        data.slot.data.stepSequencer.buffer[v] == 1)
        {
        data.slot.data.stepSequencer.buffer[v] = 0;  // make it a rest
        p = incrementAndWrap(p, trackLen);
        }
                        
    // we gotta do this because we just deleted some notes :-(
    sendAllNotesOff();
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



void loadBuffer(uint8_t position, uint8_t note, uint8_t velocity)
    {
    data.slot.data.stepSequencer.buffer[position * 2] = note;
    data.slot.data.stepSequencer.buffer[position * 2 + 1] = velocity;
    }



#ifdef INCLUDE_BUFFERED_CURSOR_X_POS

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

#else

// Uno version -- no buffer so we scroll right to -1 immediately.  But it saves us
// 20 bytes or so, which matters now.  ;-(
int16_t getNewCursorXPos(uint8_t trackLen)
    {
    // pot / 2 * (tracklen + 1) / 9 - 1
    // we do it this way because pot * (tracklen + 1) / 10 - 1 exceeds 64K
    return ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + 1)) >> 9)) - 1;  ///  / 1024 - 1;
    }
    
#endif


void resetStepSequencer()
    {
    local.stepSequencer.currentPlayPosition = GET_TRACK_LENGTH() - 1;
    }
        
void stopStepSequencer()
    {
    resetStepSequencer();
    local.stepSequencer.playState = PLAY_STATE_STOPPED;
    sendAllNotesOff();
    }


// Plays and records the sequence
void stateStepSequencerPlay()
    {
    // first we:
    // compute TRACKLEN, the length of the track
    // compute SKIP, the number of lines on the screen the track takes up
    uint8_t trackLen = GET_TRACK_LENGTH();
    uint8_t numTracks = GET_NUM_TRACKS();
    
    // this little function correctly maps:
    // 8 -> 1
    // 12 -> 1
    // 16 -> 1
    // 24 -> 2
    // 32 -> 2
    // 48 -> 3
    // 64 -> 4    
    uint8_t skip = ((trackLen + 15) >> 4);      // that is, trackLen / 16

    if (entry)
        {
        entry = false;
        local.stepSequencer.currentRightPot = -1;
        
/*
// The vaguaries of the compiler tell me that if I remove this
// code, the bytes go UP!!!!  A lot!  But I need to remove it, it's
// wrong.  So I'm putting in dummy code which fools the compiler just enough.

for(uint8_t i = 0; i < numTracks; i++)
{
//  (GET_TRACK_LENGTH() >> 8)   --->    0   but the compiler can't figure that out
local.stepSequencer.noteOff[i] = local.stepSequencer.noteOff[i + (GET_TRACK_LENGTH() >> 8)];
}
*/
        
        /*
          for(uint8_t i = 0; i < numTracks; i++)
          {
          local.stepSequencer.noteOff[i] = NO_NOTE;
          }
        */
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_STEP_SEQUENCER_SURE);
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        if (local.stepSequencer.currentEditPosition >= 0)
            {
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
            if (local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE)
                {
                // enter data
                uint8_t msb = (uint8_t) (local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] >> 7);
                uint8_t lsb = (uint8_t) (local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] & 127);
                loadBuffer(trackLen * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, msb, lsb);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            else
#endif
                {
                // add a rest
                loadBuffer(trackLen * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 0, 0);
                removeSuccessiveTies(local.stepSequencer.currentEditPosition, trackLen);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            }
        else    // toggle mute
            {
            local.stepSequencer.muted[local.stepSequencer.currentTrack] = !local.stepSequencer.muted[local.stepSequencer.currentTrack] ;
            }       
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        if (local.stepSequencer.currentEditPosition >= 0)
            {
            // add a tie.
            // We only permit ties if (1) the note before was NOT a rest and
            // (2) the note AFTER is NOT another tie (to prevent us from making a full line of ties)
            // These two positions (before and after) are p and p2 
            uint8_t p = local.stepSequencer.currentEditPosition - 1;
            if (p == 255) p = trackLen - 1;             // we wrapped around from 0
            uint8_t p2 = p + 2;
            if (p2 >= trackLen) p2 = 0;
            
            uint8_t v = (trackLen * local.stepSequencer.currentTrack + p) * 2 ;
            uint8_t v2 = (trackLen * local.stepSequencer.currentTrack + p2) * 2 ;
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
            if (local.stepSequencer.data[local.stepSequencer.currentTrack] != STEP_SEQUENCER_DATA_NOTE)
                {
                // erase data
                uint8_t msb = 0;
                uint8_t lsb = 0;
                loadBuffer(trackLen * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, msb, lsb);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            else
#endif
                // don't add if a rest precedes it or a tie is after it
                if (((data.slot.data.stepSequencer.buffer[v + 1] == 0) &&           // rest before
                        (data.slot.data.stepSequencer.buffer[v] == 0)) ||
                        ((data.slot.data.stepSequencer.buffer[v2 + 1] == 1) &&          // tie after
                        (data.slot.data.stepSequencer.buffer[v2] == 0)))
                    {
                    // do nothing
                    }
                else
                    {
                    loadBuffer(trackLen * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 1, 0);
                    local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
                    local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                    }
            }
        else 
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
            if (!data.slot.data.stepSequencer.locked)
#endif
                {
                // do a "light" clear, not a full reset
                memset(data.slot.data.stepSequencer.buffer + trackLen * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
                }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
        if (options.stepSequencerSendClock)
            {
            // we always stop the clock just in case, even if we're immediately restarting it
            stopClock(true);
            }
#endif
        switch(local.stepSequencer.playState)
            {
            case PLAY_STATE_STOPPED:
                {
                local.stepSequencer.playState = PLAY_STATE_WAITING;
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
                if (options.stepSequencerSendClock)
                    {
                    // Possible bug condition:
                    // The MIDI spec says that there "should" be at least 1 ms between
                    // starting the clock and the first clock pulse.  I don't know if that
                    // will happen here consistently.
                    startClock(true);
                    }
#endif                
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
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        state = STATE_STEP_SEQUENCER_MENU;
        entry = true;
        }
    else if (potUpdated[LEFT_POT])
        {
        local.stepSequencer.currentTrack = ((pot[LEFT_POT] * numTracks) >> 10);         //  / 1024;
        local.stepSequencer.currentTrack = bound(local.stepSequencer.currentTrack, 0, numTracks);
#ifdef INCLUDE_PROVIDE_RAW_CC
        setParseRawCC(local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC);
#endif
        }
    else if (potUpdated[RIGHT_POT])
        {
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
    else if (newItem && (itemType == MIDI_NOTE_ON) //// there is a note played
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
        && !data.slot.data.stepSequencer.locked 
        && local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NOTE
#endif
        )
        {
        TOGGLE_IN_LED();
        uint8_t note = itemNumber;
        uint8_t velocity = itemValue;
                
        // here we're trying to provide some slop so the user can press the note early.
        // we basically are rounding up or down to the nearest note
        uint8_t pos = local.stepSequencer.currentEditPosition >= 0 ? 
            local.stepSequencer.currentEditPosition :
            local.stepSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0);
        if (pos >= trackLen) pos = 0;
        if (pos < 0) pos = trackLen - 1;

        // add a note
        loadBuffer(trackLen * local.stepSequencer.currentTrack + pos, note, velocity);
        removeSuccessiveTies(pos, trackLen);

        if (local.stepSequencer.currentEditPosition >= 0)
            {
            local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
            }
        else 
            {
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
            local.stepSequencer.dontPlay[local.stepSequencer.currentTrack] = 1;
            if (!options.stepSequencerNoEcho)          // only play if we're echoing
                {
                sendTrackNote(note, velocity, local.stepSequencer.currentTrack);
                }
#endif
            }
                        
        local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
        }
    else if (newItem && (itemType == MIDI_NOTE_OFF)
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
        && !data.slot.data.stepSequencer.locked  
        && local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NOTE
#endif
        )
        {
        sendTrackNote(itemNumber + 128, itemValue, local.stepSequencer.currentTrack);
        }
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
    else if (newItem && (itemType == MIDI_AFTERTOUCH || itemType == MIDI_AFTERTOUCH_POLY))
        {
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_AFTERTOUCH)
            {
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = (itemValue << 7) + 1;
            }
        }
    else if (newItem && (itemType == MIDI_PROGRAM_CHANGE))
        {
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_PC)
            {
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = (itemNumber << 7) + 1;
            }
        }
    else if (newItem && (itemType == MIDI_PITCH_BEND))
        {
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_BEND)
            {
            uint16_t v = itemValue;
            if (v == MAX_CONTROL_VALUE)
                v = MAX_CONTROL_VALUE - 1;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
        }
    else if (newItem && (itemType == MIDI_CC_7_BIT))
        {
        if ((local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_CC ||
                local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_14_BIT_CC) &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = (itemValue << 7) + 1;
            }
        }
    else if (newItem && (itemType == MIDI_CC_14_BIT))
        {
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_14_BIT_CC &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            uint16_t v = itemValue;
            if (v == MAX_CONTROL_VALUE)
                v = MAX_CONTROL_VALUE - 1;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
        }
    else if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_NRPN &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            uint16_t v = itemValue;
            if (v == MAX_CONTROL_VALUE)
                v = MAX_CONTROL_VALUE - 1;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
        }
    else if (newItem && (itemType == MIDI_RPN_14_BIT))
        {
        if (local.stepSequencer.data[local.stepSequencer.currentTrack] == STEP_SEQUENCER_DATA_RPN &&
            local.stepSequencer.controlParameter[local.stepSequencer.currentTrack] == itemNumber)
            {
            uint16_t v = itemValue;
            if (v == MAX_CONTROL_VALUE)
                v = MAX_CONTROL_VALUE - 1;
            local.stepSequencer.lastControlValue[local.stepSequencer.currentTrack] = v + 1;
            }
        }
#endif
       
    playStepSequencer();
    if (updateDisplay)
        {
        drawStepSequencer(trackLen, numTracks, skip);
        }
    }


// Various choices in the menu
#define STEP_SEQUENCER_MENU_SOLO 0
#define STEP_SEQUENCER_MENU_LENGTH 1
#define STEP_SEQUENCER_MENU_MIDI_OUT 2
#define STEP_SEQUENCER_MENU_VELOCITY 3
#define STEP_SEQUENCER_MENU_FADER 4

#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
#define STEP_SEQUENCER_MENU_RESET 5
#define STEP_SEQUENCER_MENU_SAVE 6
#define STEP_SEQUENCER_MENU_TYPE 7
#define STEP_SEQUENCER_MENU_SEND_CLOCK 8
#define STEP_SEQUENCER_MENU_LOCK 9
#define STEP_SEQUENCER_MENU_NO_ECHO 10
#define STEP_SEQUENCER_MENU_OPTIONS 11
#else
#define STEP_SEQUENCER_MENU_RESET 5
#define STEP_SEQUENCER_MENU_SAVE 6
#define STEP_SEQUENCER_MENU_OPTIONS 7
#endif



// Gives other options
void stateStepSequencerMenu()
    {
    uint8_t result;

#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
    const char* menuItems[12] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("RESET TRACK"),
        PSTR("SAVE"), 
        PSTR("TYPE (TRACK)"),
        options.stepSequencerSendClock ? PSTR("NO CLOCK CONTROL") : PSTR("CLOCK CONTROL"),
        data.slot.data.stepSequencer.locked ? PSTR("UNLOCK") : PSTR("LOCK"),
        options.stepSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 12, STATE_NONE, STATE_NONE, 1);
#else
    const char* menuItems[8] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("RESET TRACK"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 8, STATE_NONE, STATE_NONE, 1);
#endif

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
                case STEP_SEQUENCER_MENU_LENGTH:
                    {
                    state = STATE_STEP_SEQUENCER_LENGTH;                            
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
                case STEP_SEQUENCER_MENU_RESET:
                    {
                    resetTrack(local.stepSequencer.currentTrack);
                    break;
                    }
                case STEP_SEQUENCER_MENU_SAVE:
                    {
                    state = STATE_STEP_SEQUENCER_SAVE;
                    }
                break;
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
                case STEP_SEQUENCER_MENU_TYPE:
                    {
                    state = STATE_STEP_SEQUENCER_MENU_TYPE;
                    }
                break;
                case STEP_SEQUENCER_MENU_SEND_CLOCK:
                    {
                    options.stepSequencerSendClock = !options.stepSequencerSendClock;
                    if (options.stepSequencerSendClock)
                        {
                        // the logic here is that if we are suddenly NOW sending the clock,
                        // we should stop the clock so we're not just constantly sending pulses
                        // if we're currently playing
                        stopClock(true);
                        }
                    else
                        {
                        // the logic here is that if we are suddenly no longer sending the
                        // clock, we don't want to leave the clock OFF because the user
                        // has no easy way of turning it on again!
                        continueClock(true);
                        }
                    saveOptions();
                    }
                break;
                case STEP_SEQUENCER_MENU_LOCK:
                    {
                    data.slot.data.stepSequencer.locked = !data.slot.data.stepSequencer.locked;
                    }
                break;
                case STEP_SEQUENCER_MENU_NO_ECHO:
                    {
                    options.stepSequencerNoEcho = !options.stepSequencerNoEcho;
                    saveOptions();
                    }
                break;
#endif
                case STEP_SEQUENCER_MENU_OPTIONS:
                    {
                    optionsReturnState = STATE_STEP_SEQUENCER_MENU;
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
    
    
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
// Gives other options
void stateStepSequencerMenuType()
    {
    uint8_t result;
    
    const char* menuItems[8] = { PSTR("NOTE"), PSTR("CC"), PSTR("14 BIT CC"), PSTR("NRPN"), PSTR("RPN"), 
                                 PSTR("PC"), PSTR("BEND"), PSTR("AFTERTOUCH") };
    result = doMenuDisplay(menuItems, 8, STATE_OPTIONS_TEMPO, optionsReturnState, 1);

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
    uint8_t result = doNumericalDisplay(0, max, local.stepSequencer.controlParameter[local.stepSequencer.currentTrack], 0, OTHER_NONE);
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
#endif    


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
            (clearEvenIfNoteNotFinished || (currentTime >= local.stepSequencer.offTime[track])) &&  // it's time to clear the note
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

        local.stepSequencer.currentPlayPosition = incrementAndWrap(local.stepSequencer.currentPlayPosition, trackLen);

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

#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
            if (local.stepSequencer.data[track] != STEP_SEQUENCER_DATA_NOTE)
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
#endif                        
                if (vel == 0 && note == 1)  // tie
                    {
                    local.stepSequencer.offTime[track] = currentTime + (div100(notePulseRate * getMicrosecsPerPulse() * noteLength));
                    }
                else if (vel == 0 && note == 0) // rest
                    {
                    local.stepSequencer.noteOff[track] = NO_NOTE;
                    }
                else if (vel != 0 
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
                    && !local.stepSequencer.dontPlay[track]  // not a rest or tie
#endif
                    )            
                    {
                    if (local.stepSequencer.velocity[track] != STEP_SEQUENCER_NO_OVERRIDE_VELOCITY)
                        vel = local.stepSequencer.velocity[track];

// The Atmel compiler optimizer is insane.  If I replace the following two lines with just this:
//                              if (!shouldMuteTrack(track))
// ... then I go UP by 8 bytes!
                    if ((!local.stepSequencer.solo && !local.stepSequencer.muted[track]) ||                 // If solo is off AND we're not muted......  OR...
                        (local.stepSequencer.solo && track == local.stepSequencer.currentTrack))        // Solo is turned on we're the current track regardless of mute
                        {
                        uint16_t newvel = (vel * (uint16_t)(local.stepSequencer.fader[track])) >> 5;
                        if (newvel > 127) 
                            newvel = 127;
                        sendTrackNote(note, (uint8_t)newvel, track);         // >> 5 is / FADER_IDENTITY_VALUE, that is, / 32
                        }
                    local.stepSequencer.offTime[track] = currentTime + (div100(notePulseRate * getMicrosecsPerPulse() * noteLength));
                    local.stepSequencer.noteOff[track] = note;
                    }
            }
#ifdef INCLUDE_EXTENDED_STEP_SEQUENCER
        // clear the dontPlay flags
        memset(local.stepSequencer.dontPlay, 0, numTracks);
#endif
        }

    // click track
    doClick();
    }
        
        
#endif

