////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License





#include "All.h"


/// SETUP
/// Top-level Arduino function, does initial set up of everything


void setPinsAndMasks(uint8_t pin, uint8_t mode, uint8_t &mask)
    {
    pinMode(pin, mode);
    mask = digitalPinToBitMask(pin);
    // For some reason, we can't set the port, due to fragileness in Arduino's portOutputRegister macro.  :-(
    }

#ifdef HEADLESS_RESET

void setup()
    {
    fullReset();

    // Show the welcome message
    write8x5Glyph(led2, GLYPH_8x5_GIZMO_PT1);
    write8x5Glyph(led, GLYPH_8x5_GIZMO_PT2);
    rotateMatrix(led2, DIR_180);
    rotateMatrix(led, DIR_180);
    sendMatrix(led, led2);
    delay(2000);
	
    write3x5Glyphs(GLYPH_SYSTEM_RESET);
    rotateMatrix(led2, DIR_180);
    rotateMatrix(led, DIR_180);
    sendMatrix(led, led2);
    }

void loop()
    {
    // do nothing
    }
	
#else

void setup()
    {
#ifndef HEADLESS
    // set up the pin mode, masks, and ports for the buttons and LEDs on the board
	
    setPinsAndMasks(PIN_LED_RED, OUTPUT, LED_RED_mask);
    setPinsAndMasks(PIN_LED_GREEN, OUTPUT, LED_GREEN_mask);
    setPinsAndMasks(PIN_BACK_BUTTON, INPUT_PULLUP, BACK_BUTTON_mask);
    setPinsAndMasks(PIN_MIDDLE_BUTTON, INPUT_PULLUP, MIDDLE_BUTTON_mask);
    setPinsAndMasks(PIN_SELECT_BUTTON, INPUT_PULLUP, SELECT_BUTTON_mask);
	
    // For some reason, we can't set the port in setPinsAndMasks, due to fragileness in Arduino's portOutputRegister macro.  :-(
    port_LED_RED = portOutputRegister(digitalPinToPort(PIN_LED_RED));
    port_LED_GREEN = portOutputRegister(digitalPinToPort(PIN_LED_GREEN));
    port_BACK_BUTTON = portInputRegister(digitalPinToPort(PIN_BACK_BUTTON));
    port_MIDDLE_BUTTON = portInputRegister(digitalPinToPort(PIN_MIDDLE_BUTTON));
    port_SELECT_BUTTON = portInputRegister(digitalPinToPort(PIN_SELECT_BUTTON));

    // All three buttons pressed on startup?  Do a full reset after 5 seconds
    if (digitalRead(PIN_BACK_BUTTON) == 0 && digitalRead(PIN_SELECT_BUTTON) == 0)
		{
    uint8_t semi = digitalRead(PIN_MIDDLE_BUTTON);    // if 0 (button pressed), then not semi
      
    	// Turn the board LEDs on
		digitalWrite(PIN_LED_RED, 0);
		digitalWrite(PIN_LED_GREEN, 0);
		delay(5000);
		
		if (semi)
			{
        semiReset();
			}
		else
			{
	      fullReset();
  		}
		}

    // Turn the board LEDs off
    digitalWrite(PIN_LED_RED, 1);
    digitalWrite(PIN_LED_GREEN, 1);
#endif	// HEADLESS

	// Define a few PSTRs that will be used several times
    nrpn_p = PSTR("NRPN");
    rpn_p = PSTR("RPN");
    cc_p = PSTR("CC");
    v_p = PSTR("IS");
    options_p = PSTR("OPTIONS");

	// seed the random number generator.  I need something which has
	// a moderate amount of entropy but is tiny.  analogRead(A0) is actually
	// pretty crummy.  There are lots of high-entropy libraries but they're 
	// big code.  Here's my simple approach
	randomSeed(analogRead(A0) ^ (analogRead(A2) << 8) ^ (analogRead(A4) << 16) ^ (analogRead(A5) << 24));

    // prepare the pots
    setupPots();
    
    // Set up the CV Gates.  For now we just do digitalWrite
    pinMode(VOLTAGE_GATE, OUTPUT);
    digitalWrite(VOLTAGE_GATE, 0);
	
    // Set up the LED display	
    initLED();
    setBlinkOnOff(1, 4);		// as fast as we can go.  This is good because we're only redrawing every 32 ticks (about 100 times a second).  It's also faster than 999 BPM (our maximum).
	
    // load the options from flash
    loadOptions();

    // Show the welcome message
    write8x5Glyph(led2, GLYPH_8x5_GIZMO_PT1);
    write8x5Glyph(led, GLYPH_8x5_GIZMO_PT2);
    //rotateMatrix(led2, DIR_180);
    //rotateMatrix(led, DIR_180);
    sendMatrix(led, led2);
    delay(2000);
	
    // Reset the menu delay
#ifdef INCLUDE_OPTIONS_MENU_DELAY
    setMenuDelay(options.menuDelay);
#else
    setScrollDelays(DEFAULT_MENU_DELAY, DEFAULT_SHORT_DELAY);
#endif
    
    /// Set up MIDI
    MIDI.setHandleClock(handleClock);
    MIDI.setHandleStart(handleStart);
    MIDI.setHandleStop(handleStop);
    MIDI.setHandleContinue(handleContinue);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleAfterTouchPoly(handleAfterTouchPoly);
    MIDI.setHandleControlChange(handleGeneralControlChange);
    MIDI.setHandleProgramChange(handleProgramChange);
    MIDI.setHandleAfterTouchChannel(handleAfterTouchChannel);
    MIDI.setHandlePitchBend(handlePitchBend);
    MIDI.setHandleSystemExclusive(handleSystemExclusive);
    MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
    MIDI.setHandleSongPosition(handleSongPosition);
    MIDI.setHandleSongSelect(handleSongSelect);
    MIDI.setHandleTuneRequest(handleTuneRequest);
    MIDI.setHandleActiveSensing(handleActiveSensing);
    MIDI.setHandleSystemReset(handleSystemReset);
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();

    defaultState = STATE_NONE;

	// start the DAC (actually this does nothing for now
#ifdef INCLUDE_VOLTAGE
	initDAC();
#endif

    // start clock
    startClock(true);

    // reset ticks and pulses
    uint32_t m = micros();

    // start NOW
    targetNextPulseTime = m;
    targetNextTickTime = m;
    }



/// LOOP
// Just updates the ticks and pauses, then calls go()
// Note sure how stable our ticks are.  We may need to move to an interrupt model.

void loop() 
    {
    updateTicksAndWait();
    go();
    }
  
#endif // HEADLESS_RESET




