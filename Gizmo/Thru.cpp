////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

#if defined(__MEGA__)

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
				channel = (options.channelOut + i - 1) % NUM_MIDI_CHANNELS + 1;
				for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
					{
					sendNoteOff(note, velocity, channel);
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
		uint8_t note = options.thruChordMemory[i] - options.thruChordMemory[0] + note;  // can't overflow, it'll only go to 254 (127 + 127).
		if (note <= 127)
			sendNoteOff(note, velocity, channel);
		}
	}

void stateThruPlay()
    {
    if (entry)
        {
        sendAllNotesOff();
        resetDistributionNotes();
        local.thru.debounceState = DEBOUNCE_STATE_OFF;
        entry = false;
        }
                        
    if (updateDisplay)
        {
        clearScreen();
        write3x5Glyphs(GLYPH_PLAY);
        }
        
    // here we check if it's time to submit a NOTE OFF
    if (local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED && 
		local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000 <= currentTime)
    	{
		performThruNoteOff(local.thru.debounceNote, 127, options.channelOut);
		local.thru.debounceState = DEBOUNCE_STATE_OFF;
    	}

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_THRU);
        sendAllNotesOff();
        }

    else if (!bypass && newItem && (itemChannel == options.channelIn || options.channelIn == CHANNEL_OMNI) && options.channelOut != 0)
        {
        uint8_t channel = options.channelOut;
                
        if (itemType == MIDI_NOTE_ON)
            {
            if (itemNumber != local.thru.debounceNote ||
            	local.thru.debounceState == DEBOUNCE_STATE_OFF ||
            	((local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED ||
            	  local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_UP) &&
            	  local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000 <= currentTime))
            	{
        		if (options.thruDebounceMilliseconds != 0)
        			{
        			// set up state machine
        			local.thru.debounceState = DEBOUNCE_STATE_FIRST_NOTE_DOWN;
        			local.thru.debounceTime = currentTime;
        			local.thru.debounceNote = itemNumber;
        			}
        			
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
					local.thru.distributionNotes[local.thru.currentDistributionChannelIndex] = itemNumber;
					local.thru.currentDistributionChannelIndex++;
					if (local.thru.currentDistributionChannelIndex > options.thruNumDistributionChannels )  // yes, it's > not >= because options.thruNumDistributionChannels starts at *1*
						local.thru.currentDistributionChannelIndex = 0;
					}
	
				// NOTE REPLICATION
				for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
					{
					sendNoteOn(itemNumber, itemValue, channel);
					}
				
				// CHORD MEMORY
				for(uint8_t i = 1; i < options.thruChordMemorySize; i++)  // Yes, I see the *1*.  We aren't playing the bottom note a second time
					{
					uint8_t note = options.thruChordMemory[i] - options.thruChordMemory[0] + itemNumber;  // can't overflow, it'll only go to 254 (127 + 127).
					if (note <= 127)
						sendNoteOn(note, itemValue, channel);
					}
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
            if (local.thru.debounceState == DEBOUNCE_STATE_FIRST_NOTE_DOWN && 
            	local.thru.debounceNote == itemNumber)
            	{
            	if (local.thru.debounceTime + ((uint32_t)options.thruDebounceMilliseconds) * 1000 >= currentTime)            			
					{
					local.thru.debounceState = DEBOUNCE_STATE_FIRST_NOTE_UP_IGNORED;
					local.thru.debounceTime = currentTime;
					}
				else
					{
					performThruNoteOff(itemNumber, itemValue, channel);
					local.thru.debounceState = DEBOUNCE_STATE_OFF;
					
					// if we instead do the below behavior, then a rapid note-down after a long
					// note down and a note up will be ignored.  This might be good for sudden
					// drops in holding down the note, but I think it's better to just debounce
					// at the very beginning
					
					//local.thru.debounceState = DEBOUNCE_STATE_FIRST_NOTE_UP;
					//local.thru.debounceTime = currentTime;
					}
				}
			else		// we don't care about this one
				{
				performThruNoteOff(itemNumber, itemValue, channel);
				}
			}
        else if (itemType == MIDI_AFTERTOUCH_POLY)
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
					if (local.thru.distributionNotes[i] == itemNumber)
						{
						channel = (options.channelOut + i - 1) % NUM_MIDI_CHANNELS + 1;
						for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
							{
							sendPolyPressure(itemNumber, itemValue, channel);
							}
						}
					}
				}
			else
				{
				// NOTE REPLICATION
				for(uint8_t i = 0; i <= options.thruExtraNotes; i++)            // do at least once
					{
					sendPolyPressure(itemNumber, itemValue, channel);
					}
				}

			// CHORD MEMORY
			for(uint8_t i = 1; i < options.thruChordMemorySize; i++)  // Yes, I see the *1*.  We aren't playing the bottom note a second time
				{
				uint8_t note = options.thruChordMemory[i] - options.thruChordMemory[0] + itemNumber;  // can't overflow, it'll only go to 254 (127 + 127).
				if (note <= 127)
					sendPolyPressure(note, itemValue, channel);
				}
			}
        }
    }
        

#endif // defined(__MEGA__)

