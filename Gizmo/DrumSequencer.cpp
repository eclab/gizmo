////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#ifdef INCLUDE_DRUM_SEQUENCER


/////// DATA ACCESS MACROS AND FUNCTIONS


// The data buffer is laid out as follows:
// 1. TRACK DATA[tracks] -- this is two bytes per track.  We'll call these bytes TRACK0 and TRACK1.
// 2. GROUP DATA[groups] -- this is one byte per group
// 3. GROUP TRACK DATA[groups][tracks] -- this is one half byte per group per track  Thus 2 pairs are stuffed into a byte.  The total number of pairs per track is NUM_TRACKPAIRS.
// 4. NOTE DATA[groups][tracks][notes] -- this is 1 *bit* per group per track per note.  Thus 8 notes are stuffed into a byte.  Each byte can be accessed with GET_NOTES/SET_NOTES.
//    The total number of bytes constituting all the notes is NUM_NOTE_BYTES.
//
// Many of these values are uint8_t.  But as the size of the offset grows, the locations need to move to uint16_t.


//// TRACK DATA
// track byte 0 and track byte 1
#define TRACK_OFFSET                                            (0)                     // I don't think we need to make this uint16_t but it might not hurt
#define GET_TRACK0(track)                                       (data.slot.data.drumSequencer.data[TRACK_OFFSET + (track) * 2 + 0])
#define GET_TRACK1(track)                                       (data.slot.data.drumSequencer.data[TRACK_OFFSET + (track) * 2 + 1])
#define SET_TRACK0(track, val)                                  (data.slot.data.drumSequencer.data[TRACK_OFFSET + (track) * 2 + 0] = (val))
#define SET_TRACK1(track, val)                                  (data.slot.data.drumSequencer.data[TRACK_OFFSET + (track) * 2 + 1] = (val))
#define TRACK_DATALEN                                           (local.drumSequencer.numTracks * 2)                     // this maxes out at 40 bytes


//// GROUP DATA
// group byte
#define GROUP_OFFSET                                            (TRACK_OFFSET + TRACK_DATALEN)                  // I don't think we need to make this uint16_t but it might not hurt
#define GET_GROUP(group)                                        (data.slot.data.drumSequencer.data[GROUP_OFFSET + (group)])
#define SET_GROUP(group, val)                                   (data.slot.data.drumSequencer.data[GROUP_OFFSET + (group)] = (val))
#define GROUP_DATALEN                                           (local.drumSequencer.numGroups)                                 // This maxes out at 70 bytes


//// GROUP-TRACK DATA
// group-track data, 2 at a time (4 bits each)
#define GROUPTRACK_OFFSET                                       (GROUP_OFFSET + GROUP_DATALEN)                  // I don't think we need to make this uint16_t but it might not hurt
#define NUM_TRACKPAIRS                                          (local.drumSequencer.numTracks >> 1)            // evaluates to 1/2 the number of tracks
#define GET_GROUPTRACK_PAIR(group, trackpair)                   (data.slot.data.drumSequencer.data[GROUPTRACK_OFFSET + NUM_TRACKPAIRS * (group) + (trackpair)])
#define SET_GROUPTRACK_PAIR(group, trackpair, val)              (data.slot.data.drumSequencer.data[GROUPTRACK_OFFSET + NUM_TRACKPAIRS * (group) + (trackpair)] = (val))
#define GROUPTRACK_DATALEN                                      (NUM_TRACKPAIRS * local.drumSequencer.numGroups)        // This maxes out at 220 bytes


//// NOTE DATA
// note bits, 8 at a time
#define NOTE_OFFSET                                             (GROUPTRACK_OFFSET + (uint16_t) GROUPTRACK_DATALEN)  // at this point we're definitely in the uint16_t range
#define NUM_NOTE_BYTES                                          (local.drumSequencer.numNotes >> 3)            // evaluates to 1/8 the number of notes in a track
#define GET_NOTE_OFFSET(group, track)                           (NOTE_OFFSET + NUM_NOTE_BYTES * local.drumSequencer.numTracks * (uint16_t)(group) + NUM_NOTE_BYTES * (uint16_t)(track))
#define GET_NOTES(group, track, notes)                          (data.slot.data.drumSequencer.data[GET_NOTE_OFFSET((group), (track)) + (notes)])   // notes are in groups of 8
#define SET_NOTES(group, track, notes, val)                     (data.slot.data.drumSequencer.data[GET_NOTE_OFFSET((group), (track)) + (notes)] = (val))   // notes are in groups of 8
#define NOTE_DATALEN                                            (NUM_NOTE_BYTES * local.drumSequencer.numGroups * (uint16_t) local.drumSequencer.numTracks)



///// A NOTE ON GROUP LENGTHS
/////
///// Each group has a maximum length determined by the format.  This length can be computed using
/////                   numFormatNotes(data.slot.data.drumSequencer.format)
///// This value is stored locally, and in most cases can be accessed more conveniently, as
/////                   local.drumSequencer.numNotes
///// numNotes is set during initialization and loading, so in general it's always correct.
///// An individual group can be resized to a value less than this.  The actual length of the group is unchanged,
/////       but Gizmo will not permit you to enter notes beyond the length, nor will it play them.  You cannot
/////       reduce the group size to less than 1.  This is accessed using the function:
/////                   getGroupLength(group)                   [returns a value from 1...64]
///// The ACTUAL storage of the group length is a value 0...63, where 0 means "Default" ("Full Length"), and
/////           1...63 means "use this length instead".  This is accessed and set using the functions
/////                   getGroupLengthData(group)               [returns a value 0...63]
/////                   setGroupLengthData(group, val)
///// Finally, the number of actual bytes used to store the notes in a given track in a given group, if for
/////           some reason you needed that, is specified as
/////                   (NUM_NOTE_BYTES)
///// ... which just expands to (since 8 notes are packed in one byte):
/////                   (local.drumSequencer.numNotes >> 3)


// Extract a note from data.slot.data.drumSequencer.data
uint8_t getNote(uint8_t group, uint8_t track, uint8_t note)
    {
    uint8_t div = note >> 3;                                                // /8
    uint8_t rem = note & 7;                                                 // Remainder
    uint8_t notes = GET_NOTES(group, track, div);
    return (notes >> rem) & 1;
    }

// Set (val = 1) or clear (val = 0) a note in data.slot.data.drumSequencer.data
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
 
// Toggle a note from data.slot.data.drumSequencer.data
void toggleNote(uint8_t group, uint8_t track, uint8_t note)
    {
    setNote(group, track, note, !getNote(group, track, note));          // this could of course be more efficient...
    }

// Set (to 1) a note from data.slot.data.drumSequencer.data
void setNote(uint8_t group, uint8_t track, uint8_t note)
    {
    setNote(group, track, note, 1);
    }

// Clear (to 0) a note from data.slot.data.drumSequencer.data
void clearNote(uint8_t group, uint8_t track, uint8_t note)
    {
    setNote(group, track, note, 0);
    }
       
// Clear all notes in a given track and group from data.slot.data.drumSequencer.data
void clearNotes(uint8_t group, uint8_t track)
    {
    memset(&(data.slot.data.drumSequencer.data[GET_NOTE_OFFSET(group, track)]), 0, (local.drumSequencer.numNotes >> 3));  // / 8
    }
        
// Return the pattern (0...15) for a given group and track from data.slot.data.drumSequencer.data
uint8_t getPattern(uint8_t group, uint8_t track)
    {
    uint8_t trackpair = track >> 1;                                         // /2
    uint8_t gt = GET_GROUPTRACK_PAIR(group, trackpair);
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
        
// Set the pattern (0...15) for a given group and track from data.slot.data.drumSequencer.data
void setPattern(uint8_t group, uint8_t track, uint8_t pattern)
    {
    uint8_t trackpair = track >> 1;                                         // /2
    uint8_t gt = GET_GROUPTRACK_PAIR(group, trackpair);
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
    SET_GROUPTRACK_PAIR(group, trackpair, gt);
    }
        
        
// Return the length (0 = DEFAULT, 1...63) for a given group from data.slot.data.drumSequencer.data.
uint8_t getGroupLengthData(uint8_t group)
    {
    uint8_t gt = GET_GROUP(group);
    // group length is the high 6 bits
    return (gt >> 2);
    }

// Set the length (0 = DEFAULT, 1...63) for a given group from data.slot.data.drumSequencer.data.
void setGroupLengthData(uint8_t group, uint8_t groupLength)
    {
    uint8_t gt = GET_GROUP(group);
    // group length is the high 6 bits
    gt = (gt & 3) | (groupLength << 2);
    SET_GROUP(group, gt);
    }

// Return the actual length, in number of notes (1...64), for a given group from data.slot.data.drumSequencer.data.
uint8_t getGroupLength(uint8_t group)
    {
    uint8_t gl = getGroupLengthData(group);
    if (gl == DRUM_SEQUENCER_GROUP_LENGTH_DEFAULT) 
        return local.drumSequencer.numNotes;
    if (gl > local.drumSequencer.numNotes)      /// uh... that's an error
        gl = local.drumSequencer.numNotes;
    else return gl;
    }

// Get the note speed (0, 1, 2, 3)
// Note speeds are 0=DEFAULT, 1 = 2x, 2 = 4x, 3 = 1/2x  [maybe?]
uint8_t getNoteSpeed(uint8_t group)
    {
    uint8_t gt = GET_GROUP(group);
    // note speed is the low 2 bits
    return (gt & 3);
    }

// Set the note speed (0, 1, 2, 3)
// Note speeds are 0=DEFAULT, 1 = 2x, 2 = 4x, 3 = 1/2x  [maybe?]
void setNoteSpeed(uint8_t group, uint8_t noteSpeed)
    {
    uint8_t gt = GET_GROUP(group);
    // note speed is the low 2 bits
    gt = (gt & (63 << 2)) | noteSpeed;
    SET_GROUP(group, gt);
    }

// Get the MIDI channel for a track.  Channels are 0 = Off, 1...16, 17 = Default, 18 = Choke, 19 = Self Choke
uint8_t getMIDIChannel(uint8_t track)
    {
    uint8_t gt = GET_TRACK0(track);
    // MIDI Channel is the high 5 bits of byte 0
    return (gt >> 3);
    }

// Set the MIDI channel for a track.  Channels are 0 = Off, 1...16, 17 = Default
void setMIDIChannel(uint8_t track, uint8_t channel)
    {
    if (track != local.drumSequencer.numTracks - 1 && // not accent track
        channel >= DRUM_SEQUENCER_MIDI_ACCENT) // uh oh
        {
        channel = 0;
        }
        
    if (track == 0 && (channel == DRUM_SEQUENCER_MIDI_CHOKE || channel == DRUM_SEQUENCER_MIDI_SELF_CHOKE))
    	{
    	channel = 0;
    	}

    uint8_t gt = GET_TRACK0(track);
    // MIDI Channel is the high 5 bits of byte 0
    gt = (gt & 7) | (channel << 3);
    SET_TRACK0(track, gt);
    }

// Get the note velocity (volume) for a track.  Legal values are 0, 1, 2, 3, 4, 5, 6, 7 representing MIDI 15, 31, 47, 63, 79, 95, 111, 127
uint8_t getNoteVelocity(uint8_t track)
    {
    uint8_t gt = GET_TRACK0(track);
    // Note Velocity is the low 3 bits of byte 0
    return (gt & 7);
    }

// Get the actual MIDI note velocity (volume) for a note velocity
uint8_t getNoteMIDIVelocity(uint8_t noteVelocity)
    {
    if (options.drumSequencerLinearCurve)
        {
        // This list has several nice features:
        // 1. It starts reasonably low
        // 2. It includes 127, which is the maximum MIDI velocity
        // 3. It includes 64, which is the default MIDI velocity
        // 4. It has round and understandable numbers
        // 5. It increases by roughly 1.25 each step, and 1.25 * 1.25 is approximately 1.6x, and 1.25 * 1.25 * 1.25 (here) is 2x
        const uint8_t midiVelocities[8] = { 25, 32, 40, 50, 64, 80, 100, 127 };
        return midiVelocities[noteVelocity];
        }
    else
        {
        // This list has several nice features:
        // 1. It starts reasonably low
        // 2. It includes 127, which is the maximum MIDI velocity
        // 3. It includes 64, which is the default MIDI velocity
        // 4. It has round and understandable numbers
        // 5. It linearly increases, for drum machines where MIDI velocity already corresponds to exponential increase.
        //    Note that this is not quite evenly spread: that would be 15, 31, ..., 127; but this would not include 64.
        const uint8_t midiVelocities[8] = { 16, 32, 48, 64, 80, 96, 112, 127 };
        return midiVelocities[noteVelocity];
        }
    }

// Set the note velocity (volume) for a track.  Legal values are 0, 1, 2, 3, 4, 5, 6, 7 representing MIDI 25, 32, 40, 50, 64, 80, 100, 127
void setNoteVelocity(uint8_t track, uint8_t velocity)
    {
    uint8_t gt = GET_TRACK0(track);
    // Note Velocity is the low 3 bits of byte 0
    gt = (gt & 0xF8) | velocity;                                            // 0xF8 is 11111000
    SET_TRACK0(track, gt);
    }

// Get the note pitch for a track.  Legal values are 0...127
uint8_t getNotePitch(uint8_t track)
    {
    uint8_t gt = GET_TRACK1(track);
    // Note Pitch is the high 7 bits of byte 1
    return (gt >> 1);
    }

// Set the note pitch for a track.  Legal values are 0...127
void setNotePitch(uint8_t track, uint8_t pitch)
    {
    uint8_t gt = GET_TRACK1(track);
    // Note Pitch is the high 7 bits of byte 1
    gt = (gt & 1) | (pitch << 1);
    SET_TRACK1(track, gt);
    }

// Get the mute for a track (0 or 1)
uint8_t getMute(uint8_t track)
    {
    uint8_t gt = GET_TRACK1(track);
    // Mute is the low bit of byte 1
    return (gt & 1);
    }

// Set the mute for a track (0 or 1)
void setMute(uint8_t track, uint8_t mute)
    {
    uint8_t gt = GET_TRACK1(track);
    // Mute is the low bit of byte 1
    gt = (gt & 0xFE) | mute;                         // 0xFE is 11111110
    SET_TRACK1(track, gt);
    }

// For a given format (layout), returns the standard (full) number of notes per group.  This can be shortened with getGroupLength
uint8_t numFormatNotes(uint8_t format)
    {
    const uint8_t notes[DRUM_SEQUENCER_NUM_FORMATS] = { 8, 16, 32, 8, 16, 32, 64, 8, 16, 32, 64, 16, 32, 64, 32, 64 };
    return notes[format];
    } 

// For a given format (layout), returns the number of tracks
uint8_t numFormatTracks(uint8_t format)
    {
//    const uint8_t tracks[DRUM_SEQUENCER_NUM_FORMATS] = { 20, 20, 20, 16, 16, 16, 16, 12, 12, 12, 12, 8, 8, 8, 6, 4 };
    const uint8_t tracks[DRUM_SEQUENCER_NUM_FORMATS] = { 20, 20, 22, 16, 16, 16, 18, 14, 12, 12, 12, 8, 8, 8, 6, 4 };
    return tracks[format];
    } 

// For a given format (layout), returns the number of groups
uint8_t numFormatGroups(uint8_t format)
    {
    // Note that we return 15 even though we could fit 17
    const uint8_t groups[DRUM_SEQUENCER_NUM_FORMATS] = { 10, 6, 3, 13, 8, 4, 2, 15, 11, 6, 3, 15, 9, 5, 12, 10 };
    return groups[format];
    } 

/// Clears the notes in the current group
void clearCurrentGroup()
    {
    for(uint8_t i = 0; i < local.drumSequencer.numTracks; i++)
        {
        clearNotes(local.drumSequencer.currentGroup, i);
        }
    }

uint8_t trackIsClear(uint8_t group, uint8_t track)
    {
    for(uint8_t i = 0 ; i < local.drumSequencer.numNotes; i++)
        {
        if (getNote(group, track, i))   // not clear
            return false;
        }
    return true;
    }
        
/// Clears the notes in the current track in the current group
/// If the track in the group is entirely empty already, then with the
/// given per-step randomize probability, the track will be randomized instead.
void clearCurrentTrackInGroup()
    {
    if (options.drumSequencerRandomize && trackIsClear(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack))
        {
        for(uint8_t i = 0 ; i < local.drumSequencer.numNotes; i++)
            {
            if (random(0, DRUM_SEQUENCER_MAX_RANDOM) < options.drumSequencerRandomize)
                {
                setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, i);
                }
            else
                {
                clearNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, i);
                }
            }
        }
    else
        {
        clearNotes(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack);
        }
    }









/////// INITIALIZATION


// clears the mark but not the mark transition
void clearMark()
    {
    local.drumSequencer.markGroup = DRUM_SEQUENCER_NO_MARK;
    local.drumSequencer.markPosition = DRUM_SEQUENCER_NO_MARK;
    local.drumSequencer.markTrack = DRUM_SEQUENCER_NO_MARK;
    }

void initDrumSequencer(uint8_t format)
    {
    data.slot.type = SLOT_TYPE_DRUM_SEQUENCER;              // meh, no reason to set this but...

    // set these first so the later computations work right
    local.drumSequencer.format = format;
    local.drumSequencer.fillGroup = 1;
    local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_OFF;
    local.drumSequencer.numGroups = numFormatGroups(local.drumSequencer.format);
    local.drumSequencer.numTracks = numFormatTracks(local.drumSequencer.format);
    local.drumSequencer.numNotes = numFormatNotes(local.drumSequencer.format);
    local.drumSequencer.currentGroup = 0;
    local.drumSequencer.currentTrack = 0;
    local.drumSequencer.currentTransition = 0;
    local.drumSequencer.currentEditPosition = 0;
    local.drumSequencer.currentPlayPosition = 0;
    local.drumSequencer.repeatSequence = DRUM_SEQUENCER_SEQUENCE_REPEAT_LOOP;
    local.drumSequencer.nextSequence = DRUM_SEQUENCER_NEXT_SEQUENCE_END;
    local.drumSequencer.solo = 0;
    local.drumSequencer.playState = PLAY_STATE_STOPPED;
    local.drumSequencer.performanceMode = false;
    local.drumSequencer.transitionCountdown = 255;
    local.drumSequencer.sequenceCountdown = 255;
    local.drumSequencer.patternCountup = 255;
    // backups don't matter
    local.drumSequencer.goNextTransition = false;
    local.drumSequencer.goNextSequence = false;
    local.drumSequencer.lastExclusiveTrack = NO_TRACK;
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;
    clearMark();
    
    for(uint8_t t = 0; t < numFormatTracks(format); t++)
        {
        // per-track data
        setMIDIChannel(t, DRUM_SEQUENCER_MIDI_OUT_DEFAULT);
        setNoteVelocity(t, options.drumSequencerDefaultVelocity);
        setNotePitch(t, DRUM_SEQUENCER_INITIAL_NOTE_PITCH + t);
        setMute(t, 0);
        
        for(uint8_t g = 0; g < numFormatGroups(format); g++)
            {
            // per-track, per-group data
            setPattern(g, t, DRUM_SEQUENCER_PATTERN_ALL);
            // all notes
            clearNotes(g, t);
            }
        }
        
    for(uint8_t g = 0; g < numFormatGroups(format); g++)
        {
        // per-group data
        setGroupLengthData(g, DRUM_SEQUENCER_GROUP_LENGTH_DEFAULT);
        setNoteSpeed(g, DRUM_SEQUENCER_NOTE_SPEED_DEFAULT);     
        }
    
    // The initial transition pattern is:
    // 1 LOOP
    // 2 1
    // 3 1
    // ....
    // 15 BIG LOOP
    // END
    // ...
    // END
    // ...
    for(uint8_t i = 0; i < DRUM_SEQUENCER_NUM_TRANSITIONS; i++)
        {
        if (i == 0)
            {
            local.drumSequencer.transitionGroup[i] = (i);
            local.drumSequencer.transitionRepeat[i] = DRUM_SEQUENCER_SEQUENCE_REPEAT_LOOP;
            }
        else if (i == local.drumSequencer.numGroups - 1)
            {
            local.drumSequencer.transitionGroup[i] = (i);
            local.drumSequencer.transitionRepeat[i] = DRUM_SEQUENCER_TRANSITION_REPEAT_BIG_LOOP;
            }
        else if (i < local.drumSequencer.numGroups)
            {
            local.drumSequencer.transitionGroup[i] = (i);
            local.drumSequencer.transitionRepeat[i] = 1;
            }
        else
            {
            local.drumSequencer.transitionGroup[i] = DRUM_SEQUENCER_TRANSITION_GROUP_OTHER;
            local.drumSequencer.transitionRepeat[i] = DRUM_SEQUENCER_TRANSITION_OTHER_END;
            }
        }

    for(uint8_t i = 0; i < local.drumSequencer.numTracks; i++)
        {
        local.drumSequencer.muted[i] = (getMute(i) ? DRUM_SEQUENCER_MUTED : DRUM_SEQUENCER_NOT_MUTED);
        local.drumSequencer.shouldPlay[i] = 1;  // doesn't matter
        }
    }



// Reformats the sequence as requested by the user.  
void stateDrumSequencerFormat()
    {
    uint8_t result;
    defaultMenuValue = DRUM_SEQUENCER_DEFAULT_FORMAT;
//    const char* menuItems[16] = {  PSTR("8/10/20"), PSTR("16/6/20"), PSTR("32/3/20"), PSTR("8/13/16"), PSTR("16/8/16"), PSTR("32/4/16"), PSTR("64/2/16"), PSTR("8/15/12"), PSTR("16/11/12"), PSTR("32/6/12"), PSTR("64/3/12"), PSTR("16/15/8"), PSTR("32/9/8"), PSTR("64/5/8"), PSTR("32/12/6"), PSTR("64/10/4") }; 
    const char* menuItems[16] = {  PSTR("8/10/20"), PSTR("16/6/20"), PSTR("32/3/22"), PSTR("8/13/16"), PSTR("16/8/16"), PSTR("32/4/18"), PSTR("64/2/18"), PSTR("8/15/14"), PSTR("16/11/12"), PSTR("32/6/12"), PSTR("64/3/12"), PSTR("16/15/8"), PSTR("32/9/8"), PSTR("64/5/8"), PSTR("32/12/6"), PSTR("64/10/4") }; 
    result = doMenuDisplay(menuItems, 16, STATE_NONE, 0, 1);
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
            local.drumSequencer.goNextTransition = false;
            local.drumSequencer.goNextSequence = false;
            local.drumSequencer.scheduleStop = false;
            local.drumSequencer.solo = 0;
            setNotePulseRate(options.noteSpeedType);
            goDownState(STATE_DRUM_SEQUENCER_FORMAT_NOTE);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_DRUM_SEQUENCER);
            }
        break;
        }
    }

void stateDrumSequencerFormatNote()
    {
    uint8_t note = stateEnterNote(STATE_DRUM_SEQUENCER_FORMAT);
    if (note != NO_NOTE)
        {
        // distribute notes
        for(uint8_t i = 0; i < local.drumSequencer.numTracks; i++)
            {
            setNotePitch(i, note);
            if (note < 255) note++;
            }
        goDownState(STATE_DRUM_SEQUENCER_PLAY);
        }
    }



//// Resets the countdown for repeating a group in a transition
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
        case 10: local.drumSequencer.transitionCountdown = 11; break;
        case 11: local.drumSequencer.transitionCountdown = 15; break;
        case 12: local.drumSequencer.transitionCountdown = 23; break;
        case 13: local.drumSequencer.transitionCountdown = 31; break;
        case 14: local.drumSequencer.transitionCountdown = 63; break;
        case 15: local.drumSequencer.transitionCountdown = 255; break;          // big loop
        }
    local.drumSequencer.patternCountup = 255;
    }
    
    
//// Resets the countdown for repeating a sequence
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

    

/////// PACKING AND UNPACKING

/// These functions load/unload data into data.slot.data.drumSequencer and prepare it to be stored in flash.
/// In fact we're only loading/uloading a limited amount of data -- the rest of the data we're accessing directly
/// in data.slot.data.drumSequencer.

void packDrumSequenceData()
    {
    data.slot.data.drumSequencer.format = (local.drumSequencer.format | (local.drumSequencer.fillGroup << 4));
    data.slot.data.drumSequencer.repeat = (local.drumSequencer.nextSequence | (local.drumSequencer.repeatSequence << 4));
    for(uint8_t i = 0; i < DRUM_SEQUENCER_NUM_TRANSITIONS; i++)
        {
        data.slot.data.drumSequencer.transition[i] = (local.drumSequencer.transitionGroup[i]) | (local.drumSequencer.transitionRepeat[i] << 4);
        }
    for(uint8_t i = 0; i < local.drumSequencer.numTracks; i++)
        {
        setMute(i, local.drumSequencer.muted[i] == DRUM_SEQUENCER_MUTED ? 1 : 0);
        }
    }


void unpackDrumSequenceData()
    {
    // initialization
    // set these first so the later computations work right
    local.drumSequencer.format = (data.slot.data.drumSequencer.format & 0x0F);
    local.drumSequencer.fillGroup = (data.slot.data.drumSequencer.format >> 4);
    local.drumSequencer.numGroups = numFormatGroups(local.drumSequencer.format);
    local.drumSequencer.numTracks = numFormatTracks(local.drumSequencer.format);
    local.drumSequencer.numNotes = numFormatNotes(local.drumSequencer.format);
    local.drumSequencer.currentGroup = 0;
    local.drumSequencer.currentTrack = 0;
    local.drumSequencer.currentTransition = 0;
    local.drumSequencer.currentEditPosition = 0;
    local.drumSequencer.currentPlayPosition = 0;
    local.drumSequencer.solo = 0;
    local.drumSequencer.transitionCountdown = 255;
    local.drumSequencer.sequenceCountdown = 255;
    local.drumSequencer.patternCountup = 255;
    // backups don't matter
    local.drumSequencer.goNextTransition = false;
    local.drumSequencer.goNextSequence = false;
    local.drumSequencer.lastExclusiveTrack = NO_TRACK;
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;
    clearMark();
        
    // unpacking
    local.drumSequencer.nextSequence = (data.slot.data.drumSequencer.repeat & 0x0F);
    local.drumSequencer.repeatSequence = (data.slot.data.drumSequencer.repeat >> 4);
    for(uint8_t i = 0; i < DRUM_SEQUENCER_NUM_TRANSITIONS; i++)
        {
        local.drumSequencer.transitionGroup[i] = (data.slot.data.drumSequencer.transition[i] & 0x0F);
        local.drumSequencer.transitionRepeat[i] = (data.slot.data.drumSequencer.transition[i] >> 4);
        }
    for(uint8_t i = 0; i < local.drumSequencer.numTracks; i++)
        {
        local.drumSequencer.muted[i] = (getMute(i) ? DRUM_SEQUENCER_MUTED : DRUM_SEQUENCER_NOT_MUTED);
        local.drumSequencer.shouldPlay[i] = 1;  // doesn't matter
        }
    }


// This is a slightly modified version of the code in stateLoad()... FIXME: Merge them?
void loadDrumSequence(uint8_t slot)
    {
    if (getSlotType(slot) != slotTypeForApplication(STATE_DRUM_SEQUENCER))
        {
        stopDrumSequencer();
        }
    else
        {
        loadSlot(slot);
        unpackDrumSequenceData();               // will this reset too much stuff?
        resetDrumSequencerSequenceCountdown();          // do I need this?
        }
    local.drumSequencer.scheduleStop = false;
    }
           




      
// I'd prefer the following code as it creates a bit of a buffer so scrolling to position 0 doesn't go straight
// into play position mode.  Or perhaps we should change things so that you scroll to the far RIGHT edge, dunno.
// But anyway we can't do this because adding just a little bit here radically increases our memory footprint. :-(
#define CURSOR_MARGIN (4)

// Figures out what the cursor pos is given the current pot values, plus slop
// NOTE: If tracks are longer than 119, then this is going to overflow (pot[RIGHT_POT] >> 1) * (trackLen + CURSOR_MARGIN * 2)
int16_t drumSequencerGetNewCursorXPos(uint8_t trackLen)
    {
    int16_t val = ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + CURSOR_MARGIN * 2 + 1)) >> 9)) - CURSOR_MARGIN;
    if ((val < 0) && (val > -CURSOR_MARGIN))                                                            // left gutter
        val = 0;
    else if ((val >= trackLen) && (val < trackLen + CURSOR_MARGIN))                     // right gutter, hope I got this right
        val = trackLen - 1;
    return val;
    }




// Resets the sequence playing
void resetDrumSequencer()
    {
    // reset drum sequencer
    local.drumSequencer.currentPlayPosition = getGroupLength(local.drumSequencer.currentGroup) - 1;
    resetDrumSequencerTransitionCountdown();
    
    // FIXME: Not sure if we should do this
    local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_OFF;
    }






////// CHANGING GROUP

// Changes the group, resets the countdown, and resets the play position to the end.
// This is used when you manually change the group in Group mode etc.
void drumSequencerChangeGroup(uint8_t group)
    {       
    local.drumSequencer.currentGroup = group;
    
    // revise the edit position
    uint8_t atRight = (local.drumSequencer.currentEditPosition >= getGroupLength(local.drumSequencer.currentGroup));
    uint8_t trackLen = getGroupLength(local.drumSequencer.currentGroup);
    if (atRight)
        local.drumSequencer.currentEditPosition = trackLen;                             // keep it at right
    else if (local.drumSequencer.currentEditPosition >= trackLen)
        local.drumSequencer.currentEditPosition = trackLen - 1;                 // make it a rational value
        
    resetDrumSequencer();
    }

// Changes the group, resets the countdown, but does NOT reset the play position to the end.
// This is used by the transition facility during performance mode.
void drumSequencerUpdateGroup(uint8_t group)
    {
    uint8_t play = local.drumSequencer.currentPlayPosition;
    drumSequencerChangeGroup(group);
    // because changing the group revises the play position, we reset it here
    local.drumSequencer.currentPlayPosition = play;
    }


//// Checks to see if it's time to transition to the next group.  If so,
//// advances to the next group and begins playing there.  If the next group
//// is in fact the END, then checks to see if it's time to advance to the next
//// sequence. If so, advances to the next sequence and begins playing there.  If
//// not, repeats the current sequence starting with the first transition.

void goNextTransition()
    {
    if (local.drumSequencer.goNextTransition || local.drumSequencer.goNextSequence ||
        (local.drumSequencer.performanceMode && local.drumSequencer.transitionCountdown == 0))
        {
        if (local.drumSequencer.goNextTransition >= 2)          // it's a *specific* transition
            {
            local.drumSequencer.currentTransition = local.drumSequencer.goNextTransition - 2;
            }
        else    //              either goNextTransition is 1 (because we're transitioning to next) or transitionCountdown should move us (so it's 0)
            {
            local.drumSequencer.currentTransition++;
            }
        local.drumSequencer.goNextTransition = false;
                        
        // are we at the end?
        if (local.drumSequencer.currentTransition > DRUM_SEQUENCER_NUM_TRANSITIONS || 
            local.drumSequencer.goNextSequence  ||
                (local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition] == DRUM_SEQUENCER_TRANSITION_GROUP_OTHER &&
                local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition] == DRUM_SEQUENCER_TRANSITION_OTHER_END))
            {
            if (local.drumSequencer.sequenceCountdown == 0 || local.drumSequencer.goNextSequence)
                {
                uint8_t nextSequence = local.drumSequencer.nextSequence;
                if (nextSequence == DRUM_SEQUENCER_NEXT_SEQUENCE_END)  // STOP
                    {
                    stopDrumSequencer(); 
                    }
                else            // maybe this will work out of the box?  hmmm
                    { 
                    loadDrumSequence(nextSequence - 1); 
                    resetDrumSequencerTransitionCountdown();                        // FIXME -- do I need this?
                    }  
                }
            // This should never happen.  We can't have END as the first START.  This will be interpreted
            // as repeating group 1 in a loop
            else if (local.drumSequencer.currentTransition == 0)
                {
                // do nothing
                drumSequencerUpdateGroup(0);
                }
            else if (local.drumSequencer.sequenceCountdown == 255)  // loop forever
                {
                local.drumSequencer.currentTransition = 0;
                drumSequencerUpdateGroup(local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition]);
                }       
            else
                {
                local.drumSequencer.sequenceCountdown--;
                local.drumSequencer.currentTransition = 0;
                drumSequencerUpdateGroup(local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition]);
                }
            }                       
        // are we picking a group at random?
        else if (local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition] == DRUM_SEQUENCER_TRANSITION_GROUP_OTHER &&
            local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition] != DRUM_SEQUENCER_TRANSITION_OTHER_END)
            {
            // Groups are going to be either 1-2, 1-3, or 1-4
            uint8_t grouptype = div5(local.drumSequencer.transitionRepeat - 1);             // remove END
            // repeats are LOOP, 1, 2, 3, or 4
            uint8_t repeat = DIV5_REMAINDER(grouptype, local.drumSequencer.transitionRepeat - 1);           // remove END
            // Pick a group
            uint8_t maxgroups = grouptype + 2;
            if (maxgroups > local.drumSequencer.numGroups)
                maxgroups = local.drumSequencer.numGroups;                      // make sure we're not picking an illegal group
            uint8_t group = random(0, maxgroups);
            drumSequencerUpdateGroup(group);
            // override the countdown which was set by resetDrumSequencerTransitionCountdown() called by drumSequencerUpdateGroup()
            if (repeat == 0)                // LOOP
                {
                local.drumSequencer.transitionCountdown = 255;
                }
            else
                {
                local.drumSequencer.transitionCountdown = repeat - 1;
                }
            }
        // just grab the group and repeat
        else
            {
            drumSequencerUpdateGroup(local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition]);
            }
        }
    else if (local.drumSequencer.performanceMode && local.drumSequencer.transitionCountdown != 255)
        {
        local.drumSequencer.transitionCountdown--;
                
        if (local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition] == DRUM_SEQUENCER_TRANSITION_GROUP_OTHER &&
            local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition] != DRUM_SEQUENCER_TRANSITION_OTHER_END)
            {
            // gotta pick a new random group
            uint8_t grouptype = div5(local.drumSequencer.transitionRepeat);
            uint8_t group = random(0, grouptype + 2);
            drumSequencerUpdateGroup(local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition]);
            }
        }
    else if (local.drumSequencer.performanceMode && local.drumSequencer.transitionCountdown == 255 && 
        local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition] != DRUM_SEQUENCER_TRANSITION_GROUP_OTHER &&
        local.drumSequencer.transitionRepeat[local.drumSequencer.currentTransition] == DRUM_SEQUENCER_TRANSITION_REPEAT_BIG_LOOP)                       // Big Loop
        {
        // It's a big loop, where do we go?
        uint8_t newTransition = 0;
        for(int8_t i = local.drumSequencer.currentTransition - 1; i >= 0; i--)          // notice signed
            {
            // did we find a former loop transition?
            if (local.drumSequencer.transitionGroup[i] == DRUM_SEQUENCER_TRANSITION_GROUP_OTHER ||          // end or random repeat loop
                    (local.drumSequencer.transitionGroup[i] != DRUM_SEQUENCER_TRANSITION_GROUP_OTHER &&
                        (local.drumSequencer.transitionRepeat[i] == DRUM_SEQUENCER_TRANSITION_REPEAT_BIG_LOOP ||                // big loop
                        local.drumSequencer.transitionRepeat[i] == DRUM_SEQUENCER_TRANSITION_REPEAT_LOOP)))                             // normal loop
                {
                newTransition = i + 1;
                break;
                }
            }
        local.drumSequencer.currentTransition = newTransition;
        drumSequencerUpdateGroup(local.drumSequencer.transitionGroup[local.drumSequencer.currentTransition]);
        }
    else
        {
        // Loop forever
        }
    }
        

//// Schedules a track to be muted or unmuted starts muting it (or unmuting it)

void drumSequencerAdvanceFillOrMute(uint8_t track, uint8_t action)
    {
    if (action == DRUM_SEQUENCER_ACTION_FILL || (action == DRUM_SEQUENCER_ACTION_EITHER && options.drumSequencerFill))
        {
        if (local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_OFF)
            {
            local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_SCHEDULED;
            }
        else if (local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_ON)
            {
            // do nothing
            }
        else if (local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_SCHEDULED)
            {
            local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_OFF;
            }
        }
    else
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
    }


//// Schedules soloing to begin or end

void drumSequencerAdvanceSolo()
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
    




// Sets the mark
void stateDrumSequencerMenuEditMark()
    {
    local.drumSequencer.markTrack = local.drumSequencer.currentTrack;
    local.drumSequencer.markGroup = local.drumSequencer.currentGroup;
    local.drumSequencer.markPosition = local.drumSequencer.currentEditPosition;
    if (local.drumSequencer.markPosition < 0)
        local.drumSequencer.markPosition = 0;
    uint8_t len = getGroupLength(local.drumSequencer.currentGroup);
    if (local.drumSequencer.markPosition > len)
        local.drumSequencer.markPosition = len;
    goUpState(STATE_DRUM_SEQUENCER_PLAY);
    playDrumSequencer();
    }
        
// Swaps two groups
void stateDrumSequencerMenuSwapGroup()
    {
    uint8_t fromGroup = local.drumSequencer.markGroup;
    uint8_t toGroup = local.drumSequencer.currentGroup;
    if (fromGroup == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        resetDrumSequencer();
        for(uint8_t t = 0; t < local.drumSequencer.numTracks; t++)
            {
            // exchange notes
            for(uint8_t n = 0; n < local.drumSequencer.numNotes; n++)                               // This is okay instead of getGroupLength()
                {
                uint8_t fromNote = getNote(fromGroup, t, n);
                uint8_t toNote = getNote(toGroup, t, n);
                setNote(fromGroup, t, n, toNote);
                setNote(toGroup, t, n, fromNote);
                }
            // exchange patterns
            uint8_t fromPattern = getPattern(fromGroup, t);
            uint8_t toPattern = getPattern(toGroup, t);
            setPattern(fromGroup, t, toPattern);
            setPattern(toGroup, t, fromPattern);
            }
        // exchange group lengths
        uint8_t fromGL = getGroupLengthData(fromGroup);
        uint8_t toGL = getGroupLengthData(toGroup);
        setGroupLengthData(fromGroup, toGL);
        setGroupLengthData(toGroup, fromGL);
        // exchange note speeds
        uint8_t fromSpeed = getNoteSpeed(fromGroup);
        uint8_t toSpeed = getNoteSpeed(toGroup);
        setNoteSpeed(fromGroup, toSpeed);
        setNoteSpeed(toGroup, fromSpeed);

        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        clearMark();
        }
    playDrumSequencer();
    }



void drumSequencerUnroll(uint8_t group, uint8_t track, uint8_t amount, uint8_t from, uint8_t once)
    {
    if (once)
        {
        uint8_t to = from + amount;
        if (to >= local.drumSequencer.numNotes) to = 0; 
        // this would be more efficient with GET_NOTES but whatever
        for(uint8_t i = 0; i < amount; i++)
            {
            setNote(group, track, to + i, getNote(group, track, from + i));
            }
        }
    else
        {
        for(uint8_t to = 0; to < local.drumSequencer.numNotes; to += amount)
            {
            if (to == from) continue;
            // this would be more efficient with GET_NOTES but whatever
            for(uint8_t i = 0; i < amount; i++)
                {
                setNote(group, track, to + i, getNote(group, track, from + i));
                }
            }
        }
    }

void jointStateDrumSequencerMenuUnroll(uint8_t wholeGroup)
    {
    const char* menuItems[6] = {  PSTR("8"), PSTR("8 All"), PSTR("16"), PSTR("16 All"), PSTR("32"), PSTR("32 All") };
    if (entry) 
        {
        defaultMenuValue = 2;
        }
    uint8_t result = doMenuDisplay(menuItems, local.drumSequencer.numNotes == 16 ? 2 : (local.drumSequencer.numNotes == 32 ? 4 : 6), STATE_NONE, STATE_NONE, 1);
                
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
            uint8_t amt = (currentDisplay < 2 ? 8 : (currentDisplay < 4 ? 16 : 32));
            uint8_t once = (currentDisplay == 0 || currentDisplay == 2 || currentDisplay == 6);
            
            uint8_t from = (local.drumSequencer.currentEditPosition < 0 ? 0 : 
                    (amt == 8 ? local.drumSequencer.currentEditPosition >> 3 :
                    (amt == 16 ? local.drumSequencer.currentEditPosition >> 4 : local.drumSequencer.currentEditPosition >> 5)));

            if (wholeGroup)
                {
                for(uint8_t t = 0; t < local.drumSequencer.numTracks; t++)
                    {
                    drumSequencerUnroll(local.drumSequencer.currentGroup, t, amt, from, once);
                    }
                }
            else
                {
                drumSequencerUnroll(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, amt, from, once);
                }
                                
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    }


void stateDrumSequencerMenuUnrollGroup()
    {
    const char* menuItems[6] = {  PSTR("8"), PSTR("8 All"), PSTR("16"), PSTR("16 All"), PSTR("32"), PSTR("32 All") };
    if (entry) 
        {
        defaultMenuValue = 2;
        }
    uint8_t result = doMenuDisplay(menuItems, local.drumSequencer.numNotes == 16 ? 2 : (local.drumSequencer.numNotes == 32 ? 4 : 6), STATE_NONE, STATE_NONE, 1);
                
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
            uint8_t amt = (currentDisplay < 2 ? 8 : (currentDisplay < 4 ? 16 : 32));
            uint8_t once = (currentDisplay == 0 || currentDisplay == 2 || currentDisplay == 6);
            
            uint8_t from = (local.drumSequencer.currentEditPosition < 0 ? 0 : 
                    (amt == 8 ? local.drumSequencer.currentEditPosition >> 3 :
                    (amt == 16 ? local.drumSequencer.currentEditPosition >> 4 : local.drumSequencer.currentEditPosition >> 5)));

            for(uint8_t t = 0; t < local.drumSequencer.numTracks; t++)
                {
                drumSequencerUnroll(local.drumSequencer.currentGroup, t, amt, from, once);
                }
                                
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    }


// Copies two groups
void stateDrumSequencerMenuCopyGroups(uint8_t fromGroup, uint8_t toGroup)
    {
    if (fromGroup == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        resetDrumSequencer();
        for(uint8_t t = 0; t < local.drumSequencer.numTracks; t++)
            {
            // exchange notes
            for(uint8_t n = 0; n < local.drumSequencer.numNotes; n++)                       // This is okay instead of getGroupLength()
                {
                uint8_t fromNote = getNote(fromGroup, t, n);
                setNote(toGroup, t, n, fromNote);
                }
            // exchange patterns
            uint8_t fromPattern = getPattern(fromGroup, t);
            setPattern(toGroup, t, fromPattern);
            }
        // exchange group lengths
        uint8_t fromGL = getGroupLengthData(fromGroup);
        setGroupLengthData(toGroup, fromGL);
        // exchange note speeds
        uint8_t fromSpeed = getNoteSpeed(fromGroup);
        setNoteSpeed(toGroup, fromSpeed);

        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }

// Copies two groups
void stateDrumSequencerMenuCopyGroup()
    {
    uint8_t fromGroup = local.drumSequencer.markGroup;
    uint8_t toGroup = local.drumSequencer.currentGroup;
    stateDrumSequencerMenuCopyGroups(fromGroup, toGroup);
    clearMark();
    playDrumSequencer();
    }
        
// Copies a group to the next one
void stateDrumSequencerCopyGroupToNext()
    {
    uint8_t fromGroup = local.drumSequencer.currentGroup;
    if (fromGroup >= local.drumSequencer.numGroups - 1)     // not enough room
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        stateDrumSequencerMenuCopyGroups(fromGroup, fromGroup + 1);
        drumSequencerChangeGroup(fromGroup + 1);
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        clearMark();
        }
    playDrumSequencer();
    }




// Swaps specific tracks within two groups
void stateDrumSequencerMenuSwapGroupTracks()
    {
    uint8_t fromGroup = local.drumSequencer.markGroup;
    uint8_t toGroup = local.drumSequencer.currentGroup;
    uint8_t fromTrack = local.drumSequencer.markTrack;
    uint8_t toTrack = local.drumSequencer.currentTrack;
    if (fromTrack == DRUM_SEQUENCER_NO_MARK || fromGroup == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        // don't need to stop the sequencer
        // copy notes
        for(uint8_t n = 0; n < local.drumSequencer.numNotes; n++)                                       // This is okay instead of getGroupLength()
            {
            uint8_t fromNote = getNote(fromGroup, fromTrack, n);
            uint8_t toNote = getNote(toGroup, toTrack, n);
            setNote(toGroup, toTrack, n, fromNote);
            setNote(fromGroup, fromTrack, n, toNote);
            }
        // copy patterns
        uint8_t fromPattern = getPattern(fromGroup, fromTrack);
        uint8_t toPattern = getPattern(toGroup, toTrack);
        setPattern(toGroup, toTrack, fromPattern);
        setPattern(fromGroup, fromTrack, toPattern);

        clearMark();
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }


// Copies specific track notes within two groups
void stateDrumSequencerMenuCopyGroupTrack()
    {
    uint8_t fromGroup = local.drumSequencer.markGroup;
    uint8_t toGroup = local.drumSequencer.currentGroup;
    uint8_t fromTrack = local.drumSequencer.markTrack;
    uint8_t toTrack = local.drumSequencer.currentTrack;
    if (fromTrack == DRUM_SEQUENCER_NO_MARK || fromGroup == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        // don't need to stop the sequencer
        // copy notes
        for(uint8_t n = 0; n < local.drumSequencer.numNotes; n++)                                       // This is okay instead of getGroupLength()
            {
            uint8_t fromNote = getNote(fromGroup, fromTrack, n);
            setNote(toGroup, toTrack, n, fromNote);
            }
        // copy patterns
        uint8_t fromPattern = getPattern(fromGroup, fromTrack);
        setPattern(toGroup, toTrack, fromPattern);

        clearMark();
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }

uint8_t isAccentTrack(uint8_t track)
    {
    return (track == local.drumSequencer.numTracks - 1) && (getMIDIChannel(track) >= DRUM_SEQUENCER_MIDI_ACCENT);
    }
        
// Swaps two tracks, including info
void stateDrumSequencerMenuSwapTracks()
    {
    uint8_t fromTrack = local.drumSequencer.markTrack;
    uint8_t toTrack = local.drumSequencer.currentTrack;
    if (fromTrack == DRUM_SEQUENCER_NO_MARK || isAccentTrack(fromTrack) || isAccentTrack(toTrack))
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        // don't need to stop the sequencer
        // copy notes
        for(uint8_t g = 0; g < local.drumSequencer.numGroups; g++)
            {
            for(uint8_t n = 0; n < local.drumSequencer.numNotes; n++)               // this is okay instead of getGroupLength()
                {
                uint8_t fromNote = getNote(g, fromTrack, n);
                uint8_t toNote = getNote(g, toTrack, n);
                setNote(g, toTrack, n, fromNote);
                setNote(g, fromTrack, n, toNote);
                }
            // copy patterns
            uint8_t fromPattern = getPattern(g, fromTrack);
            uint8_t toPattern = getPattern(g, toTrack);
            setPattern(g, toTrack, fromPattern);
            setPattern(g, fromTrack, toPattern);
            }
        uint8_t fromMIDIChannel = getMIDIChannel(fromTrack);
        uint8_t toMIDIChannel = getMIDIChannel(toTrack);
        setMIDIChannel(toTrack, fromMIDIChannel);
        setMIDIChannel(fromTrack, toMIDIChannel);
        uint8_t fromNoteVelocity = getNoteVelocity(fromTrack);
        uint8_t toNoteVelocity = getNoteVelocity(toTrack);
        setNoteVelocity(toTrack, fromNoteVelocity);
        setNoteVelocity(fromTrack, toNoteVelocity);
        uint8_t fromNotePitch = getNotePitch(fromTrack);
        uint8_t toNotePitch = getNotePitch(toTrack);
        setNotePitch(toTrack, fromNotePitch);
        setNotePitch(fromTrack, toNotePitch);
        uint8_t fromMute = local.drumSequencer.muted[fromTrack];
        uint8_t toMute = local.drumSequencer.muted[toTrack];
        local.drumSequencer.muted[toTrack] = fromNotePitch;
        local.drumSequencer.muted[fromTrack] = toNotePitch;

        clearMark();
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }



// Rotates a track.  The mark must be correct, and the edit position must be >= 0 
void rotateTrack(uint8_t fromTrack)
    {
    uint8_t fromGroup = local.drumSequencer.currentGroup;
    uint8_t fromPos = local.drumSequencer.markPosition;
    uint8_t toPos = (uint8_t) local.drumSequencer.currentEditPosition;
    uint8_t trackLen = getGroupLength(fromGroup);
    
	int8_t rotate = (int8_t)toPos - (int8_t)fromPos;
	if (rotate > 0)
		{
		for(uint8_t i = 0; i < rotate; i++)
			{
			uint8_t lastNote = getNote(fromGroup, fromTrack, trackLen - 1);
			for(uint8_t j = trackLen - 1; j > 0; j--)
				{
				setNote(fromGroup, fromTrack, j, getNote(fromGroup, fromTrack, j - 1));
				}
			setNote(fromGroup, fromTrack, 0, lastNote);
			}
		}
	else
		{
		rotate = 0 - rotate;
		for(uint8_t i = 0; i < rotate; i++)
			{
			uint8_t firstNote = getNote(fromGroup, fromTrack, 0);
			for(uint8_t j = 0; j < trackLen - 1; j++)
				{
				setNote(fromGroup, fromTrack, j, getNote(fromGroup, fromTrack, j + 1));
				}
			setNote(fromGroup, fromTrack, trackLen - 1, firstNote);
			}
		}
	}


void stateDrumSequencerMenuRotateTrack()
    {
    uint8_t fromTrack = local.drumSequencer.markTrack;
    uint8_t fromGroup = local.drumSequencer.markGroup;
    uint8_t fromPos = local.drumSequencer.markPosition;
    int8_t toPos = local.drumSequencer.currentEditPosition;
    
    if (toPos < 0 || 
    		fromTrack != local.drumSequencer.currentTrack ||		// including  DRUM_SEQUENCER_NO_MARK
    		fromGroup != local.drumSequencer.currentGroup ||
    		fromPos == toPos)
    	{
        goDownState(STATE_DRUM_SEQUENCER_CANT);
    	}
	else
		{
		rotateTrack(fromTrack);
        clearMark();
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }

void stateDrumSequencerMenuRotateGroup()
    {
    uint8_t fromTrack = local.drumSequencer.markTrack;
    uint8_t fromGroup = local.drumSequencer.markGroup;
    uint8_t fromPos = local.drumSequencer.markPosition;
    int8_t toPos = local.drumSequencer.currentEditPosition;
    uint8_t numTracks = local.drumSequencer.numTracks;
    
    if (toPos < 0 || 
    		fromTrack == DRUM_SEQUENCER_NO_MARK ||
    		fromGroup != local.drumSequencer.currentGroup ||
    		fromPos == toPos)
    	{
        goDownState(STATE_DRUM_SEQUENCER_CANT);
    	}
	else
		{
		for(uint8_t i = 0; i < numTracks; i++)
			{
			rotateTrack(i);
			}
        clearMark();
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }


// Copies two tracks, including info
void stateDrumSequencerMenuCopyTrack()
    {
    uint8_t fromTrack = local.drumSequencer.markTrack;
    uint8_t toTrack = local.drumSequencer.currentTrack;
    if (fromTrack == DRUM_SEQUENCER_NO_MARK || isAccentTrack(fromTrack) || isAccentTrack(toTrack))
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        // don't need to stop the sequencer
        // copy notes
        for(uint8_t g = 0; g < local.drumSequencer.numGroups; g++)
            {
            for(uint8_t n = 0; n < local.drumSequencer.numNotes; n++)               // this is okay instead of getGroupLength()
                {
                uint8_t fromNote = getNote(g, fromTrack, n);
                setNote(g, toTrack, n, fromNote);
                }
            // copy patterns
            uint8_t fromPattern = getPattern(g, fromTrack);
            setPattern(g, toTrack, fromPattern);
            }

        uint8_t fromMIDIChannel = getMIDIChannel(fromTrack);
        setMIDIChannel(toTrack, fromMIDIChannel);
        uint8_t fromNoteVelocity = getNoteVelocity(fromTrack);
        setNoteVelocity(toTrack, fromNoteVelocity);
        uint8_t fromNotePitch = getNotePitch(fromTrack);
        setNotePitch(toTrack, fromNotePitch);
        uint8_t fromMute = local.drumSequencer.muted[fromTrack];
        local.drumSequencer.muted[toTrack] = fromNotePitch;

        clearMark();
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }


// Copies two tracks, including info
void stateDrumSequencerMenuAccentTrack(int8_t amount)
    {
    uint8_t fromTrack = local.drumSequencer.currentTrack;
    uint8_t toTrack = local.drumSequencer.currentTrack + 1;
        
    //    if (fromTrack == DRUM_SEQUENCER_NO_MARK)
    if (toTrack >= local.drumSequencer.numTracks  || isAccentTrack(fromTrack) || isAccentTrack(toTrack))
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {
        // don't need to stop the sequencer
        uint8_t fromNoteVelocity = getNoteVelocity(fromTrack);
        fromNoteVelocity = fromNoteVelocity + amount;                                // 150%
        if (fromNoteVelocity > 7) fromNoteVelocity = 7;
        if (fromNoteVelocity < 0) fromNoteVelocity = 0;
        setNoteVelocity(toTrack, fromNoteVelocity);
        uint8_t fromNotePitch = getNotePitch(fromTrack);
        setNotePitch(toTrack, fromNotePitch);
        uint8_t fromMIDIChannel = getMIDIChannel(fromTrack);
        setMIDIChannel(toTrack, fromMIDIChannel);
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }


// Distributes track info from current track, except for note pitch
void stateDrumSequencerMenuDistributeTrackInfo()
    {
    if (isAccentTrack(local.drumSequencer.currentTrack) ||
    	getMIDIChannel(local.drumSequencer.currentTrack) == DRUM_SEQUENCER_MIDI_CHOKE || 
    	getMIDIChannel(local.drumSequencer.currentTrack) == DRUM_SEQUENCER_MIDI_SELF_CHOKE)
        {
        goDownState(STATE_DRUM_SEQUENCER_CANT);
        }
    else
        {       
        uint8_t fromMIDIChannel = getMIDIChannel(local.drumSequencer.currentTrack);
        uint8_t fromNoteVelocity = getNoteVelocity(local.drumSequencer.currentTrack);
        for(uint8_t t = 1; t < local.drumSequencer.numTracks; t++)
            {
            if (!isAccentTrack(t))
                {
                setMIDIChannel(t, fromMIDIChannel);
                setNoteVelocity(t, fromNoteVelocity);
                }
            }
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }
        
/// Stops the drum sequencer but does not reset it
void pauseDrumSequencer()
    {
    local.drumSequencer.playState = PLAY_STATE_PAUSED;
    if (1) //if (options.drumSequencerSendClock)
        {
        stopClock(true);
        }
    }




// Draws a transition display.
// We will display:
// 1. The GROUP at left         (or 'R1' etc., or 'E')
// 2. The REPEATS at right
// 3. The GROUP as a range in row 1  [for convenience] 
// 4. The TRANSITION as a range in row 0
// 5. If blink == true, a blinked LINE to the right of the GROUP.
//    This lets us distinguish between two successive transition displays,
//    such as copying from one transition and pasting to another.

uint8_t doTransitionDisplay(uint8_t initialTransition, uint8_t blink)
    {
    if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        options.drumSequencerDisplayGroup = !options.drumSequencerDisplayGroup;
        saveOptions();
        }
    
    uint8_t result = doNumericalDisplay(0, 19, initialTransition, false, GLYPH_NONE);
    if (result == NO_MENU_SELECTED)
        {
        if (updateDisplay)
            {
            // revise the display
            clearScreen();

            uint8_t group = local.drumSequencer.transitionGroup[currentDisplay];
            uint8_t repeat = local.drumSequencer.transitionRepeat[currentDisplay];
            if (group == DRUM_SEQUENCER_TRANSITION_GROUP_OTHER)
                {
                if (repeat == DRUM_SEQUENCER_TRANSITION_OTHER_END)
                    {
                    write3x5Glyph(led2, GLYPH_3x5_E, 0);
                    write3x5Glyph(led2, GLYPH_3x5_N, 4);
                    write3x5Glyph(led, GLYPH_3x5_D, 0);
                    }
                else            // value 0 ... 14
                    {
                    uint8_t grps = div5(repeat);
                    uint8_t reps = DIV5_REMAINDER(grps, repeat);
                                                                
                    write3x5Glyph(led2, GLYPH_3x5_R, 0);
                    write3x5Glyph(led2, GLYPH_3x5_2 + grps, 4);
                                                                
                    if (reps == 4)  // L
                        {
                        write8x5Glyph(led, GLYPH_8x5_INFINITY);
                        }
                    else
                        {
                        write3x5Glyph(led, GLYPH_3x5_1 + reps, 4);
                        }
                    }
                }
            else
                {
                if (options.drumSequencerDisplayGroup)
                    {
                    drawDrumSequencer(group, false);
                    }
                else
                    {
#ifdef TWO_SCREENS_VERTICAL
                    clearMatrix(led3);
                    clearMatrix(led4);
                    drawDrumSequencer(group, false, true);          // draw just the top
#endif TWO_SCREENS_VERTICAL
                    writeShortNumber(led2, (group + 1), false);
                    if (repeat == DRUM_SEQUENCER_TRANSITION_REPEAT_LOOP)
                        {
                        write8x5Glyph(led, GLYPH_8x5_INFINITY);
                        }
                    else if (repeat == DRUM_SEQUENCER_TRANSITION_REPEAT_BIG_LOOP)
                        {
                        write3x5Glyph(led, GLYPH_3x5_B, 1);
                        write3x5Glyph(led, GLYPH_3x5_L, 5);
                        }
                    else
                        {
                        const uint8_t repeats[14] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 24, 32, 64 };
                        writeShortNumber(led, repeats[repeat - 1], false);
                        }
                    }
                                        
                if (repeat == 0) repeat = 16;
                drawRange(led, 0, 1, MAX_DRUM_SEQUENCER_REPEATS, repeat - 1);
                drawRange(led2, 0, 1, MAX_DRUM_SEQUENCER_GROUPS, group);
                }

            drawRange(led2, 0, 0, DRUM_SEQUENCER_NUM_TRANSITIONS, currentDisplay);
                                
            if (blink)
                {
                for(uint8_t i = 0; i < 8; i++)
                    {
                    blinkPoint(led, i, 1);
                    }
                }
            }
        }
    return result;
    }



void stateDrumSequencerTransition()
    {
    uint8_t result;
    if (entry) 
        {
        }
    result = doTransitionDisplay(0, false);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
            local.drumSequencer.backup = currentDisplay;
            goDownState(STATE_DRUM_SEQUENCER_TRANSITION_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }            
    playDrumSequencer();
    }



///// MENUS



// Various choices in the menu
#define DRUM_SEQUENCER_MENU_SOLO 0
#define DRUM_SEQUENCER_MENU_MARK 1
#define DRUM_SEQUENCER_MENU_LOCAL 2
#define DRUM_SEQUENCER_MENU_TRACK 3
#define DRUM_SEQUENCER_MENU_GROUP 4
#define DRUM_SEQUENCER_MENU_TRANSITION 5
//#define DRUM_SEQUENCER_MENU_SEND_CLOCK 6
#define DRUM_SEQUENCER_MENU_PERFORMANCE 6
#define DRUM_SEQUENCER_MENU_SAVE 7
#define DRUM_SEQUENCER_MENU_CENTER 8
#define DRUM_SEQUENCER_MENU_OPTIONS 9


// Gives other options
void stateDrumSequencerMenu()
    {
    uint8_t result;
    
    const char* menuItems[10] = 
        {    
        local.drumSequencer.performanceMode ? PSTR("PAUSE") : (local.drumSequencer.solo ? PSTR("NO SOLO") : PSTR("SOLO")),
        PSTR("MARK"),
        PSTR("LOCAL"),
        PSTR("TRACK"),
        PSTR("GROUP"),
        PSTR("CHAIN"),
        //options.drumSequencerSendClock ? PSTR("NO CLOCK CONTROL") : PSTR("CLOCK CONTROL"),
        PSTR("PERFORMANCE"),
        PSTR("SAVE"), 
        PSTR("CENTER"),
        options_p 
        };
    result = doMenuDisplay(menuItems, 10, STATE_NONE, STATE_NONE, 1);

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
            goDownState(STATE_DRUM_SEQUENCER_PLAY);
            switch(currentDisplay)
                {
                case DRUM_SEQUENCER_MENU_SOLO:
                    {
                    if (local.drumSequencer.performanceMode)
                        {
                        pauseDrumSequencer();
                        }
                    else
                        {
                        local.drumSequencer.solo = !local.drumSequencer.solo;
                        }
                    }
                break;
                case DRUM_SEQUENCER_MENU_MARK:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_MARK);                            
                    }
                break;
                case DRUM_SEQUENCER_MENU_LOCAL:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_LOCAL);                            
                    }
                break;
                case DRUM_SEQUENCER_MENU_TRACK:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_TRACK);                            
                    }
                break;
                case DRUM_SEQUENCER_MENU_GROUP:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_GROUP);                            
                    }
                break;
                case DRUM_SEQUENCER_MENU_TRANSITION:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_TRANSITION);
                    }
                break;
                /*
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
                */
                case DRUM_SEQUENCER_MENU_PERFORMANCE:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_PERFORMANCE);   
                    }
                break;
                /*
                  case DRUM_SEQUENCER_MENU_EDIT:
                  {
                  goDownState(STATE_DRUM_SEQUENCER_MENU_EDIT);
                  }
                  break;
                */
                case DRUM_SEQUENCER_MENU_SAVE:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_SAVE);
                    }
                break;
                case DRUM_SEQUENCER_MENU_CENTER:
                    {
                    goDownState(STATE_DRUM_SEQUENCER_CENTER);
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
    
void stateDrumSequencerMenuPattern()    
    {
    const char* menuItems[16] =     { PSTR("OOOO"), PSTR("OOO-"), PSTR("---O"), PSTR("OO-O"), PSTR("--O-"), PSTR("O---"), PSTR("-O--"), PSTR("OO--"), PSTR("--OO"), PSTR("O-O-"), PSTR("-O-O"), 
                                      PSTR("H1/8"),                      PSTR("H1/4"),                      PSTR("H1/2"),                      PSTR("---X"),                      PSTR("XXXX") };
    const uint8_t menuIndices[16] = { P1111,        P1110,        P0001,        P1101,        P0010,        P1000,        P0100,        P1100,        P0011,        P1010,        P0101,                    
                                      DRUM_SEQUENCER_PATTERN_DROP_1_8, DRUM_SEQUENCER_PATTERN_DROP_1_4, DRUM_SEQUENCER_PATTERN_DROP_1_2, DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL, DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE };
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
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    playDrumSequencer();
    }


//// NOTE: The IMMEDIATE_RETURN feature removed from the next three functions
//// because I've found it VERY ANNOYING -- typically you need to set multiple
//// items, and so should go back into the menu.

void stateDrumSequencerMenuPerformanceKeyboard()
    {
    uint8_t result = doNumericalDisplay(CHANNEL_ADD_TO_DRUM_SEQUENCER, 17, options.drumSequencerPlayAlongChannel, true, GLYPH_PICK);
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
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            // get rid of any residual select button calls, so we don't stop when exiting here
            isUpdated(SELECT_BUTTON, RELEASED);
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    }
        
void stateDrumSequencerMenuPerformanceRepeat()  
    {
    // This is forever, 1 time, 2, 3, 4, 5, 6, 8, 9, 12, 16, 18, 24, 32, 64, 128 times 
    const char* menuItems[16] = {  PSTR("LOOP"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("8"), PSTR("9"), PSTR("12"), PSTR("16"), PSTR("18"), PSTR("24"), PSTR("32"), PSTR("64"), PSTR("128") };
    if (entry) 
        {
        defaultMenuValue = local.drumSequencer.repeatSequence;
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
            local.drumSequencer.repeatSequence = currentDisplay;
            resetDrumSequencerSequenceCountdown();
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    }

        
void stateDrumSequencerMenuPerformanceNext()
    {
    // The values are END, 0, 1, ..., 8
    // These correspond with stored values (in the high 4 bits of repeat) of 0...9
    uint8_t result = doNumericalDisplay(-1, 8, ((int16_t)(local.drumSequencer.nextSequence - 1)), true, GLYPH_NONE);
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
            local.drumSequencer.nextSequence = currentDisplay + 1;
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    }

void stateDrumSequencerMenuPerformanceStop()
    {
    options.drumSequencerStop = !options.drumSequencerStop;
    saveOptions();
    goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
    playDrumSequencer();
    }

void stateDrumSequencerMenuPerformanceFill()
    {
    options.drumSequencerFill = !options.drumSequencerFill;
    saveOptions();
    goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
    playDrumSequencer();
    }

void stateDrumSequencerMenuPerformanceNextSequence()
    {
    options.drumSequencerNextSequence = !options.drumSequencerNextSequence;
    saveOptions();
    goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
    playDrumSequencer();
    }

////  FIXME:  I note that many of these are similar.  But I can't use my standard
////  stateNumerical trick because most of them require custom loading and saving
////  using the macros rather than pointing to a spot in memory.  :-(  Maybe I can
////  come up with some kind of custom space-saving mechanism still...


void stateDrumSequencerMIDIChannelOut()
    {
    uint8_t result;
    if (entry) 
        {
        local.drumSequencer.backup = getMIDIChannel(local.drumSequencer.currentTrack);
        defaultMenuValue = local.drumSequencer.backup;
        }
    const char* menuItems[25] = { PSTR("----"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("7"), 
                                  PSTR("8"), PSTR("9"), PSTR("10"), PSTR("11"), PSTR("12"), PSTR("13"), PSTR("14"), PSTR("15"), PSTR("16"),
                                  PSTR("DFLT"), PSTR("CHOKE"), PSTR("SELF CHOKE"), PSTR("A +1"), PSTR("A +2"), PSTR("A +3"), PSTR("A -1"), PSTR("A -2") };
    result = doMenuDisplay(menuItems, (local.drumSequencer.currentTrack == local.drumSequencer.numTracks - 1) ? 24 : 20, STATE_NONE, STATE_NONE, 1);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
            setMIDIChannel(local.drumSequencer.currentTrack, currentDisplay);
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            setMIDIChannel(local.drumSequencer.currentTrack, local.drumSequencer.backup);
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    playDrumSequencer();
    }



void stateDrumSequencerVelocity()
    {
    uint8_t result;
    if (entry) 
        {
        local.drumSequencer.backup = getNoteVelocity(local.drumSequencer.currentTrack);
        }
    result = doNumericalDisplay(1, 8, local.drumSequencer.backup + 1, false, GLYPH_NONE, 
        GLYPH_NONE, immediateReturn && (immediateReturnState == STATE_DRUM_SEQUENCER_PLAY));
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            setNoteVelocity(local.drumSequencer.currentTrack, currentDisplay - 1);  // so we can hear it
            }
        break;
        case MENU_SELECTED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            setNoteVelocity(local.drumSequencer.currentTrack, local.drumSequencer.backup);
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    playDrumSequencer();

    /*
      uint8_t result;
      if (entry) 
      {
      local.drumSequencer.backup = getNoteVelocity(local.drumSequencer.currentTrack);
      defaultMenuValue = local.drumSequencer.backup;
      }
      const char* menuItems[8] = { PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("7"), PSTR("8") };
      result = doMenuDisplay(menuItems, 8, STATE_NONE, STATE_NONE, 1);
      switch (result)
      {
      case NO_MENU_SELECTED:
      {
      setNoteVelocity(local.drumSequencer.currentTrack, currentDisplay);  // so we can hear it
      }
      break;
      case MENU_SELECTED:
      {
      goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
      }
      break;
      case MENU_CANCELLED:
      {
      setNoteVelocity(local.drumSequencer.currentTrack, local.drumSequencer.backup);
      goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
      }
      break;
      }
      playDrumSequencer();
    */
    }


void stateDrumSequencerMenuDefaultVelocity()
    {
    uint8_t result;
    if (entry) 
        {
        local.drumSequencer.backup = getNoteVelocity(local.drumSequencer.currentTrack);
        }
    result = doNumericalDisplay(1, 8, local.drumSequencer.backup + 1, false, GLYPH_NONE, GLYPH_NONE, immediateReturnState == STATE_DRUM_SEQUENCER_PLAY);  // notice getGroupLength, not getGroupLengthData
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            setNoteVelocity(local.drumSequencer.currentTrack, currentDisplay - 1);  // so we can hear it
            }
        break;
        case MENU_SELECTED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            setNoteVelocity(local.drumSequencer.currentTrack, local.drumSequencer.backup);
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    playDrumSequencer();

    /*
      uint8_t result;
      if (entry) 
      {
      local.drumSequencer.backup = getNoteVelocity(local.drumSequencer.currentTrack);
      defaultMenuValue = options.drumSequencerDefaultVelocity;
      }
      const char* menuItems[8] = { PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("7"), PSTR("8") };
      result = doMenuDisplay(menuItems, 8, STATE_NONE, STATE_NONE, 1);
      switch (result)
      {
      case NO_MENU_SELECTED:
      {
      setNoteVelocity(local.drumSequencer.currentTrack, currentDisplay);  // so we can hear it
      }
      break;
      case MENU_SELECTED:
      { 
      options.drumSequencerDefaultVelocity = currentDisplay;   
      saveOptions();
      setNoteVelocity(local.drumSequencer.currentTrack, local.drumSequencer.backup);
      goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
      }
      break;
      case MENU_CANCELLED:
      {
      setNoteVelocity(local.drumSequencer.currentTrack, local.drumSequencer.backup);
      goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
      }
      break;
      }
      playDrumSequencer();
    */
    }


// Writes to the screen the current note.  Useful for quickly determining the current note of a track
void stateDrumSequencerPitchBack()
    {
    clearScreen();
    writeNotePitchLong(led2, led, getNotePitch(local.drumSequencer.currentTrack));
    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(immediateReturnState);                
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        goUpState(STATE_DRUM_SEQUENCER_TRACK_PITCH);  // return to it
        }

    // allow play-through
    if (newItem && (itemType == MIDI_NOTE_ON))   //// there is a note played
        {
        uint8_t out = getMIDIChannel(local.drumSequencer.currentTrack);
        if (out == DRUM_SEQUENCER_MIDI_OUT_DEFAULT) 
            out = options.channelOut;
        if (out != DRUM_SEQUENCER_NO_MIDI_OUT && out < DRUM_SEQUENCER_MIDI_ACCENT)
            {
            sendNoteOn(itemNumber, getNoteMIDIVelocity(getNoteVelocity(local.drumSequencer.currentTrack)), out);
            }
        }

    playDrumSequencer();
    }

void stateDrumSequencerPitch()
    {
    uint8_t note = stateEnterNote(STATE_DRUM_SEQUENCER_TRACK_PITCH_BACK);
    if (note != NO_NOTE)
        {
        setNotePitch(local.drumSequencer.currentTrack, note);
        goUpState(immediateReturnState);
        }
    playDrumSequencer();
    }
    
void stateDrumSequencerMenuCenter()
    {
    uint8_t note = stateEnterNote(STATE_DRUM_SEQUENCER_PLAY);
    if (note != NO_NOTE)
        {
        uint8_t newNote = div12(note) * 12;
        if (newNote == note)
            {
            options.drumSequencerControllerCenter = note;
            saveOptions();
            goUpState(STATE_DRUM_SEQUENCER_PLAY);
            }
        else
            {
            goDownState(STATE_DRUM_SEQUENCER_CANT);
            }
        }
    playDrumSequencer();
    }


void stateDrumSequencerGroupLength()
    {
    uint8_t result;
    if (entry) 
        {
        }
    // we're going 1...numNotes
    result = doNumericalDisplay(1, local.drumSequencer.numNotes, getGroupLength(local.drumSequencer.currentGroup), false, GLYPH_NONE);  // notice getGroupLength, not getGroupLengthData
                
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
            // we're 1...numNotes, need to convert to DEFAULT=0, 1...numNotes-1
            if (currentDisplay == local.drumSequencer.numNotes)         // should be "default"
                {
                setGroupLengthData(local.drumSequencer.currentGroup, DRUM_SEQUENCER_GROUP_LENGTH_DEFAULT);              // because we can't set to 64
                }
            else
                {
                setGroupLengthData(local.drumSequencer.currentGroup, currentDisplay);
                }
            drumSequencerChangeGroup(local.drumSequencer.currentGroup);    // makes play and edit positions rational
            }
        // FALL THRU
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }            
    playDrumSequencer();
    }


void stateDrumSequencerGroupSpeed()
    {
    uint8_t result;
    if (entry) 
        {
        defaultMenuValue = getNoteSpeed(local.drumSequencer.currentGroup);
        }
    const char* menuItems[4] = { PSTR("1"), PSTR("2"), PSTR("4"), PSTR("1/2") };
    result = doMenuDisplay(menuItems, 4, STATE_NONE, STATE_NONE, 1);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
            setNoteSpeed(local.drumSequencer.currentGroup, currentDisplay);
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_MENU);
            }
        break;
        }
    playDrumSequencer();
    }
        
        
void insertTransition(uint8_t pos)
    {
    if (local.drumSequencer.performanceMode) resetDrumSequencer();
        
    for(uint8_t i = DRUM_SEQUENCER_NUM_TRANSITIONS - 1; i > pos ; i--)
        {
        local.drumSequencer.transitionGroup[i] = local.drumSequencer.transitionGroup[i - 1];
        local.drumSequencer.transitionRepeat[i] = local.drumSequencer.transitionRepeat[i - 1];
        }
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;            // clear the mark
    }

void copyTransition(uint8_t from, uint8_t to)
    {
    if (local.drumSequencer.performanceMode) resetDrumSequencer();
        
    uint8_t group = local.drumSequencer.transitionGroup[from]; 
    uint8_t repeat = local.drumSequencer.transitionRepeat[from]; 
    insertTransition(to);
    local.drumSequencer.transitionGroup[to] = group; 
    local.drumSequencer.transitionRepeat[to] = repeat; 
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;            // clear the mark
    }

void swapTransition(uint8_t from, uint8_t to)
    {
    if (local.drumSequencer.performanceMode) resetDrumSequencer();
    
    uint8_t temp = local.drumSequencer.transitionGroup[from]; 
    local.drumSequencer.transitionGroup[from] = local.drumSequencer.transitionGroup[to];
    local.drumSequencer.transitionGroup[to] = temp;
    
    temp = local.drumSequencer.transitionRepeat[from];
    local.drumSequencer.transitionRepeat[from] = local.drumSequencer.transitionRepeat[to];
    local.drumSequencer.transitionRepeat[to] = temp;
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;            // clear the mark
    }

void deleteTransition(uint8_t pos)
    {
    if (local.drumSequencer.performanceMode) resetDrumSequencer();
        
    for(uint8_t i = pos; i < DRUM_SEQUENCER_NUM_TRANSITIONS - 1; i++)
        {
        local.drumSequencer.transitionGroup[i] = local.drumSequencer.transitionGroup[i + 1];
        local.drumSequencer.transitionRepeat[i] = local.drumSequencer.transitionRepeat[i + 1];
        }
    local.drumSequencer.transitionGroup[DRUM_SEQUENCER_NUM_TRANSITIONS - 1] = DRUM_SEQUENCER_TRANSITION_GROUP_OTHER;
    local.drumSequencer.transitionRepeat[DRUM_SEQUENCER_NUM_TRANSITIONS - 1] = DRUM_SEQUENCER_TRANSITION_OTHER_END;
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;            // clear the mark
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;            // clear the mark
    }
        
void moveTransition(uint8_t from, uint8_t to)
    {
    if (local.drumSequencer.performanceMode) resetDrumSequencer();
        
    uint8_t group = local.drumSequencer.transitionGroup[from]; 
    uint8_t repeat = local.drumSequencer.transitionRepeat[from]; 
    deleteTransition(from);
    insertTransition(to);
    local.drumSequencer.transitionGroup[to] = group; 
    local.drumSequencer.transitionRepeat[to] = repeat; 
    local.drumSequencer.markTransition = DRUM_SEQUENCER_NO_MARK;            // clear the mark
    }


void stateDrumSequencerTransitionGoGroup()
    {
    uint8_t group = local.drumSequencer.transitionGroup[local.drumSequencer.backup];
    if (group == DRUM_SEQUENCER_TRANSITION_GROUP_OTHER)
        {
        goUpState(STATE_DRUM_SEQUENCER_TRANSITION_CANT);
        }
    else
        {
        drumSequencerChangeGroup(group);
        goUpState(STATE_DRUM_SEQUENCER_PLAY);
        }
    playDrumSequencer();
    }


void stateDrumSequencerTransitionCopy()
    {
    if (local.drumSequencer.markTransition == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_TRANSITION_CANT);
        }
    else
        {
        copyTransition(local.drumSequencer.markTransition, local.drumSequencer.backup);
        goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_TRANSITION);
        }
    playDrumSequencer();
    }


void stateDrumSequencerTransitionSwap()
    {
    if (local.drumSequencer.markTransition == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_TRANSITION_CANT);
        }
    else
        {
        swapTransition(local.drumSequencer.markTransition, local.drumSequencer.backup);
        goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_TRANSITION);
        }
    playDrumSequencer();
    }


void stateDrumSequencerTransitionMove()
    {
    if (local.drumSequencer.markTransition == DRUM_SEQUENCER_NO_MARK)
        {
        goDownState(STATE_DRUM_SEQUENCER_TRANSITION_CANT);
        }
    else
        {
        moveTransition(local.drumSequencer.markTransition, local.drumSequencer.backup);
        goUpState(immediateReturn ? immediateReturnState : STATE_DRUM_SEQUENCER_TRANSITION);
        }
    playDrumSequencer();
    }

void stateDrumSequencerTransitionDelete()
    {
    deleteTransition(local.drumSequencer.backup);
    goUpState(STATE_DRUM_SEQUENCER_TRANSITION);
    playDrumSequencer();
    }


#define DRUM_SEQUENCER_ADD 0
#define DRUM_SEQUENCER_EDIT 1

void stateDrumSequencerTransitionEdit()
    {
    local.drumSequencer.transitionOperationBackup = DRUM_SEQUENCER_EDIT;
    goDownState(STATE_DRUM_SEQUENCER_EDIT_TRANSITION_GROUP);
    playDrumSequencer();
    }


void stateDrumSequencerTransitionAdd()
    {
    local.drumSequencer.transitionOperationBackup = DRUM_SEQUENCER_ADD;
    local.drumSequencer.transitionGroupBackup = local.drumSequencer.currentGroup;           // 0 ... 14
    goDownState(STATE_DRUM_SEQUENCER_EDIT_TRANSITION_GROUP);
    playDrumSequencer();
    }

void stateDrumSequencerTransitionMark()
    {
    local.drumSequencer.markTransition = local.drumSequencer.backup;
    goDownState(STATE_DRUM_SEQUENCER_TRANSITION);
    playDrumSequencer();
    }

/// TRANSITION is stored in local.drumSequencer.transitionBackup    
/// OPERATION is stored in local.drumSequencer.operationBackup    
/// GROUP is stored in local.drumSequencer.transitionGroupBackup

void stateDrumSequencerTransitionEditGroup()
    {
    if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        options.drumSequencerDisplayGroup = !options.drumSequencerDisplayGroup;
        saveOptions();
        }
    
    uint8_t result;
    if (entry) 
        {
        }
    secondGlyph = GLYPH_3x5_G;
    result = doNumericalDisplay(0, local.drumSequencer.numGroups + 1, local.drumSequencer.transitionGroup[local.drumSequencer.backup] + 1, true, GLYPH_DEFAULT);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // only draw custom when we're displaying a group other than default and extra
            if (options.drumSequencerDisplayGroup && currentDisplay > 0 && currentDisplay <= local.drumSequencer.numGroups )
                {
                if (updateDisplay)
                    {
                    uint8_t group = currentDisplay - 1;
                    clearScreen();
                    drawDrumSequencer(group, false);
                    drawRange(led2, 0, 1, MAX_DRUM_SEQUENCER_GROUPS, group);
                    }
                }
#ifdef TWO_SCREENS_VERTICAL
            else if (currentDisplay > 0 && currentDisplay <= local.drumSequencer.numGroups)
                {
                if (updateDisplay)
                    {
                    uint8_t group = currentDisplay - 1;
                    clearMatrix(led3);
                    clearMatrix(led4);
                    drawDrumSequencer(group, false, true);          // draw just the top
                    }
                }
#endif TWO_SCREENS_VERTICAL
            }
        break;
        case MENU_SELECTED:
            {
            if (currentDisplay == 0)
                {
                local.drumSequencer.transitionGroupBackup = DRUM_SEQUENCER_TRANSITION_GROUP_OTHER;
                goDownState(STATE_DRUM_SEQUENCER_EDIT_TRANSITION_SPECIAL);
                }
            else if (currentDisplay == local.drumSequencer.numGroups + 1)
                {
                local.drumSequencer.transitionGroupBackup = local.drumSequencer.currentGroup;
                goDownState(STATE_DRUM_SEQUENCER_EDIT_TRANSITION_REPEAT);
                }
            else
                {
                local.drumSequencer.transitionGroupBackup = currentDisplay - 1;
                goDownState(STATE_DRUM_SEQUENCER_EDIT_TRANSITION_REPEAT);
                }
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_DRUM_SEQUENCER_TRANSITION);
            }
        break;
        }            
    playDrumSequencer();
    }

void stateDrumSequencerTransitionEditRepeat()
    {
    uint8_t result;
    if (entry) 
        {
        defaultMenuValue = local.drumSequencer.transitionRepeat[local.drumSequencer.backup];
        }
    secondGlyph = GLYPH_3x5_R;
    const char* menuItems[16] = { PSTR("LOOP"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("7"), PSTR("8"), PSTR("9"), PSTR("12"), PSTR("16"), PSTR("24"), PSTR("32"), PSTR("64"), PSTR("BIG LOOP") };
    result = doMenuDisplay(menuItems, 16, STATE_NONE, STATE_NONE, 1);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
            if (local.drumSequencer.transitionOperationBackup == DRUM_SEQUENCER_ADD) 
                {
                insertTransition(local.drumSequencer.backup);
                }
            local.drumSequencer.transitionGroup[local.drumSequencer.backup] = local.drumSequencer.transitionGroupBackup;
            local.drumSequencer.transitionRepeat[local.drumSequencer.backup] = currentDisplay;   
            goUpState(STATE_DRUM_SEQUENCER_TRANSITION);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_DRUM_SEQUENCER_TRANSITION);
            }
        break;
        }
    playDrumSequencer();
    }


void stateDrumSequencerTransitionEditSpecial()
    {
    uint8_t result;
    if (entry) 
        {
        defaultMenuValue = local.drumSequencer.transitionRepeat[local.drumSequencer.backup];
        if (defaultMenuValue < 3)
            defaultMenuValue = 0;
        }
                
    // END:     End marker: this is not a group, it marks the termination of the sequence.  Not permitted in transition slot 0.
    // R1:      Random(choose beetween groups 0 and 1)
    // R2:      Random(choose beetween groups 0 and 2)
    // R3:      Random(choose beetween groups 0 and 3)
    // -1:      one time
    // -2:      two times
    // -3:      three times
    // -4:      four times
    // -L:      loop forever
            
    if (local.drumSequencer.backup == 0)  // slot 0 can't have "END"
        {
        const char* menuItems[15] = { PSTR("R2-1"), PSTR("R2-2"), PSTR("R2-3"), PSTR("R2-4"), PSTR("R2-L"), PSTR("R3-1"), PSTR("R3-2"), PSTR("R3-3"), PSTR("R3-4"), PSTR("R3-L"), PSTR("R4-1"), PSTR("R4-2"), PSTR("R4-3"), PSTR("R4-4"), PSTR("R4-L") };
        result = doMenuDisplay(menuItems, 15, STATE_NONE, STATE_NONE, 1);
        }
    else
        {
        const char* menuItems[16] = { PSTR("END"), PSTR("R2-1"), PSTR("R2-2"), PSTR("R2-3"), PSTR("R2-4"), PSTR("R2-L"), PSTR("R3-1"), PSTR("R3-2"), PSTR("R3-3"), PSTR("R3-4"), PSTR("R3-L"), PSTR("R4-1"), PSTR("R4-2"), PSTR("R4-3"), PSTR("R4-4"), PSTR("R4-L") };
        result = doMenuDisplay(menuItems, 16, STATE_NONE, STATE_NONE, 1);
        }
                        
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            }
        break;
        case MENU_SELECTED:
            {
            if (local.drumSequencer.transitionOperationBackup == DRUM_SEQUENCER_ADD)
                {
                insertTransition(local.drumSequencer.backup);
                }
            local.drumSequencer.transitionGroup[local.drumSequencer.backup] = local.drumSequencer.transitionGroupBackup;
            if (local.drumSequencer.backup == 0)                // skip "END"
                {
                local.drumSequencer.transitionRepeat[local.drumSequencer.backup] = currentDisplay + 1;                    
                }
            else
                {
                local.drumSequencer.transitionRepeat[local.drumSequencer.backup] = currentDisplay;                    
                }
            goUpState(STATE_DRUM_SEQUENCER_TRANSITION);
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_DRUM_SEQUENCER_TRANSITION);
            }
        break;
        }
    playDrumSequencer();
    }



////// DRAWING


    
// If the point holds a cursor, blinks it, else sets it.  Used to simplify
// and reduce code size
void drumSequencerBlinkOrSetPoint(unsigned char* led, uint8_t x, uint8_t y, uint8_t isCursor)
    {
    if (isCursor)
        blinkPoint(led, x, y);
    else
        setPoint(led, x, y);
    }
  
/// Returns whether a given track ought to be muted right now (doesn't consider patterns)
uint8_t drumSequencerShouldMuteTrack(uint8_t track)
    {
    uint8_t solo = (local.drumSequencer.solo == DRUM_SEQUENCER_SOLO || local.drumSequencer.solo == DRUM_SEQUENCER_SOLO_OFF_SCHEDULED);
    uint8_t muted = (local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTED || local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_OFF_SCHEDULED || local.drumSequencer.muted[track] == DRUM_SEQUENCER_MUTE_OFF_SCHEDULED_ONCE);
    return 
        // We're muted if solo is on and we're not the current track
        (solo && track != local.drumSequencer.currentTrack) ||
        // We're muted if solo is NOT on and OUR mute is on
        (!solo && muted);
    }


void drawDrumSequencerNotePitchAndVelocity(uint8_t trackLen)
    {
#ifdef TWO_SCREENS_VERTICAL
    if (local.stepSequencer.currentEditPosition < 0) return;
    int8_t pos = local.stepSequencer.currentEditPosition;
    if (pos < 0)
        pos = local.stepSequencer.currentPlayPosition;
    uint8_t note = getNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, pos);
    if (note)
        {
        writeNotePitch(led3, getNotePitch(local.drumSequencer.currentTrack));
        writeShortNumber(led4, getNoteVelocity(local.drumSequencer.currentTrack), false);
        }
    else
        {
        // do nothing
        }
#else
    // do nothing
#endif TWO_SCREENS_VERTICAL
    }


// Draws the sequence with the given track length, number of tracks, and skip size
void drawDrumSequencer(uint8_t playGroup, uint8_t drawFooters, uint8_t topScreenOnly=false)
    {
    // compute TRACKLEN, the length of the track
    // compute SKIP, the number of lines on the screen the track takes up
    uint8_t trackLen = getGroupLength(playGroup);
    uint8_t numTracks = local.drumSequencer.numTracks;

    // this little function correctly maps:
    // 8 -> 1
    // 12 -> 1
    // 16 -> 1
    // 24 -> 2
    // 32 -> 2
    // 48 -> 3
    // 64 -> 4    
    // 96 -> 6    
    uint8_t skip = ((trackLen + 15) >> 4);      // that is, trackLen / 16

#ifdef TWO_SCREENS_VERTICAL
    if (topScreenOnly)
        {
        clearMatrix(led3);
        clearMatrix(led4);
        }
    else
#endif TWO_SCREENS_VERTICAL
        clearScreen();
    
    // revise LASTTRACK to be just beyond the last track we'll draw
    //      (where TRACK is the first track we'll draw)     
        
    // this code is designed to allow the user to move down to about the middle of the screen,
    // at which point the cursor stays there and the screen scrolls instead.
    uint8_t firstTrack = local.drumSequencer.currentTrack;
    uint8_t lastTrack = numTracks;          // lastTrack is 1+ the final track we'll be drawing

#ifdef TWO_SCREENS_VERTICAL
    uint8_t fourteenskip = 14 / skip;
    if (numTracks <= fourteenskip)              // will all fit on one screen
        firstTrack = 0;
    else
        {
        if (!drawFooters) firstTrack = 0;
        uint8_t eightskip =  8 / skip;
        if (firstTrack <= eightskip)  
            firstTrack = 0;
        else firstTrack = firstTrack - eightskip;       //  + 1;   
        lastTrack = bound(lastTrack, 0, firstTrack + fourteenskip);
        if (lastTrack == numTracks)
            {
            if (lastTrack >= fourteenskip) 
                firstTrack = lastTrack - fourteenskip;
            }
        }
#else
    if (!drawFooters) firstTrack = 0;
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
#endif TWO_SCREENS_VERTICAL



    // Now we start drawing each of the tracks.  We will make blinky lights for beats or for the cursor
    // and will have solid lights or nothing for the notes or their absence.
        
//    uint8_t playGroup = (local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_ON ?
//            local.drumSequencer.fillGroup : local.drumSequencer.currentGroup);

#ifdef TWO_SCREENS_VERTICAL
    uint8_t y = 15;
#else
    uint8_t y = 7;              // we can go negative if we have two vertical screens
#endif TWO_SCREENS_VERTICAL
    for(uint8_t t = firstTrack; t < lastTrack; t++)  // for each track from top to bottom
        {
        // for each note in the track
        for (uint8_t d = 0; d < trackLen; d++)
            {
            uint8_t shouldDrawMuted = drumSequencerShouldMuteTrack(t);
            uint16_t pos = (t * (uint16_t) trackLen + d) * 2;
            uint8_t vel = getNote(playGroup, t, d);
                
            if (shouldDrawMuted)
                vel = 0;
            uint8_t xpos = d - ((d >> 4) * 16);  // x position on screen
            uint8_t blink = (drawFooters && 
                    (
                    // draw play position cursor if we're not stopped and we're in edit cursor mode
                        ((local.drumSequencer.playState != PLAY_STATE_STOPPED) && (d == local.drumSequencer.currentPlayPosition)
                        && !local.drumSequencer.performanceMode) ||   // main cursor
                    // draw play position cursor, plus the crosshatch, always if we're in play position mode
                        ((local.drumSequencer.currentEditPosition < 0 || local.drumSequencer.performanceMode) && 
                        ((d == local.drumSequencer.currentPlayPosition) ||  ((t == local.drumSequencer.currentTrack) && (abs(d - local.drumSequencer.currentPlayPosition) == 2)))) ||  // crosshatch
                        ((local.drumSequencer.currentEditPosition >= trackLen && !(local.drumSequencer.performanceMode)) && 
                        ((d == local.drumSequencer.currentPlayPosition) ||  ((t == local.drumSequencer.currentTrack) && (abs(d - local.drumSequencer.currentPlayPosition) == 1)))) ||  // smaller crosshatch    
                    // draw edit cursor
                        ((t == local.drumSequencer.currentTrack) && (d == local.drumSequencer.currentEditPosition) 
                        && !local.drumSequencer.performanceMode) ||
                
                    // draw mute or solo indicator.  Solo overrides mute.
                    // So draw if solo is on but we're not it, OR if solo is turned off and we're muted
                    ((xpos == 0 || xpos == 15) && shouldDrawMuted)));

            if (vel || blink)
                {       
                // <8 <16 <24 <32 <40 <48 <56 <64
                if (d < 32)                             // for reasons only known to ATMEL, if I don't have this line on the Uno it's much bigger.  But it's irrelevant!
                    {
                    if (d < 16)
                        {
                        if (d < 8)
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led4, d, y - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led2, d, y, blink);
                            }
                        else // < 16
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led3, d-8, y - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led, d-8, y, blink);
                            }
                        }
                    else
                        {
                        if (d < 24)
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led4, d-16, y-1 - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led2, d-16, y-1, blink);
                            }
                        else  // < 32
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led3, d-24, y-1 - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led, d-24, y-1, blink);
                            }
                        }
                    }
                else
                    {
                    if (d < 48)
                        {
                        if (d < 40)
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led4, d-32, y-2 - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led2, d-32, y-2, blink);
                            }
                        else // < 48
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led3, d-40, y-2 - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led, d-40, y-2, blink);
                            }
                        }
                    else
                        {
                        if (d < 56)
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led4, d-48, y-3 - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led2, d-48, y-3, blink);
                            }
                        else  // < 64
                            {
#ifdef TWO_SCREENS_VERTICAL
                            if (y > 7)
                                drumSequencerBlinkOrSetPoint(led3, d-56, y-3 - 8, blink);
                            else
#endif TWO_SCREENS_VERTICAL
                                if (!topScreenOnly) drumSequencerBlinkOrSetPoint(led, d-56, y-3, blink);
                            }
                        }
                    }
                }
            }
        y -= skip;

#ifdef TWO_SCREENS_VERTICAL
        if (topScreenOnly) 
            {
            if (y < 9) break;
            }
#endif TWO_SCREENS_VERTICAL
        }
    
    if (drawFooters)
        {
        // Draw the Track Number
        drawRange(led2, 0, 1, MAX_DRUM_SEQUENCER_TRACKS, local.drumSequencer.currentTrack);

        // Next the Group Number or Transition Number
        if (local.drumSequencer.performanceMode)
            {
            uint8_t trans = local.drumSequencer.currentTransition;
            if (trans == DRUM_SEQUENCER_TRANSITION_START) 
                trans = 0;
            drawRange(led2, 0, 0, DRUM_SEQUENCER_NUM_TRANSITIONS, trans);
            }
        else
            {
            drawRange(led2, 0, 0, MAX_DRUM_SEQUENCER_GROUPS, local.drumSequencer.currentGroup);
            }

        // draw pattern position
        drawRange(led, 0, 1, 4, local.drumSequencer.patternCountup & 3);

        // Are we in performance mode?
        if (local.drumSequencer.performanceMode)
            {
            blinkPoint(led, 2, 1);
            // are we going to the next transition?  (Or sequence?)
            if (local.drumSequencer.goNextSequence || local.drumSequencer.scheduleStop)
                blinkPoint(led, 3, 1);
            else if (local.drumSequencer.goNextTransition)                  // including specific transitions
                setPoint(led, 3, 1);
            }       
                
        // is our track scheduled to play?
        if (local.drumSequencer.shouldPlay[local.drumSequencer.currentTrack])
            setPoint(led, 4, 1);
                                
        // draw drum region
        if (local.drumSequencer.currentEditPosition < 0 || local.drumSequencer.currentEditPosition >= trackLen)
            {
            int8_t val = local.drumSequencer.drumRegion;
            // val should be -1, -2, or -3.  If it is not, we make it -1
            if (val >= 0 || val < -3) val = -1;
            val = (- val) - 1;              // convert to 0, 1, 2
            drawRange(led, 5, 1, 4, (uint8_t) val);
            }
        else
            {
            int8_t val = local.drumSequencer.drumRegion;
            // val should be 0, 1, 2, or 3.  If it is not, we make it 0
            if (val < 0 || val > 3) val = 0;
            drawRange(led, 5, 1, 4, (uint8_t) val);
            }

        // Are we stopped?
        if (local.drumSequencer.playState != PLAY_STATE_PLAYING)
            {
            setPoint(led, 7, 1);
            }
        else if (local.drumSequencer.invalidNoteSpeed)
            {
            blinkPoint(led, 7, 1);
            }
        }
    }







/////// PLAYING NOTES

// Chokes the given track on the given channel.  The channel is provided because
// it may differ from the track's channel is we're doing SELF CHOKE
void choke(uint8_t track, uint8_t out)
	{
	if (out == 0 || out >= DRUM_SEQUENCER_MIDI_CHOKE) return;	// "off"
	if (out == DRUM_SEQUENCER_MIDI_OUT_DEFAULT)         // 17 
		{
		out = options.channelOut;
		}

	for(uint8_t i = 0; i < local.drumSequencer.chokeCount[track]; i++)
		{
        sendNoteOn(getNotePitch(track), 0, out);		// note on(0) = note off
		}
	local.drumSequencer.chokeCount[track] = 0;
	}
	
// Sends a Note ON to the appropriate MIDI channel at the appropriate pitch and velocity.
// May also send a Note OFF to a choked channel
void sendTrackNote(uint8_t track, uint8_t accent)
    {
    uint8_t out = getMIDIChannel(track);  
    if (out >= DRUM_SEQUENCER_MIDI_ACCENT) return; // uh... 

	if (out == DRUM_SEQUENCER_MIDI_OUT_DEFAULT)         // 17 
		{
		out = options.channelOut;
		}
        
   // Handle chokes
    if (out == DRUM_SEQUENCER_MIDI_CHOKE || out == DRUM_SEQUENCER_MIDI_SELF_CHOKE) 
    	{
    	if (track == 0)	return;		// we assume we're off
    	// find the correct channel
    	for(uint8_t i = track - 1; i >= 0; i--)
    		{
    		if (getMIDIChannel(i) < DRUM_SEQUENCER_MIDI_CHOKE) 	// got it
    			{
    			if (out == DRUM_SEQUENCER_MIDI_CHOKE)
    				choke(i, getMIDIChannel(i));		// choke the other channel
    			if (out == DRUM_SEQUENCER_MIDI_SELF_CHOKE)
    				choke(track, getMIDIChannel(i));	// choke me
    			break;
    			}
    		}
    	}
    	
    uint8_t vel = getNoteVelocity(track);
    
    // change velocity if there is an accent
    if (accent >= DRUM_SEQUENCER_MIDI_ACCENT)
        {
        // These deltas correspond to 1.25, 1.50, 2.0, 2.50, and 3.0
        // const uint8_t accentDeltas[5] = { 1, 2, 3, 4, 5 };
        if (accent < DRUM_SEQUENCER_MIDI_GHOST)
        	{
        	vel =  vel + (accent - DRUM_SEQUENCER_MIDI_ACCENT) + 1;
	        if (vel > 7) vel = 7;
	    	}
	    else
	    	{
	    	vel += 2;	// it's uint, so we can't drop below zero
        	vel = vel - (accent - DRUM_SEQUENCER_MIDI_GHOST) - 1;
        	if (vel < 2) vel = 2;
        	vel -= 2;
	    	}
        }
            
    uint8_t velocity = getNoteMIDIVelocity((uint8_t)vel);
    uint8_t note = getNotePitch(track);
    if (out != DRUM_SEQUENCER_NO_MIDI_OUT)
        {
        sendNoteOn(note, velocity, out);
        if (local.drumSequencer.chokeCount[track] < DRUM_SEQUENCER_MAX_TRACK_LENGTH)	// something rational...
        	local.drumSequencer.chokeCount[track]++;
        }
    }


// Sends a Note ON to the appropriate MIDI channel at the appropriate pitch and velocity
// with no accent
void sendTrackNote(uint8_t track)
    {
    sendTrackNote(track, false);
    }

// Sends a Note ON to the appropriate MIDI channel at the appropriate pitch and velocity
// for the current playing note.  If there is an accent track, then the accent is added.
// If the track equals the accent track then nothing is played.
void sendCurrentTrackNote(uint8_t track)
    {
    uint8_t accent = false;
  
    uint8_t accentTrack = local.drumSequencer.numTracks - 1;
    uint8_t out = getMIDIChannel(accentTrack);  
    if (out >= DRUM_SEQUENCER_MIDI_ACCENT)
        {
        if (track == accentTrack) return;
        if (getNote(local.drumSequencer.currentGroup, accentTrack, local.drumSequencer.currentPlayPosition))
            {
            accent = out;           // this will be >= DRUM_SEQUENCER_MIDI_ACCENT
            }
        }
    sendTrackNote(track, accent);
    }





// Plays the current sequence
void playDrumSequencer()
    {
    // we redo this rather than take it from stateDrumSequencerPlay because we may be 
    // called from other methods as well 
        
    uint8_t trackLen = getGroupLength(local.drumSequencer.currentGroup);
    uint8_t numTracks = local.drumSequencer.numTracks;
        
    if ((local.drumSequencer.playState == PLAY_STATE_WAITING) && beat)
        local.drumSequencer.playState = PLAY_STATE_PLAYING;
        
    if (notePulse && (local.drumSequencer.playState == PLAY_STATE_PLAYING))
        {
        uint8_t oldPlayPosition = local.drumSequencer.currentPlayPosition;
        local.drumSequencer.currentPlayPosition = incrementAndWrap(local.drumSequencer.currentPlayPosition, trackLen);
        
        // update scheduled fill?
        if ((local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_SCHEDULED) &&
            (local.drumSequencer.currentPlayPosition == 0))
            {
            local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_ON;
            }
        else if ((local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_ON) &&
            (local.drumSequencer.currentPlayPosition == 0))        // all done
            {
            local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_OFF;
            }
                
        if (local.drumSequencer.currentPlayPosition == 0)
            {
            if (local.drumSequencer.performanceMode)
                goNextTransition();
                
            uint8_t noteSpeed = getNoteSpeed(local.drumSequencer.currentGroup);
            uint8_t expectedRate = getNotePulseRateFor(options.noteSpeedType);
            if (noteSpeed == 1)     // x2
                {
                local.drumSequencer.invalidNoteSpeed = (((expectedRate >> 1) * 2) != expectedRate);  // uh oh
                expectedRate = expectedRate >> 1;       // divide by two
                if (expectedRate == 0) expectedRate = 1;
                }
            else if (noteSpeed == 2)        // x2
                {
                local.drumSequencer.invalidNoteSpeed = (((expectedRate >> 2) * 4) != expectedRate);  // uh oh
                expectedRate = expectedRate >> 2;       // divide by four
                if (expectedRate == 0) expectedRate = 1;
                }
            else if (noteSpeed == 3)        // /2
                {
                local.drumSequencer.invalidNoteSpeed = (expectedRate >= 144);   // uh oh
                if (expectedRate < 144)
                    expectedRate = expectedRate << 1;       // multiply by two
                }
                        
            //// FIXME: this will divide some rates by two that don't exist.
            //// For example, 36 can't be divided by 4 (there is no 9)
            //// and 8 can't be divided by 2 (there is no 9) and
            //// 6 can't be divided by 4 (there is no 1.5) and
            //// 3 can't be divided by 2 or 4 and
            //// 2 can't be divided by 2 and
            //// 1 can't be divided by 2 or 4 and
            //// 144 and 192 can't be multiplied against 2
            //// We'll make sure it doesn't go to 0 but beyond that we have
            //// to figure out what the appropriate behavior should be
                        
            if (notePulseRate != expectedRate)      // gotta change it
                {
                setRawNotePulseRate(expectedRate, false);
                }
            }
        
        if (local.drumSequencer.performanceMode && local.drumSequencer.scheduleStop && local.drumSequencer.currentPlayPosition == options.drumSequencerStop) 
            {
            stopDrumSequencer(); 
            local.drumSequencer.goNextSequence = false;     // totally reset
            local.drumSequencer.goNextTransition = false;   // totally reset
            return; 
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
        if (local.drumSequencer.currentPlayPosition == 0)
            {
            // determine options
            uint8_t exclusiveTracks[MAX_STEP_SEQUENCER_TRACKS];
            uint8_t oneExclusive = NO_TRACK;
            uint8_t numExclusiveTracks = 0;
            for(uint8_t track = 0; track < numTracks; track++)
                {
                if (getPattern(local.drumSequencer.currentGroup, track) == DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE ||
                    ((getPattern(local.drumSequencer.currentGroup, track) == DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL) && ((local.drumSequencer.patternCountup & 3) == 3)))
                    {
                    oneExclusive = track;           // found something, might be the last one
                    if (track != local.drumSequencer.lastExclusiveTrack && !isAccentTrack(track))
                        {
                        exclusiveTracks[numExclusiveTracks++] = track;          // found a track that's not the last one
                        }
                    }
                }
            
            // pick a track
            if (numExclusiveTracks == 0)                // this happens if there are NO X tracks or ONE X track which has already been used
                {
                if (oneExclusive != NO_TRACK)   // if NO_TRACK, we don't want to reset so we can keep the lastExclusiveTrack to the fourth measure for ---X
                    local.drumSequencer.lastExclusiveTrack = oneExclusive;
                }
            else
                {
                local.drumSequencer.lastExclusiveTrack = exclusiveTracks[random(numExclusiveTracks)];
                }
            }

        // do we schedule a fill?
        uint8_t playGroup = (local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_ON ?
            local.drumSequencer.fillGroup : local.drumSequencer.currentGroup);
                                                
        for(uint8_t track = 0; track < numTracks; track++)
            {
            uint8_t pattern = getPattern(local.drumSequencer.currentGroup, track);
            if (local.drumSequencer.currentPlayPosition == 0)
                {
                // pick a random track                          
                if (pattern == DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE)
                    {
                    local.drumSequencer.shouldPlay[track] = (track == local.drumSequencer.lastExclusiveTrack);
                    }
                else if (pattern == DRUM_SEQUENCER_PATTERN_RANDOM_EXCLUSIVE_FILL)
                    {
                    local.drumSequencer.shouldPlay[track] = (track == local.drumSequencer.lastExclusiveTrack && ((local.drumSequencer.patternCountup & 3) == 3));
                    }
                /*
                  else if (pattern == DRUM_SEQUENCER_PATTERN_RANDOM_3_4)
                  {
                  local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 4) * 3);
                  }
                */
                /*
                  else if (pattern == DRUM_SEQUENCER_PATTERN_RANDOM_1_2)
                  {
                  local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 2));
                  }
                  else if (pattern == DRUM_SEQUENCER_PATTERN_RANDOM_1_4)
                  {
                  local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 4));
                  }
                  else if (pattern == DRUM_SEQUENCER_PATTERN_RANDOM_1_8)
                  {
                  local.drumSequencer.shouldPlay[track] = (random() < (RANDOM_MAX / 8));
                  }
                */
                else
                    {
                    local.drumSequencer.shouldPlay[track] = ((pattern >> (local.drumSequencer.patternCountup & 3)) & 1);                        
                    }
                }
                                                                    
            if (getNote(playGroup, track, local.drumSequencer.currentPlayPosition) && 
                local.drumSequencer.shouldPlay[track] && !drumSequencerShouldMuteTrack(track))
                {
                uint8_t play = 1;
                if (pattern == DRUM_SEQUENCER_PATTERN_DROP_1_2)
                    {
                    play = (random() < (RANDOM_MAX / 2));
                    }
                else if (pattern == DRUM_SEQUENCER_PATTERN_DROP_1_4)
                    {
                    play = (random() < (RANDOM_MAX / 4));
                    }
                else if (pattern == DRUM_SEQUENCER_PATTERN_DROP_1_8)
                    {
                    play = (random() < (RANDOM_MAX / 8));
                    }
                
                if (play)
                    {
                    sendCurrentTrackNote(track);  
                    }       
                }
            }
        }

    // click track
    doClick();
    }




int8_t drumSequencerGetKey(uint8_t octave, uint8_t note)
    {
    uint16_t val = DIV12_REMAINDER(octave, note);
    const int8_t keys[12] = { 0, -1, 1, -2, 2, 3, -3, 4, -4, 5, -6, 6 };  // 7 , -1, 8, -2, 9, 10, -3, 11, -4, 12, -6, 13, 14, -1, 15, -2, 16, 17, -3, 18, -4, 19, -5, 20, 21};
    return keys[val];
    }





///// TOP-LEVEL FACILITY




/// Stops the drum sequencer and resets it to its start position.
void stopDrumSequencer()
    {
    if (local.drumSequencer.performanceMode)
        {
        local.drumSequencer.currentGroup = 0;
        // for performance mode
        local.drumSequencer.currentTransition = DRUM_SEQUENCER_TRANSITION_START;                        // gotta make sure this is drawn right
        local.drumSequencer.goNextTransition = true;                       // should be enough to trigger going to the next transition?
        local.drumSequencer.goNextSequence = false;
        }
        
    local.drumSequencer.scheduleFill = DRUM_SEQUENCER_FILL_OFF;
    local.drumSequencer.currentPlayPosition = getGroupLength(local.drumSequencer.currentGroup) - 1;
    resetDrumSequencerSequenceCountdown();          // this will call resetDrumSequencerTransitionCountdown();
    local.drumSequencer.playState = PLAY_STATE_STOPPED;
    sendAllSoundsOff();
    local.drumSequencer.scheduleStop = false;
    local.drumSequencer.lastExclusiveTrack = NO_TRACK;
    stopClock(true);
    }

void goNextGroup()
    {
    uint8_t g = local.drumSequencer.currentGroup + 1;
    if (g >= local.drumSequencer.numGroups) g = 0;
    drumSequencerChangeGroup(g);
    }

void goPreviousGroup()
    {
    uint8_t g = local.drumSequencer.currentGroup;
    if (g == 0 || g >= local.drumSequencer.numGroups) 
        g = local.drumSequencer.numGroups - 1;
    else g--;
    drumSequencerChangeGroup(g);
    }


// Top-level editor function for the sequencer
void stateDrumSequencerPlay()
    {
    if (entry)
        {
        entry = false;
        local.drumSequencer.currentRightPot = DRUM_SEQUENCER_CURRENT_RIGHT_POT_UNDEFINED;
        local.stepSequencer.pots[LEFT_POT] = pot[LEFT_POT];
        local.stepSequencer.pots[RIGHT_POT] = pot[RIGHT_POT];
        }
        
    immediateReturn = false;
    // always do this
    leftPotParameterEquivalent = false;

    // first we:
    // compute TRACKLEN, the length of the track
    // compute SKIP, the number of lines on the screen the track takes up
    uint8_t trackLen = getGroupLength(local.drumSequencer.currentGroup);
    uint8_t numTracks = local.drumSequencer.numTracks;
    
    // this little function correctly maps:
    // 8 -> 1
    // 12 -> 1
    // 16 -> 1
    // 24 -> 2
    // 32 -> 2
    // 48 -> 3
    // 64 -> 4    
    // 96 -> 6    
    uint8_t skip = ((trackLen + 15) >> 4);      // that is, trackLen / 16

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        setParseRawCC(false);
        if (local.drumSequencer.performanceMode)
            {
            //// EXIT PERFORMANCE MODE
            local.drumSequencer.performanceMode = false;
            // reset note pulse rate!
            //setNotePulseRate(options.noteSpeedType);
            }
        else
            {
            //// EXIT SEQUENCER
            goUpState(STATE_DRUM_SEQUENCER_EXIT);
            }
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        if (button[SELECT_BUTTON])
            {
            isUpdated(SELECT_BUTTON, PRESSED);  // kill the long release on the select button

            if (local.drumSequencer.performanceMode)
                {
                //// SCHEDULE TRANSITION
                if (options.drumSequencerNextSequence)
                    {
                    local.drumSequencer.goNextSequence = !local.drumSequencer.goNextSequence;
                    }
                else
                    {
                    local.drumSequencer.goNextTransition = !local.drumSequencer.goNextTransition;           // this should convert > TRUE ("specific" transitions) to FALSE as well
                    }
                }
            else
                {
                //// ENTER PERFORMANCE MODE
                local.drumSequencer.performanceMode = true;
                local.drumSequencer.currentTransition = DRUM_SEQUENCER_TRANSITION_START;                        // gotta make sure this is drawn right
                local.drumSequencer.goNextTransition = true;                       // should be enough to trigger going to the next transition?
                local.drumSequencer.goNextSequence = false;
                resetDrumSequencerTransitionCountdown();  // otherwise we'll miss jumps to other sequences
                setParseRawCC(true);
                }
            }
        else 
            {
            if (local.drumSequencer.performanceMode)
                {
                //// SCHEDULE SOLO
                drumSequencerAdvanceSolo();
                }
            else if (local.drumSequencer.currentEditPosition < 0)
                {
                //// CLEAR TRACK ON GROUP
                clearCurrentTrackInGroup();
                }
            else if (local.drumSequencer.currentEditPosition >= trackLen)
                {
                //// CLEAR GROUP
                goDownState(STATE_DRUM_SEQUENCER_GROUP_CLEAR_SURE);
                }
            else
                {
                //// CHANGE TRACK NOTE
                immediateReturnState = STATE_DRUM_SEQUENCER_PLAY;
                goDownState(STATE_DRUM_SEQUENCER_TRACK_PITCH);
                }
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        if (button[MIDDLE_BUTTON])
            {
            isUpdated(MIDDLE_BUTTON, PRESSED);  // kill the long release on the middle button
            if (local.drumSequencer.performanceMode)
                {
                //// SCHEDULE TRANSITION
                if (options.drumSequencerNextSequence)
                    {
                    local.drumSequencer.goNextSequence = !local.drumSequencer.goNextSequence;
                    }
                else
                    {
                    local.drumSequencer.goNextTransition = !local.drumSequencer.goNextTransition;           // this should convert > TRUE ("specific" transitions) to FALSE as well
                    }
                }
            else
                {
                //// ENTER PERFORMANCE MODE
                local.drumSequencer.performanceMode = true;
                local.drumSequencer.currentTransition = DRUM_SEQUENCER_TRANSITION_START;                        // gotta make sure this is drawn right
                local.drumSequencer.goNextTransition = true;                       // should be enough to trigger going to the next transition?
                local.drumSequencer.goNextSequence = false;
                resetDrumSequencerTransitionCountdown();  // otherwise we'll miss jumps to other sequences
                setParseRawCC(true);
                }
            }
        else
            {
            //// ENTER MENU
            goDownState(STATE_DRUM_SEQUENCER_MENU);
            }
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        if (local.drumSequencer.performanceMode)
            {
            //// SCHEDULE FILL OR MUTE
            drumSequencerAdvanceFillOrMute(local.drumSequencer.currentTrack, DRUM_SEQUENCER_ACTION_EITHER);
            }
        else if (local.drumSequencer.currentEditPosition < 0)
            {
            //// TOGGLE MUTE
            local.drumSequencer.muted[local.drumSequencer.currentTrack] = !local.drumSequencer.muted[local.drumSequencer.currentTrack];
            }
        else if (local.drumSequencer.currentEditPosition >= trackLen)
            {
            //// INCREMENT GROUP
            goNextGroup();
            }
        else  // Edit
            {
            //// TOGGLE NOTE
            toggleNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, local.drumSequencer.currentEditPosition);
            local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, trackLen);
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
        if (local.drumSequencer.performanceMode && local.drumSequencer.playState == PLAY_STATE_PLAYING && !local.drumSequencer.scheduleStop)
            {
            local.drumSequencer.scheduleStop = true;
            }
        else
            {       
            //// START/STOP
            switch(local.drumSequencer.playState)
                {
                case PLAY_STATE_STOPPED:
                    {
                    local.drumSequencer.playState = PLAY_STATE_WAITING;

                    if (local.drumSequencer.performanceMode)
                        {
                        local.drumSequencer.currentGroup = 0;
                        // for performance mode
                        local.drumSequencer.currentTransition = DRUM_SEQUENCER_TRANSITION_START;                        // gotta make sure this is drawn right
                        local.drumSequencer.goNextTransition = true;                       // should be enough to trigger going to the next transition?
                        local.drumSequencer.goNextSequence = false;
                        }
                                                                                
                    // Though this is done in stopDrumSequencer we have to do it again because we may be in a different group now. 
                    local.drumSequencer.currentPlayPosition = getGroupLength(local.drumSequencer.currentGroup) - 1;
                    resetDrumSequencerSequenceCountdown();          // this will call resetDrumSequencerTransitionCountdown();

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
                    stopDrumSequencer();
                    }
                break;
                case PLAY_STATE_PAUSED:
                    {
                    local.drumSequencer.playState = PLAY_STATE_PLAYING;
                    // we always stop the clock just in case, even if we're immediately restarting it
                    stopClock(true);
                    continueClock(true);
                    }
                break;
                }
            }
        }
    else if (potUpdated[LEFT_POT])
        {
        if (!local.drumSequencer.performanceMode && local.drumSequencer.currentEditPosition >= trackLen)
            {
            /*
          //// CHANGE GROUP
          #define BIG_POT_UPDATE (32)
          if (potChangedBy(local.drumSequencer.pots, LEFT_POT, BIG_POT_UPDATE))
            */
            //// CHANGE GROUP
            uint8_t group = ((pot[LEFT_POT] * local.drumSequencer.numGroups) >> 10);         //  / 1024;
            group = bound(group, 0, local.drumSequencer.numGroups);
            drumSequencerChangeGroup(group);
            }
        else
            {
            //// CHANGE TRACK
            local.drumSequencer.currentTrack = ((pot[LEFT_POT] * numTracks) >> 10);         //  / 1024;
            local.drumSequencer.currentTrack = bound(local.drumSequencer.currentTrack, 0, numTracks);
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
                goDownState(STATE_DRUM_SEQUENCER_TRACK_VELOCITY);
                }
            }
        else
            {
            //// CHANGE POSITION
            int16_t newPos = drumSequencerGetNewCursorXPos(trackLen);
            if (lockoutPots ||      // using an external NRPN device, which is likely accurate
                local.drumSequencer.currentRightPot == DRUM_SEQUENCER_CURRENT_RIGHT_POT_UNDEFINED ||   // nobody's been entering data
                local.drumSequencer.currentRightPot >= newPos && local.drumSequencer.currentRightPot - newPos >= 2 ||
                local.drumSequencer.currentRightPot < newPos && newPos - local.drumSequencer.currentRightPot >= 2)
                {
                local.drumSequencer.currentEditPosition = newPos;
                local.drumSequencer.currentRightPot = DRUM_SEQUENCER_CURRENT_RIGHT_POT_UNDEFINED;
                }
            }
        }
        
        
    ///// INCOMING MIDI DATA

    // rerouting to new channel
    if (newItem && 
        itemType != MIDI_CUSTOM_CONTROLLER && 
        local.drumSequencer.performanceMode && 
        options.drumSequencerPlayAlongChannel != CHANNEL_ADD_TO_DRUM_SEQUENCER &&
        options.drumSequencerPlayAlongChannel != CHANNEL_PICK)
        {
        TOGGLE_IN_LED();
        // figure out what the channel should be
        uint8_t channelOut = options.drumSequencerPlayAlongChannel;
        if (channelOut == DRUM_SEQUENCER_CHANNEL_DEFAULT_MIDI_OUT)
            channelOut = options.channelOut;
                
        // send the appropriate command
        if (channelOut != DRUM_SEQUENCER_NO_MIDI_OUT)
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
            case CC_EXTRA_PARAMETER_M:
            case CC_EXTRA_PARAMETER_N:
            case CC_EXTRA_PARAMETER_O:
            case CC_EXTRA_PARAMETER_P:
            case CC_EXTRA_PARAMETER_Q:
            case CC_EXTRA_PARAMETER_R:
            case CC_EXTRA_PARAMETER_S:
            case CC_EXTRA_PARAMETER_T:
                {
                // Set Track
                uint8_t track = itemNumber - CC_EXTRA_PARAMETER_A;
                if (track < local.drumSequencer.numTracks)
                    local.drumSequencer.currentTrack = track;
                break;
                }
            case CC_EXTRA_PARAMETER_U:
                {
                // Set Drum Note
                immediateReturnState = STATE_DRUM_SEQUENCER_PLAY;
                goDownState(STATE_DRUM_SEQUENCER_TRACK_PITCH);
                break;
                }
            case CC_EXTRA_PARAMETER_V:
                {
                // Previous Group
                goPreviousGroup();
                break;
                }
            case CC_EXTRA_PARAMETER_W:
                {
                // Next Group
                goNextGroup();
                break;
                }
            case CC_EXTRA_PARAMETER_X:
                {
                // Toggle/Schedule Mute
                if (local.drumSequencer.performanceMode)
                    {
                    //// SCHEDULE MUTE
                    drumSequencerAdvanceFillOrMute(local.drumSequencer.currentTrack, DRUM_SEQUENCER_ACTION_MUTE);
                    }
                else
                    {
                    //// TOGGLE MUTE
                    local.drumSequencer.muted[local.drumSequencer.currentTrack] = !local.drumSequencer.muted[local.drumSequencer.currentTrack];
                    }
                break;
                }
            case CC_EXTRA_PARAMETER_Y:
                {
                // Toggle/Schedule Solo
                if (local.drumSequencer.performanceMode)
                    {
                    drumSequencerAdvanceSolo();
                    }
                else
                    {
                    local.drumSequencer.solo = !local.drumSequencer.solo;
                    }
                break;
                }
            case CC_EXTRA_PARAMETER_Z:
                {
                // Clear Group
                goDownState(STATE_DRUM_SEQUENCER_GROUP_CLEAR_SURE);
                break;
                }
            case CC_EXTRA_PARAMETER_1:
                {
                // Clear Track in Group
                clearCurrentTrackInGroup();
                break;
                }
            case CC_EXTRA_PARAMETER_2:
                {
                // Schedule Transition
                if (local.drumSequencer.performanceMode)
                    {
                    //// SCHEDULE TRANSITION
                    local.drumSequencer.goNextTransition = !local.drumSequencer.goNextTransition;       // this should convert > TRUE ("specific" transitions) to FALSE as well
                    }
                break;
                }
            case CC_EXTRA_PARAMETER_3:
                {
                // Toggle Click Track
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_CLICK);
                break;
                }
            case CC_EXTRA_PARAMETER_4:
                {
                // Set Mark
                goDownState(STATE_DRUM_SEQUENCER_MARK);
                break;
                }
            case CC_EXTRA_PARAMETER_5:
                {
                // Copy Track (Local)
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_LOCAL_COPY);
                break;
                }
            case CC_EXTRA_PARAMETER_6:
                {
                // Swap Tracks (Local)
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_LOCAL_SWAP);
                break;
                }
      
            // IN ORDER THOUGH IT DOESNT LOOK LIKE IT
      
            case CC_EXTRA_PARAMETER_11:
                {
                // UNUSED
                break;
                }
            case CC_EXTRA_PARAMETER_12:
                {
                // UNUSED
                break;
                }

            case CC_EXTRA_PARAMETER_13:
                {
                // Copy Whole Tracks
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_TRACK_COPY);
                break;
                }
            case CC_EXTRA_PARAMETER_14:
                {
                // Swap Whole Tracks
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_TRACK_SWAP);
                break;
                }
            case CC_EXTRA_PARAMETER_15:
                {
                // Copy Group
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_GROUP_COPY);
                break;
                }
            case CC_EXTRA_PARAMETER_16:
                {
                // Swap Groups
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_GROUP_SWAP);
                break;
                }
            case CC_EXTRA_PARAMETER_17:
                {
                // Schedule Next Sequence
                local.drumSequencer.goNextSequence = !local.drumSequencer.goNextSequence;
                break;
                }


            case CC_EXTRA_PARAMETER_7:
                {
                // Stamp Group
                goDownState(STATE_DRUM_SEQUENCER_GROUP_COPY_TO_NEXT);
                break;
                }
            case CC_EXTRA_PARAMETER_8:
                {
                // Accent Track
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_TRACK_ACCENT);
                break;
                }
            case CC_EXTRA_PARAMETER_9:
                {
                // Rotate Group
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_GROUP_ROTATE);
                break;
                }
            case CC_EXTRA_PARAMETER_10:
                {
                // (Performance Mode) Pause
                pauseDrumSequencer();
                break;
                }
                        
            // this is a discontinuity, hope compiler can handle it
                        
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_1:
                {
                // Set Group Speed Multiplier
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_DRUM_SEQUENCER_GROUP_SPEED);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_2:
                {
                // midi out
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_DRUM_SEQUENCER_TRACK_MIDI_CHANNEL_OUT);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_3:
                {
                // velocity
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_DRUM_SEQUENCER_TRACK_VELOCITY);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_4:
                {
                // group length
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                leftPotParameterEquivalent = true;
                goDownState(STATE_DRUM_SEQUENCER_GROUP_LENGTH);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_5:
                {
                // pattern
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_LOCAL_PATTERN);
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
                /*
              //// IRRELEVANT
              leftPotParameterEquivalent = true;
              AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
              goDownState(STATE_OPTIONS_TRANSPOSE);
              break;
                */
                // Toggle/Schedule Mute
                if (local.drumSequencer.performanceMode)
                    {
                    //// SCHEDULE FILL
                    drumSequencerAdvanceFillOrMute(local.drumSequencer.currentTrack, DRUM_SEQUENCER_ACTION_FILL);
                    }
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
                //// STUPID
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_NOTE_SPEED);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_10:
                {
                /*
             ///// IRRELEVANT
             leftPotParameterEquivalent = true;
             AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
             goDownState(STATE_OPTIONS_PLAY_LENGTH);
                */
                // Save
                leftPotParameterEquivalent = true;
                goDownState(STATE_DRUM_SEQUENCER_SAVE);
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
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_15:
                {
                // Select Transition
                leftPotParameterEquivalent = true;
                //AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                IMMEDIATE_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_DRUM_SEQUENCER_TRANSITION);
                break;
                }
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_16:
                {
                // Update pot as if it were a left parameter equivalent (see line 586 of MidiInterface)
                uint16_t equivalentPot = (itemValue >> 4); 
                //// CHANGE GROUP
                uint8_t group = ((equivalentPot * local.drumSequencer.numGroups) >> 10);         //  / 1024;
                group = bound(group, 0, local.drumSequencer.numGroups);
                drumSequencerChangeGroup(group);
                break;
                }
            // Do I need any more of these?
            case CC_LEFT_POT_PARAMETER_EQUIVALENT_6_LSB:
                {
                leftPotParameterEquivalent = true;
                AUTO_RETURN(STATE_DRUM_SEQUENCER_PLAY);
                goDownState(STATE_OPTIONS_TEMPO);
                break;
                }
            }
        }

  
    else if (bypass)
        {
        // do nothing
        }
        
    // everything after this should be denied if we're in bypass

    else if (newItem && (itemType == MIDI_NOTE_ON) && 
        local.drumSequencer.performanceMode && 
        options.drumSequencerPlayAlongChannel == CHANNEL_PICK)
        {
        if (itemNumber >= options.drumSequencerControllerCenter && itemNumber < (options.drumSequencerControllerCenter + 34))
            {
            const int8_t keys[34] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6, 7, -1, 8, -1, 9, 10, -1, 11, -1, 12, -1, 13, 14, -1, 15, -1, 16, 17, -1, 18, -1, 19 }; 
            itemNumber -= options.drumSequencerControllerCenter;
            if (keys[itemNumber] != -1)
                {
                local.drumSequencer.goNextTransition = keys[itemNumber] + 2;            // load a "specific" transition into goNextTransition, not just TRUE
                }
            }
        }
    else if (newItem && (itemType == MIDI_NOTE_ON))   //// there is a note played
        {
        TOGGLE_IN_LED();
        uint8_t note = itemNumber;
        uint8_t len = getGroupLength(local.drumSequencer.currentGroup);

        /// We have different ways of entering drum note information depending on the edit mode
                        
        uint16_t octave = div12(note + MIDDLE_C - options.drumSequencerControllerCenter);               // adjust using the keyboard center
        
        if (octave >= 5)  // middle c and up
            {
            int8_t key = drumSequencerGetKey(octave, note + MIDDLE_C - options.drumSequencerControllerCenter);          // adjust using the keyboard center
            if (local.drumSequencer.currentEditPosition >= 0 && local.drumSequencer.currentEditPosition < len)
                {
                if (local.drumSequencer.drumRegion < 0)
                    local.drumSequencer.drumRegion = 0;
                            
                //// We're in EDIT MODE

                // STRATEGY:
                // If the key is >= 0, it reflects the drum beat we want to toggle, plus the octave.
                // These are WHITE NOTES
                // If the key is -1, -2, -3, or -4, it reflects the REGION of the drum beat (we have four regions because we have up to 0...63).
                // These are the BLACK NOTES Db, Eb, Gb, and Ab
                // If the key is -6, means to toggle the CURRENT NOTE
                // This is the BLACK NOTE Bb
                                
                if ((octave == 7 && key > 1) || octave > 7)
                    {
                    local.drumSequencer.accent = true;
                    }
                else if (key >= 0)
                    {
                    if (octave == 5 || octave == 6 || (octave == 7 && (key == 0 || key == 1)))              // only valid ones
                        {
                        // compute the proper key
                        if (octave == 6) key += 7;
                        else if (octave == 7) key += 14;
                                                
                        // at this point key should range from 0...15

                        uint8_t toggle = key + 16 * local.drumSequencer.drumRegion;
                        
                        // doing this without % is painful
                        if (toggle >= len)
                            {
                            if (len == 8)
                                {
                                toggle = toggle - ((toggle >> 3) << 3);         // toggle = toggle % 8
                                }
                            else if (len == 16)
                                {
                                toggle = toggle - ((toggle >> 4) << 4);         // toggle = toggle % 16
                                }
                            else if (len == 32)
                                {
                                toggle = toggle - ((toggle >> 5) << 5);         // toggle = toggle % 32
                                }
                            }
                                
                        uint8_t track = local.drumSequencer.currentTrack;
                        if (local.drumSequencer.accent && track < local.drumSequencer.numTracks - 1)  // there's another track
                            {
                            track++;
                            }
                        toggleNote(local.drumSequencer.currentGroup, track, toggle);
                        }
                    }
                else if (key == -6 && octave == 5)
                    {
                    uint8_t track = local.drumSequencer.currentTrack;
                    if (local.drumSequencer.accent && track < local.drumSequencer.numTracks - 1)  // there's another track
                        {
                        track++;
                        }
                    setNote(local.drumSequencer.currentGroup, track, local.drumSequencer.currentEditPosition);
                    local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, len);
                    }
                else if (key == -6 && octave == 6)
                    {
                    uint8_t track = local.drumSequencer.currentTrack;
                    if (local.drumSequencer.accent && track < local.drumSequencer.numTracks - 1)  // there's another track
                        {
                        track++;
                        }
                    clearNote(local.drumSequencer.currentGroup, track, local.drumSequencer.currentEditPosition);
                    local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, len);
                    }
                else if (key == -6 && octave == 7)
                    {
                    uint8_t track = local.drumSequencer.currentTrack;
                    if (local.drumSequencer.accent && track < local.drumSequencer.numTracks - 1)  // there's another track
                        {
                        track++;
                        }
                    toggleNote(local.drumSequencer.currentGroup, track, local.drumSequencer.currentEditPosition);
                    local.drumSequencer.currentEditPosition = incrementAndWrap(local.drumSequencer.currentEditPosition, len);
                    }
                else
                    {
                    local.drumSequencer.drumRegion = (0 - key - 1);
                    }
                }
            else
                {
#define DRUM_OPERATION_TOGGLE (-1)
#define DRUM_OPERATION_SET (-3)
#define DRUM_OPERATION_CLEAR (-2)
                //#define DRUM_OPERATION_CLEAR_ALWAYS (-4)

                if (local.drumSequencer.drumRegion >= 0)
                    local.drumSequencer.drumRegion = DRUM_OPERATION_TOGGLE;
                                        
                // Try to add some slop so the user can come in at the right time
                uint8_t pos = local.drumSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0);
                if (pos >= len) pos = 0;

                //// We're in PLAY POSITION MODE or RIGHT mode
                                
                // STRATEGY:
                // If the key is >= 0, it reflects the TRACK we want to apply our operation to, plus the octave.
                // These are WHITE NOTES
                // If the key is -1, it turns on TOGGLING the note
                // If the key is -2, it turns on SETTING the note
                // If the key is -3, it turns on CLEARING the note
                // If the key is -4, it does nothing [for now]
                // These are the BLACK NOTES Db, Eb, Gb, and Ab
                // If the key is -6, means to apply the opertion to the the CURRENT NOTE
                // This is the BLACK NOTE Bb
                                
                if (key >= 0)   // perform the operation on the track indicated
                    {
                    uint8_t track = ((octave - 5) * 7 + key);
                    if (track < local.drumSequencer.numTracks)
                        {
                        if (local.drumSequencer.drumRegion == DRUM_OPERATION_TOGGLE)
                            {
                            uint8_t n = getNote(local.drumSequencer.currentGroup, track, pos);
                            setNote(local.drumSequencer.currentGroup, track, pos, !n);
                            if (n == 0) 
                                sendTrackNote(track);
                            }
                        else if (local.drumSequencer.drumRegion == DRUM_OPERATION_SET)
                            {
                            setNote(local.drumSequencer.currentGroup, track, pos, 1);
                            sendTrackNote(track);
                            }
                        else // if (local.drumSequencer.drumRegion == DRUM_OPERATION_SET)
                            {
                            setNote(local.drumSequencer.currentGroup, track, pos, 0);
                            }
                        }
                    }
                else if (key == -6)     // perform the operation locally
                    {
                    if (local.drumSequencer.drumRegion == DRUM_OPERATION_TOGGLE)
                        {
                        uint8_t n = getNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, pos);
                        setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, pos, !n);
                        if (n == 0) 
                            sendTrackNote(local.drumSequencer.currentTrack);
                        }
                    else if (local.drumSequencer.drumRegion == DRUM_OPERATION_SET)
                        {
                        setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, pos, 1);
                        sendTrackNote(local.drumSequencer.currentTrack);
                        }
                    else // if (local.drumSequencer.drumRegion == DRUM_OPERATION_SET)
                        {
                        setNote(local.drumSequencer.currentGroup, local.drumSequencer.currentTrack, pos, 0);
                        }
                    }
                else if (key == -4)  // just play the note
                    {
                    // sendTrackNote(local.drumSequencer.currentTrack);                     // we're not doing this right now, it's confusing
                    }
                else // key is -1, -2, -3                       // set the operation
                    {
                    local.drumSequencer.drumRegion = key;
                    }
                }
            }
        else
            {
            sendTrackNote(local.drumSequencer.currentTrack);
            }
        }
    else if (newItem && (itemType == MIDI_NOTE_OFF))
        {
        TOGGLE_IN_LED();
        uint8_t note = itemNumber;

        /// We have different ways of entering drum note information depending on the edit mode
                        
        uint16_t octave = div12(note + MIDDLE_C - options.drumSequencerControllerCenter);               // adjust using the keyboard center
                
        if (octave >= 7 || octave < 5)
            {
            int8_t key = drumSequencerGetKey(octave, note + MIDDLE_C - options.drumSequencerControllerCenter);
                        
            if ((octave == 7 && key > 1) || octave > 7 || octave < 5)
                {
                local.drumSequencer.accent = false;
                }
            }
        }
    
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

    playDrumSequencer();
    if (updateDisplay)
        {
        uint8_t playGroup = (local.drumSequencer.scheduleFill == DRUM_SEQUENCER_FILL_ON ?
            local.drumSequencer.fillGroup : local.drumSequencer.currentGroup);
        drawDrumSequencer(playGroup, true);
        }
    }

    

#endif INCLUDE_DRUM_SEQUENCER

