#include "All.h"

#if defined(__AVR_ATmega2560__)

void resetDistributionNotes() 
	{ 
	memset(local.thru.distributionNotes, NO_NOTE, 16);
	local.thru.currentDistributionChannelIndex = 0; 
	}


void stateThruPlay()
	{
	if (entry)
		{
		sendAllNotesOff();
		resetDistributionNotes();
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
		  sendAllNotesOff();
		  }

	else if (!bypass && newItem && (itemChannel == options.channelIn || options.channelIn == CHANNEL_OMNI) && options.channelOut != 0)
		{
		uint8_t channel = options.channelOut;
		
		if (itemType == MIDI_NOTE_ON)
			{
			// NOTE DISTRIBUTION OVER MULTIPLE CHANNELS
			if (options.thruNumDistributionChannels > 0)
				{
				// revise the channel
				channel = (options.channelOut + local.thru.currentDistributionChannelIndex - 1) % 16 + 1;
					
				// do I need to turn off a note?
				if (local.thru.distributionNotes[local.thru.currentDistributionChannelIndex] != NO_NOTE)
					{
					for(uint8_t i = 0; i <= options.thruExtraNotes; i++)		// do at least once
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
			for(uint8_t i = 0; i <= options.thruExtraNotes; i++)		// do at least once
				{
				sendNoteOn(itemNumber, itemValue, channel);
				}
			}
		else if (itemType == MIDI_NOTE_OFF)
			{
			// NOTE DISTRIBUTION OVER MULTIPLE CHANNELS
			if (options.thruNumDistributionChannels > 0)
				{
				uint8_t found = 0;
				
				// find the channel
				for(uint8_t i = 0; i < options.thruNumDistributionChannels; i++)
					{
					uint8_t ii = (local.thru.currentDistributionChannelIndex + i);
					if (ii >= options.thruNumDistributionChannels)
						ii = 0;
						
					if (local.thru.distributionNotes[ii] == itemNumber)
						{
						found = 1;
						channel = (options.channelOut + ii - 1) % 16 + 1;
						}
					}
					
				if (!found)  // uh oh, maybe turned off earlier?  Don't play off
					return;
				}

			// NOTE REPLICATION
			for(uint8_t i = 0; i <= options.thruExtraNotes; i++)		// do at least once
				{
				sendNoteOff(itemNumber, itemValue, channel);
				}
			}
		}
	}
	
#endif // defined(__AVR_ATmega2560__)

