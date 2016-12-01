#include "All.h"

//// TIME

/// The current TIME in microseconds.  It's updated every tick.
GLOBAL uint32_t currentTime;




//// TICKS

/// Estimated time of the next TICK (in microseconds)
GLOBAL uint32_t targetNextTickTime = 0;

/// The number of TICKS so far
GLOBAL uint32_t tickCount = 0;




//// PULSES

// Estimated time of the next PULSE (in microseconds)
GLOBAL uint32_t targetNextPulseTime = 0;

// Number of microseconds between PULSES.  20833 microseconds per pulse is  is a tempo of 120 BPM (our default)
// Specifically: 120 BEAT/MIN * 1 MIN / 60 SEC * 1 SEC / 1000000 MSEC * 24 PULSE/BEAT = 3/62500 PULSE/MSEC
// Flipping that we have 62500 MSEC / 3 PULSE ~ 20833 MSEC/PULSE
GLOBAL uint32_t microsecsPerPulse = 20833;

// The number of PULSES so far.
GLOBAL uint32_t pulseCount = 0;

// Is a PULSE presently underway this TICK?
GLOBAL uint8_t pulse;

// Are we potentially swinging this note?
GLOBAL uint8_t swingToggle = 0;

// how much time should we delay?
GLOBAL uint32_t swingTime;




//// NOTE PULSES

// The number of PULSES per NOTE PULSE.  By default this is 6, which is sixteenth notes.
//     But that's just a placeholder: this value will be updated immediately by loading
//     the options.noteSpeedType value and passing it to setNotePulseRate(...) (which
//     by default also is sixteenth notes).
GLOBAL uint8_t notePulseRate = 6;

// The number of PULSES left before the next NOTE PULSE.   This value is updated to
//     the notePulseRate when it reaches zero.
GLOBAL uint8_t notePulseCountdown = 1;

// Is a NOTE PULSE presently underway this TICK?
GLOBAL uint8_t notePulse;



//// BEATS

// The number of PULSES left before the next BEAT (quarter note).  This value is updated to
//      PULSES_PER_BEAT (24, dictated by MIDI spec) when it reaches zero.  
GLOBAL uint8_t beatCountdown = 1;

// Is a BEAT presently underway this TICK?
GLOBAL uint8_t beat;




///// UPDATE TICKS AND WAIT
///// Called only by loop().  This does a delay the proper amount of time
///// in order sleep to the next tick, then updates the tick.
void updateTicksAndWait()
    {
    targetNextTickTime += TARGET_TICK_TIMESTEP;
    currentTime = micros();
    if (currentTime > targetNextTickTime)
        {
        // uh oh, this will happen once per hour or so when micros rolls over. 
        // otherwise it implies that we're not processing fast enough so we might
        // want to debug here.
        
        // Makes things really slow
        //delayMicroseconds(targetNextTickTime - currentTime);  // this should be right, as we wrap around
        }
    else
        {
        // sleep until the target time
        delayMicroseconds(targetNextTickTime - currentTime);
        }
    
    // this better be accurate...
    ++tickCount;
    }

// Returns whether we are using a clock command that either PASSES THROUGH or EMITS clock messages, 
// other than IGNORE_MIDI_CLOCK, which has already been handled specially at this point (in
// handleClockCommand())
uint8_t shouldEmitClockMessages()
	{
	return (options.clock == USE_MIDI_CLOCK ||
		   options.clock == GENERATE_MIDI_CLOCK
#if defined(__AVR_ATmega2560__)
	|| options.clock == DIVIDE_MIDI_CLOCK
#endif // defined(__AVR_ATmega2560__)
			)
		   && !bypass;
	}


uint32_t lastExternalPulseTime = 0;
uint32_t externalMicrosecsPerPulse = 0;
uint8_t clockState = CLOCK_STOPPED;

/// Estimates the microseconds per pulse if we're being driven by a remote external clock.
/// If we have just STARTed, then lastExternalPulseTime is set to 0, so this is our FIRST pulse.
/// We need at two pulses to get an estimate.  Prior to the second pulse, our estimate is 0.
/// At the first pulse (and every pulse thereafter), we record the time of the pulse in 
/// lastExternalPulseTime. At the second pulse (and every pulse thereafter), the estimate
//  is (of course) the difference between the current time and the last pulse.
//
//  The issue here is that when a note is played on the VERY FIRST PULSE, application such
//  as the step sequencer need to know when to turn the note off.  If there's swing, then they
//  have to use the clock microseconds estimate to figure this out.  But we don't have it yet.
//  So we'll probably use the previous (non-external) microseconds estimate.  This is yucky
//  and could make bad sounds until we're into the second note.  However it's probably not
//  an issue because swing doesn't happen until the SECOND note typically, and that's going to
//  be well after the first pulse, at which point the estimate will be realistic.  I HOPE!
void updateExternalClock()
	{
    if (lastExternalPulseTime > 0)	// we've seen one pulse already
    	{
    	// note that we're overwriting the microsecsPerPulse variable.  This will get
    	// reset when the user changes the clock setting back to something that's not
    	// external (see the case for STATE_OPTIONS_MIDI_CLOCK in TopLevel.cpp)
    	externalMicrosecsPerPulse = currentTime - lastExternalPulseTime;
    	}
     lastExternalPulseTime = currentTime;
	}

void pulseClock()
    {
    if (clockState == CLOCK_STOPPED)
        return;

	// update our external clock pulse estimate
	if (USING_EXTERNAL_CLOCK())  // we're using an external clock
		updateExternalClock(); 

    pulse = 1;
    pulseCount++;

#if defined(__AVR_ATmega2560__)
    if (options.clock != DIVIDE_MIDI_CLOCK && shouldEmitClockMessages())
#else
    if (shouldEmitClockMessages())
#endif
        { MIDI.sendRealTime(MIDIClock); TOGGLE_OUT_LED(); }
    }

uint8_t stopClock(uint8_t fromButton = false)
    {
    if (fromButton && USING_EXTERNAL_CLOCK())
        return 0;
        
    if (clockState != CLOCK_RUNNING)
    	return 0;

	lastExternalPulseTime = 0;
	externalMicrosecsPerPulse = 0;
	
    clockState = CLOCK_STOPPED;

    if (shouldEmitClockMessages())
        { MIDI.sendRealTime(MIDIStop); TOGGLE_OUT_LED(); }

    // if we stop the clock some notes may be playing.  We reset them here.
    sendAllNotesOff();

    return 1;
    }

extern uint8_t drawBeatToggle;
extern uint8_t drawNotePulseToggle;

uint8_t startClock(uint8_t fromButton = false)
    {
    if (fromButton && USING_EXTERNAL_CLOCK())
        return 0;
        
	if (clockState != CLOCK_STOPPED)
		return 0;

    notePulseCountdown = 1;
    beatCountdown = 1;
    drawBeatToggle = 0;
    drawNotePulseToggle = 0;
    pulseCount = 0;
    clockState = CLOCK_RUNNING;
    swingToggle = 0;

    if (shouldEmitClockMessages())
        { MIDI.sendRealTime(MIDIStart); }

    // When we start the clock we want to have applications starting at their initial points.
    // NOTE: It may be that instead actually want to START them playing.  But I don't think so.
    // I think the user should "arm" the applications by starting them manually but in the
    // situation where the system clock is currently stopped.  Then he can control them via
    // the system clock like this. 
    if (application == STATE_STEP_SEQUENCER)
    	{
    	// reset the step sequencer
    	resetStepSequencer();
		}    	
    else if (application == STATE_RECORDER)
    	{
    	// reset the recorder
    	resetRecorder();
    	}

    return 1;
    }
        
uint8_t continueClock(uint8_t fromButton = false)
    {
    if (clockState != CLOCK_STOPPED)
    	return 0;
    	
    if (fromButton && USING_EXTERNAL_CLOCK())
        return 0;

    clockState = CLOCK_RUNNING;

    if (shouldEmitClockMessages())
        { MIDI.sendRealTime(MIDIContinue); TOGGLE_OUT_LED(); }
    return 1;
    }     
        
uint8_t getClockState()
    {
    return clockState;
    }   


///// SET PULSE RATE
///// Given a tempo in Beats Per Minute, sets the global variables such that the system issues
///// a PULSE at that rate.
void setPulseRate(uint16_t bpm)
    {
    long currentTime = micros();                // note local variable
    
    // BPM conversion to usec/pulse:
    // X Beat/Minute * 24 pulses/Beat / 60000000 usec/Minute = Y pulses/usec
    // Then flip
    // So you have usecs/pulse = 1 / (bpm * 24 / 60000000) = 2500000 / bpm
  
    // this division will be costly, but I don't see any way around it.
    microsecsPerPulse = (((uint32_t) 2500000) / bpm);
  
    // update the target pulse time, but don't starve if we're constantly changing the pulse rate
    targetNextPulseTime =  (targetNextPulseTime - currentTime > microsecsPerPulse ? currentTime + microsecsPerPulse : targetNextPulseTime);
    }

  


//// Table of note pulse rates corresponding to each note speed (such as NOTE_SPEED_QUARTER)
GLOBAL static uint8_t notePulseRateTable[16] = { 1, 2, 3, 4, 6, 8, 12, 24, 32, 36, 48, 60, 72, 96, 144, 198 };

///// SET NOTE PULSE RATE
///// Given a note speed type (various NOTE_SPEED_* values defined in LEDDisplay.h), sets up
///// the global variables such that the system issues a NOTE PULSE at that rate.
void setNotePulseRate(uint8_t noteSpeedType)
    {
    notePulseRate = notePulseRateTable[noteSpeedType];
    notePulseCountdown = 1;   // set it up so that if we're right on a note pulse border, we pulse next time.
    beatCountdown = 1;
    drawNotePulseToggle = 0;
    drawBeatToggle = 0;
    }

uint32_t getMicrosecsPerPulse()
	{
	if (externalMicrosecsPerPulse)
		return externalMicrosecsPerPulse;
	else return microsecsPerPulse;
	}



