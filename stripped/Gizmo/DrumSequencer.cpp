////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#ifdef INCLUDE_DRUM_SEQUENCER



// The data buffer is laid out as follows:
// 1. TRACK DATA[tracks]
// 2. GROUP DATA[groups]
// 3. GROUP TRACK DATA[groups][tracks]
// 4. NOTE DATA[groups][tracks][notes]


//// TRACK DATA
// track byte 0 and track byte 1
#define TRACK_OFFSET                                                    (0)                     // I don't think we need to make this uint16_t but it might not hurt
#define GET_TRACK0(track)                                       (data.slot.data.drumSequencer.data[TRACK_OFFSET + track * 2 + 0])
#define GET_TRACK1(track)                                       (data.slot.data.drumSequencer.data[TRACK_OFFSET + track * 2 + 1])
#define SET_TRACK0(track, val)                                  (data.slot.data.drumSequencer.data[TRACK_OFFSET + track * 2 + 0] = (val))
#define SET_TRACK1(track, val)                                  (data.slot.data.drumSequencer.data[TRACK_OFFSET + track * 2 + 1] = (val))
#define TRACK_DATALEN                                                   (local.drumSequencer.numTracks * 2)


//// GROUP DATA
// group byte
#define GROUP_OFFSET                                                    (TRACK_OFFSET + TRACK_DATALEN)                  // I don't think we need to make this uint16_t but it might not hurt
#define GET_GROUP(group)                                        (data.slot.data.drumSequencer.data[GROUP_OFFSET + group])
#define SET_GROUP(group, val)                                   (data.slot.data.drumSequencer.data[GROUP_OFFSET + group] = (val))
#define GROUP_DATALEN                                                   (local.drumSequencer.numGroups)


//// GROUP-TRACK DATA
// group-track data, 2 at a time (4 bits each)
#define GROUPTRACK_OFFSET                                               (GROUP_OFFSET + GROUP_DATALEN)                  // I don't think we need to make this uint16_t but it might not hurt
#define HALFTRACKS                                                              (local.drumSequencer.numTracks >> 1)                    //    / 2
#define GET_GROUPTRACK(group, trackpair)        (data.slot.data.drumSequencer.data[GROUPTRACK_OFFSET + HALFTRACKS * group + trackpair])
#define SET_GROUPTRACK(group, trackpair, val)   (data.slot.data.drumSequencer.data[GROUPTRACK_OFFSET + HALFTRACKS * group + trackpair] = (val))
#define GROUPTRACK_DATALEN                                              (HALFTRACKS * local.drumSequencer.numGroups)


//// NOTE DATA
// note bits, 8 at a time
#define NOTE_OFFSET                                                     ((GROUPTRACK_OFFSET + (uint16_t) GROUPTRACK_DATALEN))  // at this point we're definitely in the uint16_t range
#define GET_NOTE_OFFSET(group, track)                   (NOTEOFFSET + (local.drumSequencer.numTracks * (uint16_t) local.drumSequencer.numNotes) * group + local.drumSequencer.numNotes * (uint16_t) track])
#define GET_NOTES(group, track, notes)                  (data.slot.data.drumSequencer.data[GET_NOTE_OFFSET(group, track) + notes])
#define SET_NOTES(group, track, notes, val)             (data.slot.data.drumSequencer.data[GET_NOTE_OFFSET(group, track) + notes] = (val))
#define NOTE_DATALEN                                                    (local.drumSequencer.numGroups * (uint16_t) local.drumSequencer.numTracks * (uint16_t) local.drumSequencer.numNotes)



uint8_t getNote(uint8_t group, uint8_t track, uint8_t note)
    {
    uint8_t div = note >> 3;                                                // /8
    uint8_t rem = note & 7;                                                 // Remainder
    uint8_t notes = GET_NOTES(group, track, div);
    return (notes >> rem) & 1;
    }

void setNote(uint8_t group, uint8_t track, uint8_t note, uint8_t val)
    {
    uint8_t div = note >> 3;                                                // /8
    uint8_t rem = note & 7;                                                 // Remainder
    uint8_t notes = GET_NOTES(group, track, div);
    if (val)
        {
        // set the bit
        notes |= (((unsigned char) 1) << rem);
        }
    else
        {
        // clear the bit
        notes &= ~(((unsigned char) 1) << rem);
        }
    SET_NOTES(group, track, div, notes);
    }
 
void setNote(uint8_t group, uint8_t track, uint8_t note)
    {
    setNote(group, track, note, 1);
    }

void clearNote(uint8_t group, uint8_t track, uint8_t note)
    {
    setNote(group, track, note, 0);
    }
       
void clearNotes(uint8_t group, uint8 track)
    {
    memset(&(data.slot.data.drumSequencer.data[GET_NOTE_OFFSET(group, track)]), 0, (local.drumSequencer.numNotes >> 3));  // / 8
    }
        
uint8_t getPattern(uint8_t group, uint8_t track)
    {
    uint8_t trackpair = track >> 1;                                         // /2
    uint8_t gt = GET_GROUPTRACK(group, trackpair);
    if (track & 1)
        {
        // high bits
        return (gt >> 4);
        }
    else
        {
        // low bits
        return (gt & 0x0F);
        }
    }
        
void setPattern(uint8_t group, uint8_t track, uint8_t pattern)
    {
    uint8_t trackpair = track >> 1;                                         // /2
    uint8_t gt = GET_GROUPTRACK(group, trackpair);
    if (track & 1)
        {
        // high bits
        gt = (gt & 0x0F) | (pattern << 4);
        }
    else
        {
        // low bits
        gt = (gt & 0xF0) | pattern;     
        }
    SET_GROUPTRACK(group, trackpair, gt);
    }
        
        
uint8_t getGroupLength(uint8_t group)
    {
    uint8_t gt = GET_GROUP(group);
    // group length is the high 4 bits
    return (gt >> 4);
    }

uint8_t getActualGroupLength(uint8_t group)
    {
    uint8_t gl = getGroupLength(group);
    uint8_t nn = local.drumSequencer.numNotes;
    if (gl == GROUP_LENGTH_DEFAULT) return nn;
    else return (uint8_t)((nn * (uint16_t)gl) >> 4);                // /16
    }

void setGroupLength(uint8_t group, uint8_t groupLength)
    {
    uint8_t gt = GET_GROUP(group);
    // group length is the high 4 bits
    gt = (gt & 0x0F) | (groupLength << 4);
    SET_GROUP(group, gt);
    }

uint8_t getNoteSpeed(uint8_t group)
    {
    uint8_t gt = GET_GROUP(group);
    // note speed is the low 4 bits
    return (gt & 0x0F);
    }

void setNoteSpeed(uint8_t group, uint8_t noteSpeed)
    {
    uint8_t gt = GET_GROUP(group);
    // note speed is the low 4 bits
    gt = (gt & 0xF0) | noteSpeed;
    SET_GROUP(group, gt);
    }

uint8_t getMIDIChannel(uint8_t track)
    {
    uint8_t gt = GET_TRACK0(track);
    // MIDI Channel is the high 5 bits of byte 0
    return (gt >> 3);
    }

void setMIDIChannel(uint8_t track, uint8_t channel)
    {
    uint8_t gt = GET_TRACK0(track);
    // MIDI Channel is the high 5 bits of byte 0
    gt = (gt & 7) | (channel << 3);
    SET_TRACK0(track, gt);
    }

uint8_t getNoteVelocity(uint8_t track)
    {
    uint8_t gt = GET_TRACK0(track);
    // Note Velocity is the low 3 bits of byte 0
    return (gt & 7);
    }

void setNoteVelocity(uint8_t track, uint8_t velocity)
    {
    uint8_t gt = GET_TRACK0(track);
    // Note Velocity is the low 3 bits of byte 0
    gt = (gt & 0xF8) | velocity;                                            // 0xF8 is 11111000
    SET_TRACK0(track, gt);
    }

uint8_t getNotePitch(uint8_t track)
    {
    uint8_t gt = GET_TRACK1(track);
    // Note Pitch is the high 7 bits of byte 1
    return (gt >> 1);
    }

void setNotePitch(uint8_t track, uint8_t pitch)
    {
    uint8_t gt = GET_TRACK1(track);
    // Note Pitch is the high 7 bits of byte 1
    gt = (gt & 1) | (pitch << 1);
    SET_TRACK1(track, gt);
    }

uint8_t getMute(uint8_t track)
    {
    uint8_t gt = GET_TRACK1(track);
    // Mute is the low bit of byte 1
    return (gt & 1);
    }

void setMute(uint8_t track, uint8_t mute)
    {
    uint8_t gt = GET_TRACK1(track);
    // Mute is the low bit of byte 1
    gt = (gt & 0xFE) | mute;                                                        // 0xFE is 11111110
    SET_TRACK1(track, gt);
    }

void numFormatNotes(uint8_t format)
    {
    const uint8_t notes[NUM_FORMATS] = { 8, 8, 16, 16, 32, 32, 64, 64 };
    return notes[format];
    } 

void numFormatTracks(uint8_t format)
    {
    const uint8_t tracks[NUM_FORMATS] = { 16, 12, 16, 12, 16, 12, 16, 12 };
    return tracks[format];
    } 

void numFormatGroups(uint8_t format)
    {
    // Note that we return 15 even though we could fit 17
    const uint8_t groups[NUM_FORMATS] = { 13, 15, 8, 11, 4, 6, 2, 3 };
    return groups[format];
    } 


void goNextTransition()
    {
    if (local.drumSequencer.transitionCountdown == 0)
        {
        local.drumSequencer.currentTransition++;
        uint8_t repeat = 0;
        uint8_t group =  0;
                
        // are we at the end?
        if (local.drumSequencer.currentTransition > NUM_TRANSITIONS ||
                (local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition] == TRANSITION_GROUP_OTHER &&
                local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition] == TRANSITION_OTHER_END))
            {
            // This should never happen.  We can't have END as the first START.  This will be interpreted
            // as repeating group 1 in a loop
            if (local.drumSequencer.currentTransition == 0)
                {
                repeat = TRANSITION_REPEAT_LOOP;
                group = 0;
                }
            else
                {
                if (local.drumSequencer.sequenceCountdown == 0)
                    {
                    uint8_t nextSequence = data.slot.data.stepSequencer.nextSequence;
                    if (nextSequence == NEXT_SEQUENCE_END)  // STOP
                        { stopStepSequencer(); return; }
                    else            // maybe this will work out of the box?  hmmm
                        { 
                        loadDrumSequence(nextSequence - 1); 
                        resetDrumSequencerTransitionCountdown();                        // FIXME -- do I need this?
                        }  
                    }
                else if (local.drumSequencer.sequenceCountdown != 255)  // loop forever
                    {
                    local.drumSequencer.sequenceCountdown--;
                    resetDrumSequencerTransitionCountdown();
                    }       
                }
            }                       
        // are we picking a group at random?
        else if (local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition] == TRANSITION_GROUP_OTHER &&
            local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition] != TRANSITION_OTHER_END)
            {
            // Groups are going to be either 1-2, 1-3, or 1-4
            uint8_t grouptype = div5(local.drumSequencer.transitionRepeat);
            // repeats are LOOP, 1, 2, 3, or 4
            repeat = DIV5_REMAINDER(grouptype, local.drumSequencer.transitionRepeat);
            // Pick a group
            group = random(0, grouptype + 2);
            }
        // just grab the group and repeat
        else
            {
            repeat = local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition];
            group =  local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition];
            }
        }
    else if (local.drumSequencer.transitionCountdown != 255)  // loop forever
        {
        local.drumSequencer.transitionCountdown--;
        }       
    }
        


void initDrumSequencer(uint8_t format)
    {
    data.slot.type = SLOT_TYPE_DRUM_SEQUENCER;              // meh, no reason to set this but...

    // set these first so the later computations work right
    local.drumSequencer.format = format;
    local.drumSequencer.numGroups = numFormatGroups(local.drumSequencer.format);
    local.drumSequencer.numTracks = numFormatTracks(local.drumSequencer.format);
    local.drumSequencer.numNotes = numFormatNotes(local.drumSequencer.format);
    local.drumSequencer.currentGroup = 0;
    local.drumSequencer.currentTrack = 0;
    local.drumSequencer.currentTransition = 0;
    local.drumSequencer.currentEditPosition = 0;
    local.drumSequencer.currentPlayPosition = 0;
    local.drumSequencer.repeatSequence = SEQUENCE_REPEAT_LOOP;
    local.drumSequencer.nextSequence = NEXT_SEQUENCE_END;

    // All notes are set to 0 (off)
    // group length 0 is GROUP_LENGTH_DEFAULT
    // note speed 0 is NOTE_SPEED_DEFAULT
    memset(data.slot.data.drumSequencer.data, 0, DATA_LENGTH);
        
    // MIDI channel 17 is MIDI_OUT_DEFAULT
    // note velocity should be MAX_NOTE_VELOCITY
    // note pitch is set to INITIAL_NOTE_PITCH
    // mute is 0
    // pattern OOOO is 15 for all group/track combos
    for(uint8_t i = 0; i < numFormatTracks(format); i++)
        {
        setMIDIChannel(i, MIDI_OUT_DEFAULT);
        setNoteVelocity(i, MAX_NOTE_VELOCITY);
        setNotePitch(i, INITIAL_NOTE_PITCH);
        for(uint8_t j = 0; j < numFormatGroups(group); j++)
            {
            setPattern(j, i, DRUM_SEQUENCER_PATTERN_ALL);
            }
        }
        
    // The initial transition pattern is:
    // 1 LOOP
    // 2 LOOP
    // 3 LOOP
    // ....
    // 15 LOOP
    // END LOOP
    // ...
    for(uint8_t i = 0; i < NUM_TRANSITIONS; i++)
        {
        if (i < MAX_GROUPS)
            {
            local.drumSequencer.transitionGroup[i] = (i + TRANSITION_GROUP_FIRST);
            }
        else
            {
            local.drumSequencer.transitionGroup[i] = TRANSITION_GROUP_END;
            }
        }
    memset(local.drumSequencer.transitionRepeat, TRANSITION_REPEAT_LOOP, NUM_TRANSITIONS);
    }



// Reformats the sequence as requested by the user
void stateDrumSequencerFormat()
    {
    uint8_t result;
    const char* menuItems[8] = {  PSTR("8/12/15"), PSTR("16/12/11"), PSTR("32/12/6"), PSTR("64/12/3"), PSTR("8/16/13"), PSTR("16/16/8"), PSTR("32/16/4"), PSTR("64/16/2") };   /// PSTR("8/6/32"), PSTR("16/32/3"), PSTR("64/32/1") 
    result = doMenuDisplay(menuItems, 8, STATE_NONE, 0, 1);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            initDrumSequencer(currentDisplay);
            setParseRawCC(false);                               // do we need this?
            stopDrumSequencer();
            // stopDrumSequencer() just set the transitionCountdown to something insane.  Set it to infinity
            local.drumSequencer.transitionCountdown = 255;
            local.drumSequencer.sequenceCountdown = 255;
            local.drumSequencer.patternCountup = 255;
            local.drumSequencer.performanceMode = 0;
            local.drumSequencer.solo = 0;
            setNotePulseRate(options.noteSpeedType);
            goDownState(STATE_DRUM_SEQUENCER_PLAY);
            }
        break;
        case MENU_CANCELLED:
            {
            goDownState(STATE_DRUM_SEQUENCER);
            }
        break;
        }
    }


void packDrumSequenceData()
    {
    for(uint8_t i = 0; i < NUM_TRANSITIONS; i++)
        {
        data.slot.data.drumSequencer.transition[i] = (local.drumSequencer.transitionGroup[i]) | (local.drumSequencer.transitionRepeat[i] << 4);
        }
    data.slot.data.drumSequencer.repeat = (local.drumSequencer.nextSequence | (local.drumSequencer.repeatSequence << 4));
    data.slot.data.drumSequencer.format = local.drumSequencer.format;       // wasting 4 bits for now
    }

void unpackDrumSequenceData(struct _drumSequencer *seq)
    {
    local.drumSequencer.format = (data.slot.data.drumSequencer.format & 0x0F);
    initDrumSequencer(local.drumSequencer.format);
        
    memcopy(data.slot.data.drumSequencer.data, data.slot.data.drumSequencer.data, DATA_LENGTH);
    for(uint8_t i = 0; i < NUM_TRANSITIONS; i++)
        {
        local.drumSequencer.transitionGroup[i] = (data.slot.data.drumSequencer.transition[i] & 0x0F);
        local.drumSequencer.transitionRepeat[i] = (data.slot.data.drumSequencer.transition[i] >> 4);
        }
    local.drumSequencer.nextSequence = (data.slot.data.drumSequencer.repeat & 0x0F);
    local.drumSequencer.repeatSequence = (data.slot.data.drumSequencer.repeat >> 4);
    local.drumSequencer.numGroups = numFormatGroups(local.drumSequencer.format);
    local.drumSequencer.numTracks = numFormatTracks(local.drumSequencer.format);
    local.drumSequencer.numNotes = numFormatNotes(local.drumSequencer.format);
    }


void advanceMute(uint8_t track)
    {
    switch(local.drumSequencer.muted[track])
        {
        case DRUM_SEQUENCER_NOT_MUTED:
            {
            local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTE_ON_SCHEDULED_ONCE;
            break;
            }
        case DRUM_SEQUENCER_MUTED:
            {
            local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE;
            break;
            }
        case DRUM_SEQUENCER_MUTE_ON_SCHEDULED:
            {
            local.drumSequencer.muted[track] = DRUM_SEQUENCER_NOT_MUTED;
            break;
            }
        case DRUM_SEQUENCER_MUTE_OFF_SCHEDULED:
            {
            local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTED;
            break;
            }
        case DRUM_SEQUENCER_MUTE_ON_SCHEDULED_ONCE:
            {
            local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTE_ON_SCHEDULED;
            break;
            }
        case DRUM_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE:
            {
            local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTE_OFF_SCHEDULED;
            break;
            }
        }
    }

void advanceSolo()
    {
    switch(local.drumSequencer.solo)
        {
        case DRUM_SEQUENCER_NO_SOLO:
            {
            local.drumSequencer.solo = DRUM_SEQUENCER_SOLO_ON_SCHEDULED;
            break;
            }
        case DRUM_SEQUENCER_SOLO:
            {
            local.drumSequencer.solo = DRUM_SEQUENCER_SOLO_OFF_SCHEDULED;
            break;
            }
        case DRUM_SEQUENCER_SOLO_ON_SCHEDULED:
            {
            local.drumSequencer.solo = DRUM_SEQUENCER_NO_SOLO;
            break;
            }
        case DRUM_SEQUENCER_SOLO_OFF_SCHEDULED:
            {
            local.drumSequencer.solo = DRUM_SEQUENCER_SOLO;
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





void stateDrumSequencerMenuPattern()    
    {
    const char* menuItems[16] =     { PSTR("OOOO"), PSTR("OOO-"), PSTR("---O"), PSTR("OO-O"), PSTR("--O-"), PSTR("O---"), PSTR("-O--"), PSTR("OO--"), PSTR("--OO"), PSTR("O-O-"), PSTR("-O-O"), 
                                      PSTR("R1/8"),                      PSTR("R1/4"),                      PSTR("R1/2"),                      PSTR("R3/4"),                      PSTR("EXCL") };
    const uint8_t menuIndices[16] = { P1111,        P1110,        P0001,        P1101,        P0010,        P1000,        P0100,        P1100,        P0011,        P1010,        P0101,                    
                                      DRUM_SEQUENCER_PATTERN_RANDOM_1_8, DRUM_SEQUENCER_PATTERN_RANDOM_1_4, DRUM_SEQUENCER_PATTERN_RANDOM_1_2, DRUM_SEQUENCER_PATTERN_RANDOM_3_4, DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE };
    if (entry)
        {
        // find the pattern
        for(uint8_t i = 0 ; i < 16; i++)
            {
            if (menuIndices[i] == getPattern(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack))
                {
                defaultMenuValue = i;
                break;
                }
            }
        }
        
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
    playDrumSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            setPattern(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, menuIndices[currentDisplay]);
            goUpState(immediateReturnState);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    }



void resetDrumSequencerTransitionCountdown()
    {
    // This is forever, 1 time, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 32, 64 times 
    switch(local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition])
        {
        case 0: local.drumSequencer.transitionCountdown = 255; break;
        case 1: local.drumSequencer.transitionCountdown = 0; break;
        case 2: local.drumSequencer.transitionCountdown = 1; break;
        case 3: local.drumSequencer.transitionCountdown = 2; break;
        case 4: local.drumSequencer.transitionCountdown = 3; break;
        case 5: local.drumSequencer.transitionCountdown = 4; break;
        case 6: local.drumSequencer.transitionCountdown = 5; break;
        case 7: local.drumSequencer.transitionCountdown = 6; break;
        case 8: local.drumSequencer.transitionCountdown = 7; break;
        case 9: local.drumSequencer.transitionCountdown = 8; break;
        case 10: local.drumSequencer.transitionCountdown = 9; break;
        case 11: local.drumSequencer.transitionCountdown = 11; break;
        case 12: local.drumSequencer.transitionCountdown = 15; break;
        case 13: local.drumSequencer.transitionCountdown = 23; break;
        case 14: local.drumSequencer.transitionCountdown = 31; break;
        case 15: local.drumSequencer.transitionCountdown = 63; break;
        }
    local.drumSequencer.patternCountup = 255;
    }
    
void resetDrumSequencerSequenceCountdown()
    {
    // This is forever, 1 time, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 32, 64 times 
    switch(local.drumSequencer.repeatSequence)
        {
        case 0: local.drumSequencer.sequenceCountdown = 255; break;
        case 1: local.drumSequencer.sequenceCountdown = 0; break;
        case 2: local.drumSequencer.sequenceCountdown = 1; break;
        case 3: local.drumSequencer.sequenceCountdown = 2; break;
        case 4: local.drumSequencer.sequenceCountdown = 3; break;
        case 5: local.drumSequencer.sequenceCountdown = 4; break;
        case 6: local.drumSequencer.sequenceCountdown = 5; break;
        case 7: local.drumSequencer.sequenceCountdown = 6; break;
        case 8: local.drumSequencer.sequenceCountdown = 7; break;
        case 9: local.drumSequencer.sequenceCountdown = 8; break;
        case 10: local.drumSequencer.sequenceCountdown = 9; break;
        case 11: local.drumSequencer.sequenceCountdown = 11; break;
        case 12: local.drumSequencer.sequenceCountdown = 15; break;
        case 13: local.drumSequencer.sequenceCountdown = 23; break;
        case 14: local.drumSequencer.sequenceCountdown = 31; break;
        case 15: local.drumSequencer.sequenceCountdown = 63; break;
        }
    resetDrumSequencerTransitionCountdown();                // I think I need to do this?
    }

    
    
    
void resetGroup(uint8_t group)
    {
    for(uint8_t i = 0; i < local.drumSequencer.numTracks; i++)
        {
        resetTrack(i, group);
        }
    // setGroupLength(group, local.drumSequencer....
    }
        
void resetTrack(uint8_t track, uint8_t group)
    {
    clearNotes(group, track);
//      setPattern(group, track, DRUM_SEQUENCER_PATTERN_ALL);
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
    uint8_t solo = (local.drumSequencer.solo == DRUM_SEQUENCER_SOLO || local.drumSequencer.solo == DRUM_SEQUENCER_SOLO_OFF_SCHEDULED);
    uint8_t muted = (local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTED || local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_OFF_SCHEDULED || local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE);
    return 
        // We're muted if solo is on and we're not the current track
        (solo && track != local.drumSequencer.currentTrack) ||
        // We're muted if solo is NOT on and OUR mute is on
        (!solo && muted);
    }


// Draws the sequence with the given track length, number of tracks, and skip size
void drawDrumSequencer(uint8_t trackLen, uint8_t numTracks, uint8_t skip)
    {
    clearScreen();
    
    // revise LASTTRACK to be just beyond the last track we'll draw
    //      (where TRACK is the first track we'll draw)     
        
    // this code is designed to allow the user to move down to about the middle of the screen,
    // at which point the cursor stays there and the screen scrolls instead.
    uint8_t firstTrack = local.drumSequencer.currentTrack;
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
        // for each note in the track
        for (uint8_t d = 0; d < trackLen; d++)
            {
            uint8_t shouldDrawMuted = shouldMuteTrack(t);
            uint16_t pos = (t * (uint16_t) trackLen + d) * 2;
            uint8_t vel = getNote(local.drumSequencer.currentGroup, t, d);
                
            else if ((local.drumSequencer.data[local.drumSequencer.currentTrack] != DRUM_SEQUENCER_DATA_NOTE) &&
                ((vel & 127) | ((data.slot.data.drumSequencer.buffer[pos] & 127) << 7)) != 0)
                vel = 1;  // so we draw it

            if (shouldDrawMuted)
                vel = 0;
            uint8_t xpos = d - ((d >> 4) * 16);  // x position on screen
            uint8_t blink = (
                // draw play position cursor if we're not stopped and we're in edit cursor mode
                    ((local.drumSequencer.playState != PLAY_STATE_STOPPED) && (d == local.drumSequencer.currentPlayPosition)
                    && !local.drumSequencer.performanceMode) ||   // main cursor
                // draw play position cursor, plus the crosshatch, always if we're in play position mode
                    (((local.drumSequencer.currentEditPosition < 0 || local.drumSequencer.currentEditPosition >= trackLen)
                        || local.drumSequencer.performanceMode) && 
                    ((d == local.drumSequencer.currentPlayPosition) ||  ((t == local.drumSequencer.currentTrack) && (abs(d - local.drumSequencer.currentPlayPosition) == 2)))) ||  // crosshatch
                // draw edit cursor
                    ((t == local.drumSequencer.currentTrack) && (d == local.drumSequencer.currentEditPosition) 
                    && !local.drumSequencer.performanceMode) ||
                
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
    if (local.drumSequencer.currentTrack < 16)
        {
        drawRange(led2, 0, 1, 16, local.drumSequencer.currentTrack);
        clearPoint(led, 6, 1);
        }
    else if (local.drumSequencer.currentTrack < 32)
        {
        drawRange(led2, 0, 1, 16, local.drumSequencer.currentTrack - 16);
        setPoint(led, 6, 1);
        }
    else
        {
        //// uhhhh......
        }

    // Next the group
    drawRange(led2, 0, 0, 15, local.drumSequencer.currentGroup);

    // Are we in performance mode?
    if (local.drumSequencer.performanceMode)
        {
        blinkPoint(led, 2, 1);
        }       
    // is our track scheduled to play?
    if (local.drumSequencer.shouldPlay[local.drumSequencer.currentTrack])
        setPoint(led, 4, 1);
                
    // draw pattern position
    drawRange(led, 0, 1, 4, local.drumSequencer.patternCountup & 3);

    // Are we stopped?
    if (local.drumSequencer.playState != PLAY_STATE_PLAYING)
        setPoint(led, 7, 1);
    }


// Sends a Note ON to the appropriate MIDI channel at the appropriate pitch and velocity
void sendTrackNote(uint8_t track)
    {
    uint8_t velocity = getNoteVelocity(track);
    uint8_t out = getMIDIChannel(track);
    uint8_t note = getNotePitch(track);
    if (out == MIDI_OUT_DEFAULT) 
        out = options.channelOut;
    if (out != NO_MIDI_OUT)
        {
        sendNoteOn(note, velocity, out);
        }
    }



/*
/// This version allows us to have a "Right Mode".  For now we don't.

// I'd prefer the following code as it creates a bit of a buffer so scrolling to position 0 doesn't go straight
// into play position mode.  Or perhaps we should change things so that you scroll to the far RIGHT edge, dunno.
// But anyway we can't do this because adding just a little bit here radically increases our memory footprint. :-(
#define CURSOR_MARGIN (4)

int16_t getNewCursorXPos(uint8_t trackLen)
{
int16_t val = ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + CURSOR_MARGIN + CURSOR_MARGIN)) >> 9)) - CURSOR_MARGIN;
if ((val < 0) && (val > -CURSOR_MARGIN))
val = 0;
if (val >= trackLen && (val < trackLen + CURSOR_MARGIN)
val = trackLen - 1;
return val;
}
*/
      
// I'd prefer the following code as it creates a bit of a buffer so scrolling to position 0 doesn't go straight
// into play position mode.  Or perhaps we should change things so that you scroll to the far RIGHT edge, dunno.
// But anyway we can't do this because adding just a little bit here radically increases our memory footprint. :-(
#define CURSOR_MARGIN (4)

int16_t getNewCursorXPos(uint8_t trackLen)
    {
    int16_t val = ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + CURSOR_MARGIN)) >> 9)) - CURSOR_MARGIN;
    if ((val < 0) && (val > -CURSOR_MARGIN))
        val = 0;
    return val;
    }




void resetDrumSequencer()
    {
    local.drumSequencer.currentPlayPosition = getActualGroupLength(local.drumSequencer.currentGroup) - 1;
    resetDrumSequencerTransitionCountdown();
    }
        
void stopDrumSequencer()
    {
    resetDrumSequencer();
    local.drumSequencer.playState = PLAY_STATE_STOPPED;
    sendAllSoundsOff();
    }




// Plays and records the sequence
void stateDrumSequencerPlay()
    {
    // first we:
    // compute TRACKLEN, the length of the track
    // compute SKIP, the number of lines on the screen the track takes up
    uint8_t trackLen = getActualGroupLength(local.drumSequencer.currentGroup());
    uint8_t numTracks = local.drumSequencer.numTracks;
    
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
        local.drumSequencer.currentRightPot = -1;
        setPots(local.drumSequencer.pots);
        }

    immediateReturn = false;

    // always do this
    leftPotParameterEquivalent = false;

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        setParseRawCC(false);
        if (local.drumSequencer.performanceMode)
            {
            //// EXIT PERFORMANCE MODE
            local.drumSequencer.performanceMode = false;
            }
        else
            {
            //// EXIT SEQUENCER
            goUpState(STATE_DRUM_SEQUENCER_SURE);
            }
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        if (local.drumSequencer.performanceMode)
            {
            //// SCHEDULE TRANSITION OR NEXT SEQUENCE?
            advanceMute(local.drumSequencer.currentTrack);
            }
        else if (local.drumSequencer.currentEditPosition < 0)
            {
            //// TOGGLE MUTE
            local.drumSequencer.muted[local.drumSequencer.currentTrack] = !local.drumSequencer.muted[local.drumSequencer.currentTrack];
            }
        else if (local.drumSequencer.currentEditPosition >= trackLen)
            {
            //// DUNNO -- FAR RIGHT
            }
        else
            {
            //// TOGGLE NOTE
            }
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        if (button[SELECT_BUTTON])
            {
            isUpdated(SELECT_BUTTON, PRESSED);  // kill the long release on the select button
            if (local.drumSequencer.performanceMode)
                {
                //// SCHEDULE MUTE
                advanceMute(local.drumSequencer.currentTrack);
                }
            else
                {
                //// ENTER PERFORMANCE MODE
                local.drumSequencer.performanceMode = true;
                resetDrumSequencerTransitionCountdown();  // otherwise we'll miss jumps to other sequences
                setParseRawCC(true);
                }
            }
        else 
            {
            if (local.drumSequencer.performanceMode)
                {
                //// SCHEDULE SOLO
                advanceSolo();
                }
            else if (local.drumSequencer.currentEditPosition < 0)
                {
                //// CLEAR TRACK
                // do a "light" clear, not a full reset
                memset(data.slot.data.drumSequencer.buffer + ((uint16_t)trackLen) * local.drumSequencer.currentTrack * 2, 0, trackLen * 2);
                }
            else if (local.drumSequencer.currentEditPosition >= trackLen)
                {
                //// DUNNO -- FAR RIGHT
                }
            else
                {
                //// DUNNO -- EDIT MODE
                }
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        //// START/STOP
        
        if (options.drumSequencerSendClock)
            {
            // we always stop the clock just in case, even if we're immediately restarting it
            stopClock(true);
            }
        switch(local.drumSequencer.playState)
            {
            case PLAY_STATE_STOPPED:
                {
                local.drumSequencer.playState = PLAY_STATE_WAITING;
                if (options.drumSequencerSendClock)
                    {
                    // Possible bug condition:
                    // The MIDI spec says that there "should" be at least 1 ms between
                    // starting the clock and the first clock pulse.  I don't know if that
                    // will happen here consistently.
                    startClock(true);
                    }
                }
            break;
            case PLAY_STATE_WAITING:
                // Fall Thru
            case PLAY_STATE_PLAYING:
                {
                stopDrumSequencer();
                }
            break;
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        if (button[MIDDLE_BUTTON])
            {
            isUpdated(MIDDLE_BUTTON, PRESSED);  // kill the long release on the middle button
            if (local.drumSequencer.performanceMode)
                {
                //// SCHEDULE MUTE
                advanceMute(local.drumSequencer.currentTrack);
                }
            else
                {
                //// ENTER PERFORMANCE MODE
                local.drumSequencer.performanceMode = true;
                resetDrumSequencerTransitionCountdown();  // otherwise we'll miss jumps to other sequences
                setParseRawCC(true);
                }
            }
        else
            {
            //// ENTER MENU
            state = STATE_DRUM_SEQUENCER_MENU;
            entry = true;
            }
        }
    else if (potUpdated[LEFT_POT])
        {
        if (local.drumSequencer.currentEditPosition >= trackLen)
            {
            //// STOP AND CHANGE GROUP
            }
        else
            {
            //// CHANGE TRACK
            local.drumSequencer.currentTrack = ((pot[LEFT_POT] * numTracks) >> 10);         //  / 1024;
            local.drumSequencer.currentTrack = bound(local.drumSequencer.currentTrack, 0, numTracks);
            setParseRawCC(local.drumSequencer.data[local.drumSequencer.currentTrack] == DRUM_SEQUENCER_DATA_CC);
            // local.drumSequencer.clearTrack = CLEAR_TRACK;
            }
        }
    else if (potUpdated[RIGHT_POT])
        {
        if (local.drumSequencer.performanceMode)
            {
            //// CHANGE TEMPO
#define BIG_POT_UPDATE (32)
            if (potChangedBy(local.drumSequencer.pots, RIGHT_POT, BIG_POT_UPDATE))
                {
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_TEMPO);
                }
            }
        else if (local.drumSequencer.currentEditPosition >= trackLen)
            {
            //// DUNNO
            }
        else
            {
            //// CHANGE POSITION
            int16_t newPos = getNewCursorXPos(trackLen);
            if (lockoutPots ||      // using an external NRPN device, which is likely accurate
                local.drumSequencer.currentRightPot == -1 ||   // nobody's been entering data
                local.drumSequencer.currentRightPot >= newPos && local.drumSequencer.currentRightPot - newPos >= 2 ||
                local.drumSequencer.currentRightPot < newPos && newPos - local.drumSequencer.currentRightPot >= 2)
                {
                local.drumSequencer.currentEditPosition = newPos;
                local.drumSequencer.currentRightPot = -1;
                }
            }
        }
        
        
///// INCOMING MIDI DATA
  
    else if (bypass)
        {
        // do nothing
        }
    

    // rerouting to new channel
    if (newItem && 
        itemType != MIDI_CUSTOM_CONTROLLER && 
        local.drumSequencer.performanceMode && 
        options.drumSequencerPlayAlongChannel != CHANNEL_TRANSPOSE && 
        options.drumSequencerPlayAlongChannel != CHANNEL_ADD_TO_DRUM_SEQUENCER)
        {
        TOGGLE_IN_LED();
        // figure out what the channel should be
        uint8_t channelOut = options.drumSequencerPlayAlongChannel;
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
        local.drumSequencer.performanceMode && 
        options.drumSequencerPlayAlongChannel == CHANNEL_TRANSPOSE)
        {
        TOGGLE_IN_LED();
        local.drumSequencer.transpose = ((int8_t)itemNumber) - (int8_t) MIDDLE_C;  // this can only range -60 ... 67
        }


    else if (newItem && 
        (itemType == MIDI_NOTE_ON))   //// there is a note played
        {
        TOGGLE_IN_LED();
        uint8_t note = itemNumber;

        // For the time being we only enter in data on the current track.  But perhaps we should allow
        // entering data in on multiple tracks at once.
        
        if ((local.drumSequencer.muted[local.drumSequencer.currentTrack] && !options.drumSequencerNoEcho)
            || (local.drumSequencer.performanceMode && options.drumSequencerPlayAlongChannel == CHANNEL_ADD_TO_DRUM_SEQUENCER ))
            {
            // play the note
            sendTrackNote(local.drumSequencer.currentTrack);
            }
        else 
            {
            /// We have different ways of entering drum note information depending on the edit mode
                
            uint8_t len = getActualGroupLength(local.drumSequencer.currentGroup());
            if (local.drumSequencer.currentEditPosition > 0 && local.drumSequencer.currentEditPosition < len)
                {
                //// We're in EDIT MODE
                //// White keys in Edit mode, starting with Middle C, correspond to individual drumbeats.  Pressing them toggles the beat.
                //// The C# and D# Black keys in Edit mode SET the current note
                //// Other black keys in Edit mode CLEAR the current note
                                
                uint16_t octave = div12(note);
                if (octave >= 5)  // middle c and up
                    {
                    uint16_t key = DIV12_REMAINDER(octave, note);
                    const uint8_t[12] whiteKey = { 0, -1, 1, -1, 2, 3, -2, 4, -2, 5, -2, 6 };
                    key = whiteKey[key];
                    if (key >= 0)           // it is in fact a white key
                        {
                        key = key + octave * 8 - 60;            // this should start with middle c at 0
                        if (key < len)
                            {
                            uint8_t n = getNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, key);
                            setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, key, !n);
                            }
                        }
                    else if (key == -1)             // it's a C# or D#  -- set
                        {
                        uint8_t n = getNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition);
                        setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition, 1);
                        }
                    else
                        {
                        uint8_t n = getNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition);
                        setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition, 0);
                        }
                    }
                }
            else if (local.drumSequencer.currentEditPosition < 0)
                {
                //// We're in PLAY POSITION MODE
                //// The C# and D# Black keys in Play position mode SET the current note and advance
                //// The F# and G# and A# Black keys in Play position mode SET the current note and advance
                //// C will move the cursor to the left, G will move it to the right.
                //// Other white keys do nothing. 
                                
                uint16_t octave = div12(note);
                uint16_t key = DIV12_REMAINDER(octave, note);
                if (key == 1 || key == 3)       //  C# or D#
                    {
                    // set
                    setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition, 1);
                    local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, len);
                    }
                else if (key == 6 || key == 8 || key == 10)   // F#, G#, A#
                    {
                    // clear
                    setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition, 0);
                    local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, len);
                    }
                else if (key == 0)     // C
                    {
                    if (local.drumSequencer.currentEditPosition == 0)
                        local.drumSequencer.currentEditPosition = len - 1;
                    else local.drumSequencer.currentEditPosition--;
                    }
                else if (key == 7)              // G
                    {
                    local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, len);
                    }
                local.drumSequencer.currentRightPot = getNewCursorXPos(len);
                }
            else            // right
                {
                }
            }
        }
    // NOTE: we ignore NOTE_OFF
    
    else if (newItem && (itemType == MIDI_AFTERTOUCH || itemType == MIDI_AFTERTOUCH_POLY))
        {
        // pass through -- hope this is right
        // Note that we're always converting to channel aftertouch
         
        sendControllerCommand(CONTROL_TYPE_AFTERTOUCH, 0, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_PROGRAM_CHANGE))
        {
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_PC, 0, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_PITCH_BEND))
        {
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_PITCH_BEND, 0, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_CC_7_BIT))
        {
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_CC, itemNumber, itemValue << 7, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_CC_14_BIT))
        {
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_CC, itemNumber, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_NRPN_14_BIT))
        {
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_NRPN, itemNumber, itemValue, options.channelOut);
        }
    else if (newItem && (itemType == MIDI_RPN_14_BIT))
        {
        // pass through -- hope this is right
        sendControllerCommand(CONTROL_TYPE_RPN, itemNumber, itemValue, options.channelOut);
        }
    /*
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
      if (local.drumSequencer.performanceMode)
      {
      advanceMute(itemNumber - CC_EXTRA_PARAMETER_A);
      }
      else
      {
      #ifdef INCLUDE_DRUM_SEQUENCER_CC_MUTE_TOGGLES
      local.drumSequencer.muted[itemNumber - CC_EXTRA_PARAMETER_A] = itemValue;
      #else
      local.drumSequencer.muted[itemNumber - CC_EXTRA_PARAMETER_A] = !local.drumSequencer.muted[itemNumber - CC_EXTRA_PARAMETER_A];
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
      if (itemNumber - (CC_EXTRA_PARAMETER_A + 12) < local.drumSequencer.numTracks )  // track is valid
      {
      local.drumSequencer.currentTrack = itemNumber - (CC_EXTRA_PARAMETER_A + 12);
      }
      break;
      }
      case CC_EXTRA_PARAMETER_Y:
      {
      if (local.drumSequencer.performanceMode)
      advanceSolo();
      else
      local.drumSequencer.solo = !local.drumSequencer.solo;
      break;
      }
      case CC_EXTRA_PARAMETER_Z:
      {
      // transposable
      local.drumSequencer.transposable[local.drumSequencer.currentTrack] = !local.drumSequencer.transposable[local.drumSequencer.currentTrack];
      break;
      }
      case CC_EXTRA_PARAMETER_1:
      {
      // do a "light" clear, not a full reset
      memset(data.slot.data.drumSequencer.buffer + ((uint16_t)trackLen) * local.drumSequencer.currentTrack * 2, 0, trackLen * 2);
      break;
      }
      case CC_EXTRA_PARAMETER_2:
      {
      // toggle schedule next sequence
      if (local.drumSequencer.performanceMode)
      {
      local.drumSequencer.goNextTransition = !local.drumSequencer.goNextTransition;
      }
      break;
      }
      case CC_EXTRA_PARAMETER_3:
      {
      IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_CLICK);
      break;
      }
      case CC_EXTRA_PARAMETER_4:
      {
      IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_EDIT_MARK);
      break;
      }
      case CC_EXTRA_PARAMETER_5:
      {
      IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_EDIT_COPY);
      break;
      }
      case CC_EXTRA_PARAMETER_6:
      {
      IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_EDIT_SPLAT);
      break;
      }
      case CC_EXTRA_PARAMETER_7:
      {
      IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_EDIT_MOVE);
      break;
      }
      case CC_EXTRA_PARAMETER_8:
      {
      IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_EDIT_DUPLICATE);
      break;
      }
                        
                        
      // this is a discontinuity, hope compiler can handle it
                        
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_1:
      {
      // length
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      leftPotParameterEquivalent = true;
      goDownState(STATE_DRUM_SEQUENCER_LENGTH);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_2:
      {
      // midi out
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      leftPotParameterEquivalent = true;
      goDownState(STATE_DRUM_SEQUENCER_MIDI_CHANNEL_OUT);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_3:
      {
      // velocity
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      leftPotParameterEquivalent = true;
      goDownState(STATE_DRUM_SEQUENCER_VELOCITY);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_4:
      {
      // fader
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      leftPotParameterEquivalent = true;
      goDownState(STATE_DRUM_SEQUENCER_FADER);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_5:
      {
      // pattern
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_PATTERN);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_6:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_TEMPO);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_7:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_TRANSPOSE);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_8:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_VOLUME);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_9:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_NOTE_SPEED);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_10:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_PLAY_LENGTH);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_11:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_SWING);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_12:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_PERFORMANCE_KEYBOARD);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_13:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_PERFORMANCE_REPEAT);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_14:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_DRUM_SEQUENCER_MENU_PERFORMANCE_NEXT);
      break;
      }
      case CC_LEFT_POT_PARAMETER_EQUIVALENT_6_LSB:
      {
      leftPotParameterEquivalent = true;
      AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
      goDownState(STATE_OPTIONS_TEMPO);
      break;
      }
      }
      }
    */

    playDrumSequencer();
    if (updateDisplay)
        {
        drawDrumSequencer(trackLen, numTracks, skip);
        }
    }


// Various choices in the menu
#define DRUM_SEQUENCER_MENU_SOLO 0
#define DRUM_SEQUENCER_MENU_RESET 1
#define DRUM_SEQUENCER_MENU_LENGTH 2
#define DRUM_SEQUENCER_MENU_MIDI_OUT 3
#define DRUM_SEQUENCER_MENU_VELOCITY 4
#define DRUM_SEQUENCER_MENU_FADER 5
#define DRUM_SEQUENCER_MENU_TYPE 6
#define DRUM_SEQUENCER_MENU_PATTERN 7
#define DRUM_SEQUENCER_MENU_TRANSPOSABLE 8
#define DRUM_SEQUENCER_MENU_EDIT 9
#define DRUM_SEQUENCER_MENU_SEND_CLOCK 10
#define DRUM_SEQUENCER_MENU_NO_ECHO 11
#define DRUM_SEQUENCER_MENU_PERFORMANCE 12
#define DRUM_SEQUENCER_MENU_SAVE 13
#define DRUM_SEQUENCER_MENU_OPTIONS 14



///// FIXME: We have to add "change group" and "transitions" here
///// FIXME: We also need a full submenu system for handling transition repeats and groups etc.

// Gives other options
void stateDrumSequencerMenu()
    {
    uint8_t result;

    const char* menuItems[15] = {    
        (local.drumSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("RESET TRACK"),
        PSTR("LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("PATTERN (TRACK)"),
        local.drumSequencer.transposable[local.drumSequencer.currentTrack] ? PSTR("NO TRANSPOSE (TRACK)") : PSTR("TRANSPOSE (TRACK)"),
        PSTR("EDIT"),
        options.drumSequencerSendClock ? PSTR("NO CLOCK CONTROL") : PSTR("CLOCK CONTROL"),
        options.drumSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        PSTR("PERFORMANCE"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 15, STATE_NONE, STATE_NONE, 1);

    playDrumSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            state = STATE_DRUM_SEQUENCER_PLAY;
            entry = true;
            switch(currentDisplay)
                {
                case DRUM_SEQUENCER_MENU_SOLO:
                    {
                    local.drumSequencer.solo = !local.drumSequencer.solo;
                    }
                break;
                case DRUM_SEQUENCER_MENU_RESET:
                    {
                    resetTrack(local.drumSequencer.currentTrack);
                    break;
                    }
                case DRUM_SEQUENCER_MENU_LENGTH:
                    {
                    state = STATE_DRUM_SEQUENCER_LENGTH;                            
                    }
                break;
                case DRUM_SEQUENCER_MENU_MIDI_OUT:
                    {
                    local.drumSequencer.backup = local.drumSequencer.outMIDI[local.drumSequencer.currentTrack];
                    state = STATE_DRUM_SEQUENCER_MIDI_CHANNEL_OUT;
                    }
                break;
                case DRUM_SEQUENCER_MENU_VELOCITY:
                    {
                    state = STATE_DRUM_SEQUENCER_VELOCITY;
                    }
                break;
                case DRUM_SEQUENCER_MENU_FADER:
                    {
                    state = STATE_DRUM_SEQUENCER_FADER;
                    }
                break;
                case DRUM_SEQUENCER_MENU_PATTERN:
                    {
                    immediateReturnState = STATE_DRUM_SEQUENCER_PLAY;
                    state = STATE_DRUM_SEQUENCER_MENU_PATTERN;
                    }
                break;
                case DRUM_SEQUENCER_MENU_TRANSPOSABLE:
                    {
                    local.drumSequencer.transposable[local.drumSequencer.currentTrack] = !local.drumSequencer.transposable[local.drumSequencer.currentTrack];
                    }
                break;
                case DRUM_SEQUENCER_MENU_EDIT:
                    {
                    state = STATE_DRUM_SEQUENCER_MENU_EDIT;
                    }
                break;
                case DRUM_SEQUENCER_MENU_SEND_CLOCK:
                    {
                    options.drumSequencerSendClock = !options.drumSequencerSendClock;
                    if (options.drumSequencerSendClock)
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
                case DRUM_SEQUENCER_MENU_NO_ECHO:
                    {
                    options.drumSequencerNoEcho = !options.drumSequencerNoEcho;
                    saveOptions();
                    }
                break;
                case DRUM_SEQUENCER_MENU_PERFORMANCE:
                    {
                    immediateReturnState = STATE_DRUM_SEQUENCER_MENU;
                    goDownState(STATE_DRUM_SEQUENCER_MENU_PERFORMANCE);
                    }
                break;

                case DRUM_SEQUENCER_MENU_SAVE:
                    {
                    state = STATE_DRUM_SEQUENCER_SAVE;
                    }
                break;
                case DRUM_SEQUENCER_MENU_OPTIONS:
                    {
                    immediateReturnState = STATE_DRUM_SEQUENCER_MENU;
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
            goUpState(STATE_DRUM_SEQUENCER_PLAY);
            }
        break;
        }
        
    }
    
    
// Plays the current sequence
void playDrumSequencer()
    {
    // we redo this rather than take it from stateDrumSequencerPlay because we may be 
    // called from other methods as well 
        
    uint8_t trackLen = getActualGroupLength(local.drumSequencer.currentGroup());
    uint8_t numTracks = local.drumSequencer.numTracks;
        
    if ((local.drumSequencer.playState == PLAY_STATE_WAITING) && beat)
        local.drumSequencer.playState = PLAY_STATE_PLAYING;
        
    if (notePulse && (local.drumSequencer.playState == PLAY_STATE_PLAYING))
        {
        goNextTransition();

        uint8_t oldPlayPosition = local.drumSequencer.currentPlayPosition;
        local.drumSequencer.currentPlayPosition = incrementAndWrap(local.drumSequencer.currentPlayPosition, trackLen);
        
        // maybe gotta change the pulse rate
        if (local.drumSequencer.currentPlayPosition == 0)
            {
            uint8_t noteSpeedType = getGroupSpeed(local.drumSequencer.currentGroup);
            if (noteSpeedType == 0) 
                {
                noteSpeedType = options.noteSpeedType;
                }
            else
                {
                noteSpeedType--;                // this means double-whole-note is not an option unless it's the default
                }
            setNotePulseRate(noteSpeedType);
            }
        
        // change scheduled mute?
        if (local.drumSequencer.performanceMode && local.drumSequencer.currentPlayPosition == 0)
            {
            for(uint8_t track = 0; track < numTracks; track++)
                {
                if (local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_ON_SCHEDULED)
                    local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTED;
                else if (local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_OFF_SCHEDULED)
                    local.drumSequencer.muted[track] = DRUM_SEQUENCER_NOT_MUTED;
                else if (local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_ON_SCHEDULED_ONCE)
                    local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTE_OFF_SCHEDULED;
                else if (local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE)
                    local.drumSequencer.muted[track] = DRUM_SEQUENCER_MUTE_ON_SCHEDULED;
                }
                
            if (local.drumSequencer.solo == DRUM_SEQUENCER_SOLO_ON_SCHEDULED)
                local.drumSequencer.solo = DRUM_SEQUENCER_SOLO;
            else if (local.drumSequencer.solo == DRUM_SEQUENCER_SOLO_OFF_SCHEDULED)
                local.drumSequencer.solo = DRUM_SEQUENCER_NO_SOLO;
            }

        if (local.drumSequencer.currentPlayPosition == 0)
            {
            local.drumSequencer.patternCountup++;
            }

// pick an exclusive random track
        uint8_t exclusiveTrack = 0;
        if (local.drumSequencer.currentPlayPosition == 0)
            {
            int trkcount = 0;
            for(uint8_t track = 0; track < numTracks; track++)
                {
                if (local.drumSequencer.pattern[track] == DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE)
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
            if (local.drumSequencer.currentPlayPosition == 0)
                {
                // pick a random track                          
                if (local.drumSequencer.pattern[track] == DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE)
                    {
                    local.drumSequencer.shouldPlay[track] = (track == exclusiveTrack);
                    }
                else if (local.drumSequencer.pattern[track] == DRUM_SEQUENCER_PATTERN_RANDOM_3_4)
                    {
                    local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 4) * 3);
                    }
                else if (local.drumSequencer.pattern[track] == DRUM_SEQUENCER_PATTERN_RANDOM_1_2)
                    {
                    local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 2));
                    }
                else if (local.drumSequencer.pattern[track] == DRUM_SEQUENCER_PATTERN_RANDOM_1_4)
                    {
                    local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 4));
                    }
                else if (local.drumSequencer.pattern[track] == DRUM_SEQUENCER_PATTERN_RANDOM_1_8)
                    {
                    local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 8));
                    }
                else
                    {
                    local.drumSequencer.shouldPlay[track] = ((local.drumSequencer.pattern[track] >> (local.drumSequencer.patternCountup & 3)) & 1);                        
                    }
                }
                                                            
            if (note && local.drumSequencer.shouldPlay[track] && !shouldMuteTrack(track))
                {
                sendTrackNote(track);         
                }
            }
        }

    // click track
    doClick();
    }



//// NOTE: The IMMEDIATE_RETURN feature removed from the next three functions
//// because I've found it VERY ANNOYING -- typically you need to set multiple
//// items, and so should go back into the menu.

void stateDrumSequencerMenuPerformanceKeyboard()
    {
    uint8_t result = doNumericalDisplay(CHANNEL_ADD_TO_DRUM_SEQUENCER, CHANNEL_TRANSPOSE, options.drumSequencerPlayAlongChannel, true, GLYPH_TRANSPOSE);
    playDrumSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
// do nothing
            }
        break;
        case MENU_SELECTED:
            {
            options.drumSequencerPlayAlongChannel = currentDisplay;
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
        
void stateDrumSequencerMenuPerformanceRepeat()  
    {
// This is forever, 1 time, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 times 
    const char* menuItems[16] = {  PSTR("FOREVER"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("8"), PSTR("9"), PSTR("12"), PSTR("16"), PSTR("18"), PSTR("24"), PSTR("32"), PSTR("64"), PSTR("128") };
    if (entry) 
        {
        defaultMenuValue = data.slot.data.drumSequencer.repeat & 0x0F;
        }
    uint8_t result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
                
    playDrumSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
// do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.data.drumSequencer.repeat = ((data.slot.data.drumSequencer.repeat & 0xF0) | (currentDisplay & 0x0F));
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

        
void stateDrumSequencerMenuPerformanceNext()
    {
// The values are OFF, 0, 1, ..., 8
// These correspond with stored values (in the high 4 bits of repeat) of 0...9
    uint8_t result = doNumericalDisplay(-1, 8, ((int16_t)(data.slot.data.drumSequencer.repeat >> 4)) - 1, true, GLYPH_NONE);
    playDrumSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
// do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.slot.data.drumSequencer.repeat = ((data.slot.data.drumSequencer.repeat & 0x0F) | ((currentDisplay + 1) << 4));
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


/*
  void stateDrumSequencerMenuEditMark()
  {
  local.drumSequencer.markTrack = local.drumSequencer.currentTrack;
  local.drumSequencer.markPosition = local.drumSequencer.currentEditPosition;
  if (local.drumSequencer.markPosition < 0)
  local.drumSequencer.markPosition = 0;
  goUpState(immediateReturnState);
  }

  void stateDrumSequencerMenuEditDuplicate()
  {
  local.drumSequencer.data[local.drumSequencer.currentTrack] = local.drumSequencer.data[local.drumSequencer.markTrack];
  local.drumSequencer.outMIDI[local.drumSequencer.currentTrack] = local.drumSequencer.outMIDI[local.drumSequencer.markTrack];
  local.drumSequencer.noteLength[local.drumSequencer.currentTrack] = local.drumSequencer.noteLength[local.drumSequencer.markTrack];
  local.drumSequencer.velocity[local.drumSequencer.currentTrack] = local.drumSequencer.velocity[local.drumSequencer.markTrack];
  local.drumSequencer.fader[local.drumSequencer.currentTrack] = local.drumSequencer.fader[local.drumSequencer.markTrack];
  local.drumSequencer.offTime[local.drumSequencer.currentTrack] = local.drumSequencer.offTime[local.drumSequencer.markTrack];
  local.drumSequencer.noteOff[local.drumSequencer.currentTrack] = local.drumSequencer.noteOff[local.drumSequencer.markTrack];
  local.drumSequencer.shouldPlay[local.drumSequencer.currentTrack] = local.drumSequencer.shouldPlay[local.drumSequencer.markTrack];
  local.drumSequencer.transposable[local.drumSequencer.currentTrack] = local.drumSequencer.transposable[local.drumSequencer.markTrack];
  local.drumSequencer.pattern[local.drumSequencer.currentTrack] = local.drumSequencer.pattern[local.drumSequencer.markTrack];
  #ifdef INCLUDE_ADVANCED_DRUM_SEQUENCER
  local.drumSequencer.controlParameter[local.drumSequencer.currentTrack] = local.drumSequencer.controlParameter[local.drumSequencer.markTrack];
  local.drumSequencer.lastControlValue[local.drumSequencer.currentTrack] = local.drumSequencer.lastControlValue[local.drumSequencer.markTrack];
  #endif INCLUDE_ADVANCED_DRUM_SEQUENCER

  // set the mark and edit positions to 0 temporarily so we can copy the note data properly
  uint8_t backupMarkPosition = local.drumSequencer.markPosition;
  uint8_t backupEditPosition = local.drumSequencer.currentEditPosition;
  local.drumSequencer.markPosition = 0;
  local.drumSequencer.currentEditPosition = 0;
  stateDrumSequencerMenuEditCopy(false, false);  // we copy rather than splat to save some time
        
  // reset the mark and edit positions
  local.drumSequencer.markPosition = backupMarkPosition;
  local.drumSequencer.currentEditPosition = backupEditPosition;
        
  goUpState(immediateReturnState);
  }


  void stateDrumSequencerMenuEditCopy(uint8_t splat, uint8_t move)
  {
  // verify that the two tracks are the same type
  if (local.drumSequencer.data[local.drumSequencer.markTrack] == local.drumSequencer.data[local.drumSequencer.currentTrack]) 
  {
  uint8_t len = getActualGroupLength(local.drumSequencer.currentGroup();
  uint8_t buf[MAXIMUM_TRACK_LENGTH * 2];
  // copy it to third location
  memcpy(buf, data.slot.data.drumSequencer.buffer + local.drumSequencer.markTrack * ((uint16_t)len) * 2, len * 2);
        
  if (move)
  {
  // clear mark track
  memset(data.slot.data.drumSequencer.buffer + local.drumSequencer.markTrack * ((uint16_t)len) * 2, 
  local.drumSequencer.data[local.drumSequencer.markTrack] == DRUM_SEQUENCER_DATA_NOTE ? 0 : CONTROL_VALUE_EMPTY, 
  len * 2);
  }
        
  // now copy
  uint8_t p0 = local.drumSequencer.markPosition;
  int8_t p1 = local.drumSequencer.currentEditPosition;
  if (p1 < 0) p1 = 0;
  for(uint8_t i = 0; i < len; i++)
  {
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + p1) * 2] = buf[p0 * 2];
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + p1) * 2 + 1] = buf[p0 * 2 + 1];
  p0++;
  if (p0 >= len ) { if (splat) break; else p0 = 0; }
  p1++;
  if (p1 >= len ) { if (splat) break; else p1 = 0; }
  }
                        
  // If we were splatting, and the data is note data, we need to eliminate invalid ties.  This isn't needed for
  // moving or copying, because the whole track is moved even if it's rotated.
  // Can't use removeSuccessiveTies() here.  Maybe we could merge those functions in the future.
  if (splat && local.drumSequencer.data[local.drumSequencer.markTrack] == DRUM_SEQUENCER_DATA_NOTE)
  {
  for(uint8_t j = 0; j < 2; j++) // do this twice to make sure we get everything.  I think I just need to check one more note...
  {
  for(uint8_t i = 0; i < len; i++)
  {
  if (
  // I am a tie
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + i) * 2] == 0 &&
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + i) * 2 + 1] == 1 &&
  // ...and the note before me is a rest (if I'm at pos 0, then the note before me is at pos (len - 1) )
  (i == 0 ? 
  (data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + (len - 1)) * 2] == 0 &&
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + (len - 1)) * 2 + 1] == 0) :
  (data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + (i - 1)) * 2] == 0 &&
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + (i - 1)) * 2 + 1] == 0)))
  {
  // then set me to a rest
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + i) * 2] = 0;
  data.slot.data.drumSequencer.buffer[(local.drumSequencer.currentTrack * (uint16_t)len + i) * 2 + 1] = 0;
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
  goDownState(STATE_DRUM_SEQUENCER_MENU_NO);  // failed
  }
  }
*/


#endif INCLUDE_DRUM_SEQUENCER

