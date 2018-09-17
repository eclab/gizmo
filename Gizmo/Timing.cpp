////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

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

// The number of PULSES left before we emit a divided MIDI CLOCK if we're doing CLOCK DIVIDE.
//     This value is updated to options.clockDivide when it reaches zero.
GLOBAL uint8_t dividePulseCountdown = 1;

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

// Returns whether we are using a clock command which allows us to emit a clock message
// in response to a button press, internal pulse, or 
uint8_t shouldEmitClockMessages()
    {
    return      !bypass &&
        (options.clock == GENERATE_MIDI_CLOCK ||
        options.clock == USE_MIDI_CLOCK);
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
    if (lastExternalPulseTime > 0)      // we've seen one pulse already
        {
        // note that we're overwriting the microsecsPerPulse variable.  This will get
        // reset when the user changes the clock setting back to something that's not
        // external (see the case for STATE_OPTIONS_MIDI_CLOCK in TopLevel.cpp)
        externalMicrosecsPerPulse = currentTime - lastExternalPulseTime;
        }
    lastExternalPulseTime = currentTime;
    }



/// This function is called in a variety of contexts.
/// 
///     1. Directly from handleClockCommand, if starting/stopping/continuing/pulsing,
///    or if ignoring the clock.  In these cases we want to send it out unless bypass
///    is on.
/// 2. From pulseClock or from sendDivided clock.  Here we want to send it out unless bypass
///    is on.
/// 3. From startClock, stopClock, or continueClock.  Here the clock could be started due to
///    the user manually pressing the button or due to some application; alternatively it
///    could be as result of external control.  If we're USING an external clock, we want
///    to send it on if it's not a button (because we're on external control).  If  we're
///    GENERATING a clock, we want to send it always.  If
///    we're BLOCKING, or CONSUMING the clock, we don't want to send it on.  If we're IGNORING
///    the clock, we may get called here by handleClockComand or by handleClockCommand->sendDividedClock,
///        where they're hoping we'll pass it through (so we do so).

void sendClock(midi::MidiType signal, uint8_t fromButton)
    {
    if (!bypass &&                                                                                                      // don't send if I'm bypassed
            (       (options.clock == IGNORE_MIDI_CLOCK && !fromButton) ||  // allow a send if I'm IGNORING -- ignore calls this to pass it through
            (options.clock == USE_MIDI_CLOCK && !fromButton) ||             // allow a send if I'm USING (and passing through) but NOT if the button was pressed
            (options.clock == GENERATE_MIDI_CLOCK)))        // allow a send if I'm GENERATING AND a button was pressed
        {
        MIDI.sendRealTime(signal);
        TOGGLE_OUT_LED();
        }
    }



#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
void sendDividedClock()
    {
    if (--dividePulseCountdown == 0)
        {
        dividePulseCountdown = options.clockDivisor;
        sendClock(MIDIClock, false);
        }
    }
        
void resetDividedClock()
    {
    dividePulseCountdown = 1;
    }
#endif

// This method is called whenever we get an internal or external pulse.
// The fromButton parameter is ALWAYS false.
//
// If we have an external pulse, we want to update to reflect this.
// If we're doing USE or GENERATE, pass it through (on the UNO).
// On the mega, we'll do this via division in updateTimers()
//
// Returns 1 if successful

uint8_t pulseClock(uint8_t fromButton)
    {
    if (clockState == CLOCK_STOPPED)
        return 0;

    // update our external clock pulse estimate
    if (USING_EXTERNAL_CLOCK())
        updateExternalClock(); 

#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
    // do nothing
#else
    sendClock(MIDIClock, fromButton);  // fromButton should always be FALSE
#endif
                
    pulse = 1;
    pulseCount++;
    
    return 1;
    }



// This method is only called when a user presses a BUTTON
// or when an external MIDI comes in and we're doing USE or CONSUME.
//
// If we're doing USE or CONSUME and a button is pressed, ignore it entirely.
// Otherwise, respond to it.
//
// If we're doing USE and it's not a button, *or* if we're doing
// GENERATE and it's a button, emit it.

uint8_t stopClock(uint8_t fromButton)
    {
    if (fromButton && (USING_EXTERNAL_CLOCK() || clockState == CLOCK_STOPPED))
        return 0;
        
    sendClock(MIDIStop, fromButton);

    lastExternalPulseTime = 0;
    externalMicrosecsPerPulse = 0;
    clockState = CLOCK_STOPPED;

#ifdef INCLUDE_ARPEGGIATOR
    if (application == STATE_ARPEGGIATOR)
        {
        MIDI.sendControlChange(123, 0, options.channelOut);
        }
    else
#endif

#ifdef INCLUDE_RECORDER
        if (application == STATE_RECORDER)
            {
            MIDI.sendControlChange(123, 0, options.channelOut);
            }
#endif

#ifdef INCLUDE_STEP_SEQUENCER
    if (application == STATE_STEP_SEQUENCER)
        {
        sendAllSoundsOff();
        }
#endif
        
    return 1;
    }

extern uint8_t drawBeatToggle;
extern uint8_t drawNotePulseToggle;


// This method is only called when a user presses a BUTTON
// or when an external MIDI comes in and we're dong USE or CONSUME.
//
// If we're dong USE or CONSUME and a button is pressed, ignore it entirely.
// Otherwise, respond to it.
//
// If we're doing USE and it's not a button, *or* if we're doing
// GENERATE and it's a button, emit it.

uint8_t startClock(uint8_t fromButton)
    {
    if (fromButton && (USING_EXTERNAL_CLOCK() || clockState != CLOCK_STOPPED))
        return 0;
        
    sendClock(MIDIStart, fromButton);

    dividePulseCountdown = 1;
    notePulseCountdown = 1;
    beatCountdown = 1;
    drawBeatToggle = 0;
    drawNotePulseToggle = 0;
    pulseCount = 0;
    clockState = CLOCK_RUNNING;
    swingToggle = 0;

    // When we start the clock we want to have applications starting at their initial points.
    // NOTE: It may be that instead actually want to START them playing.  But I don't think so.
    // I think the user should "arm" the applications by starting them manually but in the
    // situation where the system clock is currently stopped.  Then he can control them via
    // the system clock like this. 
    switch(application)
        {
#ifdef INCLUDE_STEP_SEQUENCER
        case STATE_STEP_SEQUENCER:
            {
            // reset the step sequencer
            resetStepSequencer();
            }
        break;
#endif
#ifdef INCLUDE_RECORDER
        case STATE_RECORDER:
            {
            // reset the recorder
            resetRecorder();
            }
        break;
#endif

        }
    return 1;
    }


// This method is only called when a user presses a BUTTON
// or when an external MIDI comes in and we're dong USE or CONSUME.
//
// If we're dong USE or CONSUME and a button is pressed, ignore it entirely.
// Otherwise, respond to it.
//
// If we're doing USE and it's not a button, *or* if we're doing
// GENERATE and it's a button, emit it.

uint8_t continueClock(uint8_t fromButton)
    {
    if (fromButton && (USING_EXTERNAL_CLOCK() || clockState != CLOCK_STOPPED))
        return 0;

    if (clockState != CLOCK_STOPPED)
        return 0;

    sendClock(MIDIContinue, fromButton);
        
    clockState = CLOCK_RUNNING;

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
GLOBAL static uint8_t notePulseRateTable[16] = { 1, 2, 3, 4, 6, 8, 12, 24, 32, 36, 48, 64, 72, 96, 144, 192 };

///// SET NOTE PULSE RATE
///// Given a note speed type (various NOTE_SPEED_* values defined in LEDDisplay.h), sets up
///// the global variables such that the system issues a NOTE PULSE at that rate.
void setNotePulseRate(uint8_t noteSpeedType)
    {
    uint8_t oldNotePulseRate = notePulseRate;
    notePulseRate = notePulseRateTable[noteSpeedType];
    if (oldNotePulseRate != notePulseRate)  // no need to create a new countdown, and the blip with it
    	{
	    notePulseCountdown = ((uint8_t)(pulseCount % notePulseRate)) + 1;
	    drawNotePulseToggle = (uint8_t)((pulseCount / notePulseRate) & 1);          // that is, %2
	    }
    }

uint32_t getMicrosecsPerPulse()
    {
    if (externalMicrosecsPerPulse)
        return externalMicrosecsPerPulse;
    else return microsecsPerPulse;
    }



void updateTimers()
    {
    // update our internal clock if we're making one
    if (!USING_EXTERNAL_CLOCK())
        {
        if (currentTime > targetNextPulseTime)
            {
            targetNextPulseTime += microsecsPerPulse;
            pulseClock(false);  // note that the 'false' is ignored
            }
        }

    if (swingTime != 0 && currentTime >= swingTime)
        {
        // play!
        notePulse = 1;
        swingTime = 0;
        }       
    
    if (pulse)
        {
        if (--notePulseCountdown == 0)
            {
            // we may start to swing.  Figure extra swing hold time
            if (swingToggle && options.swing > 0)
                {
                swingTime = currentTime + div100(notePulseRate * getMicrosecsPerPulse() * options.swing);
                }
            else
                {
                // redundant with above, but if I move this to a function, the code bytes go up
                notePulse = 1;
                swingTime = 0;
                }                       

            notePulseCountdown = notePulseRate;
                        
            if (options.noteSpeedType == NOTE_SPEED_THIRTY_SECOND || 
                options.noteSpeedType == NOTE_SPEED_SIXTEENTH ||
                options.noteSpeedType == NOTE_SPEED_EIGHTH ||
                options.noteSpeedType == NOTE_SPEED_QUARTER ||
                options.noteSpeedType == NOTE_SPEED_HALF)
                swingToggle = !swingToggle;
            else
                swingToggle = 0;
            }
                
        if (--beatCountdown == 0)
            {
            beat = 1;
            beatCountdown = PULSES_PER_BEAT;
            }
            
#ifdef INCLUDE_OPTIONS_MIDI_CLOCK_DIVIDE
        if (options.clock == USE_MIDI_CLOCK ||
            options.clock == GENERATE_MIDI_CLOCK)
            sendDividedClock();
#endif
        }
    }
