#include "All.h"

#if defined(__AVR_ATmega2560__)

void stateSplit()
	{
	  entry = false;

	  // despite the select button release ignoring, we occasionally get button
	  // bounces on the select button so I'm moving the main stuff to the middle button
	  // and only having long releases on the select button
		
	  if (isUpdated(SELECT_BUTTON, RELEASED))
		  {
		  goDownState(STATE_SPLIT_NOTE);
		  }
	  else if (isUpdated(MIDDLE_BUTTON, RELEASED))
		  {
		  goDownState(STATE_SPLIT_LAYER_NOTE);
		  }
	  else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
		  {
		  goDownState(STATE_SPLIT_CHANNEL);
		  }
	  else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
		  {
		  options.splitControls += options.splitControls;
		  if (options.splitControls > SPLIT_MIX)
			options.splitControls = SPLIT_CONTROLS_RIGHT;
		  saveOptions();
		  }
	  else if (isUpdated(BACK_BUTTON, RELEASED))
		  {
		  goUpState(STATE_ROOT);
		  }
	  else if (updateDisplay)
		  {
		  clearScreen();
		  
		  if (options.splitControls == SPLIT_MIX)
			{
			write3x5Glyphs(GLYPH_FADE);
			}
		  else
			{
			writeNotePitch(led2, options.splitNote);
			if (options.splitLayerNote != NO_NOTE)
				{
				writeNotePitch(led, options.splitLayerNote);
				}
			if (options.splitControls == SPLIT_CONTROLS_RIGHT)
				{
				setPoint(led2, 7, 0);
				}
			else
				{
				setPoint(led2, 0, 0);
				}
			}
		  }
	}
	
void stateSplitNote()
	{
	  if (stateEnterNote(GLYPH_NOTE, STATE_SPLIT) != NO_NOTE)
		  {
		  options.splitNote = itemNumber;
		  saveOptions();
		  goUpState(STATE_SPLIT);
		  }
	}
	
void stateSplitLayerNote()
	{
	  if (options.splitLayerNote != NO_NOTE)
		{
		clearScreen();
		write3x5Glyphs(GLYPH_NONE);
		if (isUpdated(SELECT_BUTTON, PRESSED))
			{
			isUpdated(SELECT_BUTTON, RELEASED);  // clear this just in case
			options.splitLayerNote = NO_NOTE;
			saveOptions();
			goUpState(STATE_SPLIT);
			}
		else if (isUpdated(BACK_BUTTON, RELEASED))
			{
			goUpState(STATE_SPLIT);
			}
		}
	  else 
		{
		  if (stateEnterNote(GLYPH_NOTE, STATE_SPLIT) != NO_NOTE)
			  {
			  options.splitLayerNote = itemNumber;
			  saveOptions();
			  goUpState(STATE_SPLIT);
			  }
		}
	}
	
#endif // defined(__AVR_ATmega2560__)