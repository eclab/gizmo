#include "All.h"




// Starting at position pos, draws up to next SEVEN notes of the arpeggio.  
// We leave a one-column space so as not to interfere with the right LED matrix
void drawArpeggio(uint8_t* mat, uint8_t pos, uint8_t editCursor, uint8_t len = 7)
    {
    clearMatrix(mat);

    uint8_t maxNote = 0;  // it's okay that this isn't -1 I think
    uint8_t minNote = ARP_REST;
    for(uint8_t i = 0; i < data.arp.length; i++)
        {
        uint8_t n = ARP_NOTEX(i);
        if (n == ARP_REST) continue;  // don't count rests are part of this
        if (n > maxNote) maxNote = n;
        if (n < minNote) minNote = n;
        }
    
    if (minNote != ARP_REST) // it's not all rests (or empty)
        {
        uint8_t interval = maxNote - minNote + 1;

        uint8_t j = 0;
        uint8_t i = 0;
        // we assume we're drawing a user-defined matrix, so we're using local.arp.currentPosition
        for(i = pos; i < data.arp.length; i++)
            {
            j = i - pos;
            if (j >= len) break;
                
            uint8_t n = ARP_NOTEX(i);
            if (n == ARP_REST)
                continue;
            else if (interval > 8)
                {
                setPoint(mat, j, n >> 1);
                if (n & 1 == 1)  // it's odd, add another point
                    setPoint(mat, j, (n >> 1) + 1);
                }
            else
                {
                setPoint(mat, j, n);
                }
            }
        }
                
    if (editCursor == EDIT_CURSOR_START)
        {
        // use the cursor to indicate where we're playing
        blinkPoint(mat, 0, 0);
        }
    else if (editCursor == EDIT_CURSOR_POS)
        {
        int8_t point = local.arp.currentPosition - pos;
        if (point >= 0 && point <= len)
            // draw at pos + 1
            blinkPoint(mat, point , 0);
        }
    }
                


// Plays a note, multiplied by the given octave, and registers
// the timestamp for it to be turned off.
void playArpeggiatorNote(uint16_t note)
    {
    if (note < 0 || note >= 127)
        return;

    sendNoteOn(local.arp.noteOff = (uint8_t) note, local.arp.velocity, options.channelOut);
                
    // this will be costly but maybe it's better than / for 32-bit?
    local.arp.offTime = currentTime + div100(notePulseRate * getMicrosecsPerPulse() * options.noteLength);
    } 


// Continue to play the arpeggio
void playArpeggio()
    {
    // turn off previous note?
    // NOTE: if noteLength==100, this won't sound in time.  So we need to erase it anyway.
    // Hence the "notePulse" bit
    if (local.arp.noteOff != NO_NOTE && local.arp.offTime != 0 && (notePulse || currentTime >= local.arp.offTime))
        {
        if (local.arp.number == ARPEGGIATOR_NUMBER_CHORD_REPEAT)
            {
            sendAllNotesOff();
            }
        else
            {
            sendNoteOff(local.arp.noteOff, 127, options.channelOut);
            }
        local.arp.noteOff = NO_NOTE;
        }

    if (notePulse)
        {
        if (local.arp.numChordNotes > 0)
            {
            if (local.arp.number <= ARPEGGIATOR_NUMBER_ASSIGN)
                {
                int16_t max = local.arp.numChordNotes * (int16_t)(options.arpeggiatorPlayOctaves + 1) - 1;
                switch(local.arp.number)
                    {
                    case ARPEGGIATOR_NUMBER_ASSIGN:
                    	// Fall Thru
                    	// [The magic here is that we do exactly the same as UP, except that in arpeggiatorAddNote we don't insert the note in order, but just tack it on the end!]
                    case ARPEGGIATOR_NUMBER_UP:
                        {
                        if (local.arp.currentPosition == ARP_POSITION_START)
                            {
                            local.arp.currentPosition = 0;
                            }
                        else
                            {
                            // though local.arp.currentPosition is signed and incrementAndWrap expects unsigned, it's okay
                            // because local.arp.currentPosition will never be < 0 at this point, nor > 127.
                            local.arp.currentPosition = incrementAndWrap(local.arp.currentPosition, max + 1);
                            }
                        }
                    break;
                    case ARPEGGIATOR_NUMBER_DOWN:
                        {
                        if (local.arp.currentPosition == ARP_POSITION_START)
                            {
                            local.arp.currentPosition = max;
                            }
                        else
                            {
                            local.arp.currentPosition--;
                            if (local.arp.currentPosition < 0)
                                local.arp.currentPosition = max;
                            }
                        }
                    break;
                    case ARPEGGIATOR_NUMBER_UP_DOWN:
                        {
                        
                        if (!local.arp.goingDown)
                            {
                            local.arp.currentPosition++;
                            }
                        else if (local.arp.goingDown)
                            {
                            local.arp.currentPosition--;
                            }
                        
                        if (local.arp.currentPosition < 0)
                            {
                            local.arp.goingDown = 0;
                            local.arp.currentPosition = ((local.arp.numChordNotes == 1) ? 0 : 1);
                            }
                        else if (local.arp.currentPosition >= max)
                            {
                            local.arp.goingDown = 1;
                            local.arp.currentPosition = max;
                            }
                        }
                    break;
                    case ARPEGGIATOR_NUMBER_RANDOM:
                        {
                        if (local.arp.numChordNotes > 1)
                            {
                            // we want semi-random: don't play the same note twice
                            uint8_t newPosition;
                            do
                                {
                                newPosition = random(max + 1);
                                }
                            while(newPosition == local.arp.currentPosition);
                            local.arp.currentPosition = newPosition;
                            }
                        else local.arp.currentPosition = 0;
                        }
                    break;
                    }

                // figure out the actual note to play, and play it

                uint16_t octave = local.arp.currentPosition / local.arp.numChordNotes;
                uint16_t note = local.arp.currentPosition - (octave * local.arp.numChordNotes);  // mod

                playArpeggiatorNote((local.arp.chordNotes[note] & 127) + 12 * octave);
                }
            else if (local.arp.number == ARPEGGIATOR_NUMBER_CHORD_REPEAT)
                {
                for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
                    {
                    playArpeggiatorNote((local.arp.chordNotes[i] & 127));
                    }
                }
            else
                {
                // we are using local.arp.currentPosition for another purposes here -- to
                // be the index into the notes array
                if (data.arp.length != 0)
                    {
                    local.arp.currentPosition++;
                    if (local.arp.currentPosition >= data.arp.length)
                        local.arp.currentPosition = 0;

                    int8_t octave = 0;  // note that this is signed
                    int8_t notei = ARP_NOTEX(local.arp.currentPosition);
                    if (notei != ARP_REST)
                        {
                        // this computes the interval between the largest and smallest notes, and rounds up to the nearest
                        // octave, in notes (12 notes to an octave).  We'll use that to determine how many "octaves" to jump
                        // when we need to jump one.
                        int16_t octaveJump = (local.arp.chordNotes[local.arp.numChordNotes - 1] - local.arp.chordNotes[0] + 1) / 12 + 1; 
                        notei -= data.arp.root;  // shift relative to root
                        while (notei < 0) { notei += local.arp.numChordNotes; octave--; }
                        while (notei >= local.arp.numChordNotes) { notei -= local.arp.numChordNotes; octave++; }
                        
                        int16_t note = ((local.arp.chordNotes[notei] & 127) + (octave * octaveJump * 12));  // I presume [0] is the root
                        
                        if (note >= 0 && note <= 127)
                            playArpeggiatorNote((uint8_t) note);
                        }
                    }                                                       
                }
            }
        else local.arp.currentPosition = ARP_POSITION_START;
        }
    
    if (updateDisplay)
        {
        // draw latch
        if (options.arpeggiatorLatch)
            setPoint(led, 7, 1);
        }
    }


// Remove a note from chordNotes, or mark it if latch mode is on.  O(n) :-(
void arpeggiatorRemoveNote(uint8_t note)
    {
    for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
        {
        if ((local.arp.chordNotes[i] & 127) == note)
            {
            if (options.arpeggiatorLatch)
                {
                // just mark the note
                local.arp.chordNotes[i] = local.arp.chordNotes[i] | 128;
                }
            else
                {
                // note overlapping regions, so we're using memmove to do the shift
                memmove(&local.arp.chordNotes[i], &local.arp.chordNotes[i+1], local.arp.numChordNotes - i - 1);
                local.arp.numChordNotes--;
                break;
                }
            }
        }
    }
        


// Add a note to chordNotes.
void arpeggiatorAddNote(uint8_t note)
    {
    // remove latched notes if ALL of them are marked
    uint8_t marked = 0;
    for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
        {
        if (local.arp.chordNotes[i] & 128)
            marked++;
        }
    if (marked == local.arp.numChordNotes)  // they're all marked, time to reset
        local.arp.numChordNotes = 0;

    if (local.arp.numChordNotes == MAX_ARP_CHORD_NOTES)  // at this stage, of we're still full, someone's holding down a lot of notes!
        return;

    // Find the note.  If it's there, return (we're not inserting it)
    for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
        {
        uint8_t n = (local.arp.chordNotes[i] & 127);
        if (n == note)
            {
            local.arp.chordNotes[i] = n;  // unmark just in case
            return;
            }
        }

    // reset up/down if necessary
    if (local.arp.numChordNotes == 0)
        local.arp.goingDown = 0;

    // add note                        
    if ((local.arp.numChordNotes == 0) || 
    	((local.arp.chordNotes[local.arp.numChordNotes - 1] & 127) < note) || 
    	(local.arp.number == ARPEGGIATOR_NUMBER_ASSIGN))  // just append us
        {
        local.arp.chordNotes[local.arp.numChordNotes] = note;
        local.arp.numChordNotes++;
        }
    else                // insert us
        {
        for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
            {
            if ((local.arp.chordNotes[i] & 127) > note)  // found it
                {
                // make space
                // note overlapping regions, so we're using memmove to do the shift
                memmove(&local.arp.chordNotes[i + 1], &local.arp.chordNotes[i], local.arp.numChordNotes - i);
                local.arp.chordNotes[i] = note;
                local.arp.numChordNotes++;
                break;
                }
            }
        }
    }

// Choose an arpeggiation, or to create one
void stateArpeggiator()
    {
    uint8_t result;
    if (entry)
        {
        local.arp.numChordNotes = 0;  // same reason     
        local.arp.currentPosition = ARP_POSITION_START;  // same reason
        local.arp.offTime = 0;  // same reason
        local.arp.goingDown = 0;  // same reason
        local.arp.playing = 0;  // don't want to add and remove notes right now
        sendAllNotesOff();
        }
    const char* menuItems[17] = { PSTR(STR_UP), PSTR(STR_DOWN), PSTR(STR_UP_DOWN), PSTR("RANDOM"), PSTR("ASSIGN"), PSTR("CHORD"), PSTR("0"), PSTR("1"), PSTR("2"), PSTR("3"), PSTR("4"), PSTR("5"), PSTR("6"), PSTR("7"), PSTR("8"), PSTR("9"), PSTR("CREATE")};
    result = doMenuDisplay(menuItems, 17, STATE_NONE,  STATE_ROOT, 1);

    entry = true;
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            entry = false;
            }
        break;
        case MENU_SELECTED:
            {
            if (currentDisplay == ARPEGGIATOR_NUMBER_CREATE)
                {
                goDownState(STATE_ARPEGGIATOR_CREATE);
                }
            else
                {
                goDownState(STATE_ARPEGGIATOR_PLAY);
                local.arp.number = currentDisplay;
                }
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_ROOT);
            }
        break;
        }
    }

GLOBAL static uint8_t arpeggiatorGlyphs[6] = { GLYPH_3x5_UP, GLYPH_3x5_DOWN, GLYPH_3x5_UP_DOWN, GLYPH_3x5_R, GLYPH_3x5_A, GLYPH_3x5_C };

// Handle the menu structure for playing an arpeggio
void stateArpeggiatorPlay()
    {
    // Select button: selects pot functions
    // Middle button: changes latch
            
    uint8_t result;
    if (entry)
        {
        local.arp.playing = 1;
        // Load the arpeggiator data
        if (local.arp.number > ARPEGGIATOR_NUMBER_CHORD_REPEAT)
            {
            LOAD_ARPEGGIO(local.arp.number - ARPEGGIATOR_NUMBER_CHORD_REPEAT - 1);
            }
        }
    const char* menuItems[2] = { PSTR("OCTAVES"), options_p };
    result = doMenuDisplay(menuItems, 2, STATE_NONE, 0, 1, 8);
        
    if (updateDisplay)
        {    
        if (local.arp.number > ARPEGGIATOR_NUMBER_CHORD_REPEAT)
            {
            uint8_t pos = 0;
            if (local.arp.currentPosition >= 7) 
                pos = local.arp.currentPosition - 6;
            drawArpeggio(led2, pos, EDIT_CURSOR_POS);
            }
        else 
            {
            clearMatrix(led2);
            write3x5Glyph(led2 , arpeggiatorGlyphs[local.arp.number], 0);
            }
        }
 
    entry = true;
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            entry = false;
            }
        break;
        case MENU_SELECTED:
            {
            switch(currentDisplay)
                {
#define ARPEGGIATOR_PLAY_OCTAVES 0
#define ARPEGGIATOR_PLAY_OPTIONS 1

                case ARPEGGIATOR_PLAY_OCTAVES:
                    {
                    goDownState(STATE_ARPEGGIATOR_PLAY_OCTAVES);
                    //state = STATE_ARPEGGIATOR_PLAY_OCTAVES;
                    }
                break;
                case ARPEGGIATOR_PLAY_OPTIONS:
                    {
                    //suggestedDefaultState = NO_SUGGESTED_DEFAULT_STATE;
                    optionsReturnState = STATE_ARPEGGIATOR_PLAY;
                    goDownState(STATE_OPTIONS);
                    //state = STATE_OPTIONS;
                    //entry = true;
                    }
                break;                    
                }
            }
        break;
        case MENU_CANCELLED:
            {
            goUpState(STATE_ARPEGGIATOR);
            }
        break;
        }

    if (result == NO_MENU_SELECTED)
        {
        if (isUpdated(MIDDLE_BUTTON, PRESSED))
            {
            options.arpeggiatorLatch = !options.arpeggiatorLatch;
            saveOptions();
            //local.arp.numChordNotes = 0;  // reset
            }
        }

    playArpeggio();          
    }
        
        
void garbageCollectNotes()
    {
    // We first need to delete notes beyond currentPosition, if the user dialed back
    // and then started playing
    if (local.arp.currentPosition < data.arp.length)  // we moved back
        {
        // mark the used notes
        for(uint8_t i = 0; i < local.arp.currentPosition; i++)
            {
            local.arp.chordNotes[ARP_NOTEX(i)] |= 128;
            }
                        
        // swap out the junk notes
        for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
            {
            if ((local.arp.chordNotes[i] & 128) == 0)
                {
                local.arp.chordNotes[i] = local.arp.chordNotes[local.arp.numChordNotes - 1];
                for(uint8_t j = 0; j < local.arp.currentPosition; j++)
                    if (ARP_NOTEX(j) == local.arp.numChordNotes - 1)
                        SET_ARP_NOTEX(j, i);
                i--;
                local.arp.numChordNotes--;
                }
            else // restore
                {
                local.arp.chordNotes[i] &= 127;
                }
            }
        }
    }



// Handle the screen for creating an arpeggio.  This first chooses the root.
void stateArpeggiatorCreate()
    {
    uint8_t note = stateEnterNote(GLYPH_ROOT, STATE_ARPEGGIATOR);
    if (note != NO_NOTE)  // it's a real note
        {
        data.arp.root = note;
        state = STATE_ARPEGGIATOR_CREATE_EDIT;
        entry = true;
        }
    }


// Handle the screen for editing an arpeggio.
void stateArpeggiatorCreateEdit()
    {
    if (entry)
        {
        local.arp.number = 0;
        local.arp.currentPosition = 0;
        data.arp.length = 0; 
        local.arp.numChordNotes = 0;
        newItem = 0;
        // clear out everything
        for(uint8_t i = 0; i < MAX_ARP_NOTES; i++)
            SET_ARP_NOTEX(i, ARP_REST);
        local.arp.currentRightPot = -1;
        entry = false;
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        sendAllNotesOff();
        state = STATE_ARPEGGIATOR_CREATE_SURE;
        }
    else if (isUpdated(SELECT_BUTTON, PRESSED))
        {
        // save!
        sendAllNotesOff();
        state = STATE_ARPEGGIATOR_CREATE_SAVE;
        entry = true;
        }
    else if (isUpdated(MIDDLE_BUTTON, PRESSED))
        {
        local.arp.currentRightPot = (uint8_t) ((pot[RIGHT_POT] * ((uint16_t) data.arp.length + 1)) >> 10);  //  / 1024);

        sendAllNotesOff();
        if (local.arp.currentPosition < MAX_ARP_NOTES)
            {
            garbageCollectNotes();
                
            // add a rest
            SET_ARP_NOTEX(local.arp.currentPosition, ARP_REST);
            local.arp.currentPosition++;
            data.arp.length = local.arp.currentPosition;
            }
        }
    else if (newItem == NEW_ITEM && itemType == MIDI_NOTE_ON)
        {
        local.arp.currentRightPot = (uint8_t) ((pot[RIGHT_POT] * ((uint16_t) data.arp.length + 1)) >> 10);  //  / 1024);

        sendAllNotesOff();
        if (local.arp.currentPosition < MAX_ARP_NOTES)
            {                       
            garbageCollectNotes();
                        
            // find or create the index of the note
            uint8_t index = 255;
            for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
                {
                if (local.arp.chordNotes[i] == itemNumber)
                    {
                    index = i;
                    break;
                    }
                }
                        
            if (index != 255 || local.arp.numChordNotes < MAX_ARP_CHORD_NOTES)  // we've got space or it's already in our table
                {
                if (index == 255)
                    {
                    index = local.arp.numChordNotes;
                    local.arp.chordNotes[index] = itemNumber;
                    local.arp.numChordNotes++;
                    }

                local.arp.lastVelocity = itemValue;
                sendNoteOn(itemNumber, itemValue, options.channelOut);
                                                                
                SET_ARP_NOTEX(local.arp.currentPosition, index);
                local.arp.currentPosition++;
                data.arp.length = local.arp.currentPosition;
                
                // RESORTING
                // At this point, chordNotes contains some numChordNotes notes,
                // and data.arp... contains indexes into this array.  We want to
                // sort the chordNotes array and ALSO reassign the data.arp array.

                // First, we back up chordNotes
                uint8_t backupChordNotes[local.arp.numChordNotes];
                memcpy(backupChordNotes, local.arp.chordNotes, local.arp.numChordNotes);
 				
                // Next we sort chordNotes low to high.  Insertion sort FTW
                int8_t j;  // j goes negative so we have to be signed
                for(uint8_t i = 1; i < local.arp.numChordNotes; i++)
                    {
                    uint8_t val = local.arp.chordNotes[i];
                    j = i - 1;
                    while((j >= 0) && (local.arp.chordNotes[j] > val))
                        {
                        local.arp.chordNotes[j + 1] = local.arp.chordNotes[j];
                        j = j - 1;
                        }
                    local.arp.chordNotes[j + 1] = val;
                    }
                    
                // Now we reassign chordNotes in the ARP values.  This is O(n^2) :-(
                for(uint8_t i = 0; i < data.arp.length; i++)    
                    {
                    uint8_t notei = ARP_NOTEX(i);
                    if (notei != ARP_REST)
                        {
                        uint8_t note = backupChordNotes[notei];
                                        
                        // find it
                        for(uint8_t j = 0; j < local.arp.numChordNotes; j++)
                            {
                            if (local.arp.chordNotes[j] == note)  // got it
                                {
                                SET_ARP_NOTEX(i, j);
                                break;
                                }
                            }
                        }
                    }
                }
            }
        }
    else if (newItem == NEW_ITEM && itemType == MIDI_NOTE_OFF)
        {
        sendNoteOff(itemNumber, itemValue, options.channelOut);
        }
    else if (potUpdated[RIGHT_POT])
        {
        uint8_t oldpos = local.arp.currentPosition;
        // set to a value between 0 and arpMaxPosition inclusive
        uint8_t newpos = (uint8_t) ((pot[RIGHT_POT] * ((uint16_t) data.arp.length + 1)) >> 10);  //  / 1024);
        
        if (lockoutPots ||                                                              // the potUpdated came from NRPN
            local.arp.currentRightPot == -1 ||              // this is the first time data is being updated
            local.arp.currentRightPot >= newpos && local.arp.currentRightPot - newpos >= 2 ||  // big enough change
            local.arp.currentRightPot < newpos && newpos - local.arp.currentRightPot >= 2)     // big enough change
            {
            local.arp.currentPosition = newpos;
            if (local.arp.currentPosition > data.arp.length)
                local.arp.currentPosition = data.arp.length;
            if (oldpos != local.arp.currentPosition)
                {
                sendAllNotesOff();
                if (local.arp.currentPosition > 0)
                    sendNoteOn(local.arp.chordNotes[ARP_NOTEX(local.arp.currentPosition - 1)], local.arp.lastVelocity, options.channelOut);
                }
            local.arp.currentRightPot = -1;
            }
        }
    
    if (updateDisplay)
        {
        clearScreen();
        int8_t drawPos = local.arp.currentPosition - 7;
        if (drawPos < 0) 
            drawPos = 0; 

        drawArpeggio(led2, drawPos, (local.arp.currentPosition < MAX_ARP_NOTES ? EDIT_CURSOR_POS : NO_EDIT_CURSOR));
        
        // draw the current note
        if (local.arp.currentPosition == 0)
            {
            // Draw nothing
            }
        else
            {
            uint8_t val = ARP_NOTEX(local.arp.currentPosition - 1);
            if (val == ARP_REST)
                {
                write3x5Glyph(led, GLYPH_3x5_R, 1);
                }
            else
                {
                writeNotePitch(led, local.arp.chordNotes[val]);
                }
            }
                        
        // draw latch
        if (options.arpeggiatorLatch)
            setPoint(led, 7, 1);
        }
        
    newItem = 0;
    }


// Handle the screen for saving an arpeggio.  
void stateArpeggiatorCreateSave()
    {
    uint8_t firstEmptySlot = 0;
    for(uint8_t i = 0; i < NUM_ARPS; i++)
        {
        if (!ARPEGGIO_IS_NONEMPTY(i))
            { firstEmptySlot = i; break; }
        }
                
    uint8_t result = doNumericalDisplay(0, NUM_ARPS - 1, firstEmptySlot, 0, OTHER_NONE);
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            data.arp.length = local.arp.currentPosition;
            // convert root from a note to an index into numChordNotes
            uint8_t r = 0;
            for(uint8_t i = 0; i < local.arp.numChordNotes; i++)
                {
                if (local.arp.chordNotes[i] >= data.arp.root)
                    { 
                    r = i;
                    break;
                    }
                }
            data.arp.root = r;
            SAVE_ARPEGGIO(currentDisplay);
            state = STATE_ARPEGGIATOR;
            entry = true;
            }
        break;
        case MENU_CANCELLED:
            {
            state = STATE_ARPEGGIATOR_CREATE_EDIT;
            }
        break;
        }
    }




