////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#ifndef __TIMING_H__
#define __TIMING_H__

#include <Arduino.h>

// TIMING
//
// We have several measures of timing:
// 
// 0. The CURRENT TIME in microseconds
//
// 1. A TICK: 1/3125 seconds.  This is the rate of MIDI transmission.
//
// 2. A PULSE: 24 pulses per beat.  Either the user specifies the beats per minute -- which
//    ultimately gets translated into microseconds per pulse -- or the pulses arrive via the 
//    MIDI clock (which is also 24 pulses per beat).
//
// 3. A BEAT: 1 beat every 24 pulses.  This is measured with the BEAT COUNTDOWN.  When we reach
//    0, we automatically reset to 24.
//
// 4. A NOTE PULSE: X pulses, depending on the kind of note speed the user has chosen relative
//    to the TEMPO (in Beats Per Minute).  For example, if he has chosen to play arpeggios at 16th 
//    note time, since a 16th note is 1/4 of a quarter note (which equals 24 PULSES), we have 
//    24 / 4 = 6 PULSES per NOTE PULSE.
//
// At the top level, our loop tries to call go() once every 1/3125 seconds, the maximum rate of MIDI
// transmission.  We call each such iteration a TICK
//
// We also need to generate a PULSE for our internal clock (if we don't use a provided MIDI clock).
// To do this we first store a TEMPO in BEATS PER MINUTE.  The pulse rate is exactly 24 times this 
// (to be compatible with the MIDI clock).  We then need to compute our MICROSECONDS PER PULSE based
// on the TEMPO.
//
// To keep the beat, we maintain a TARGET NEXT PULSE TIME, and each time we exceed this timestep
// we increment it again by MICROSECONDS PER PULSE, then set the PULSE variable to 1 so people know
// that it's time to pulse.  We might also generate a MIDI clock signal when PULSE happens,
// depending on settings.
//
// If we're running off an external MIDI clock, this stuff is all ignored: instead PULSE is set whenever
// the MIDI Clock signal arrives.
//
// When a MIDI STOP command arrives, and we're using an external MIDI clock, then we will stop
// updating in response to incoming MIDI CLOCK commands until a MIDI START or MIDI CONTINUE
// show up.

// POSSIBLE BUG: ticks roll over after about 70 minutes.  Perhaps this rollover
// could result in us dropping a pulse; if so, we'd be off by 1/24 compared to other instruments.
// Dunno if this could actually happen but it seems a possibility. 




//// TIME

/// The current TIME in microseconds.  It's updated every tick.
extern uint32_t currentTime;

//// COMPARATORS FOR TIMESTAMPS
//// These comparators work even when you have rollover, assuming the difference is less than MID_TIME, which is enormous
//// The purpose of these comparators is to allow you to compute > and >= despite rollover effects, since the
//// rollover for the Arduino clock is about every 71 minutes.
////
//// You can use these comparators for tickCount or pulseCount as well, and I'm doing so in Auto Return Time,
//// though it's a bit silly to do so since the rollover for tickCount (and the minimum rollover for pulseCount) is 15 days 

#define MIN_TIME (0)
#define MAX_TIME (4294967295)
#define MID_TIME (2147483647)
#define TIME_GREATER_THAN(x, y) ( (x) - (y) < MID_TIME)
#define TIME_GREATER_THAN_OR_EQUAL(x, y) (!TIME_GREATER_THAN(y, x))



//// TICKS

// Microseconds per tick to maintain MIDI transmission speed.
// 1/3125 sec is exactly 320 microsec
#define TARGET_TICK_TIMESTEP 320

/// Estimated time of the next TICK (in microseconds)
extern uint32_t targetNextTickTime; 

/// The number of TICKS so far
extern uint32_t tickCount;

///// UPDATE TICKS AND WAIT
///// Called only by loop().  This does a delay the proper amount of time
///// in order sleep to the next tick, then updates the tick.
void updateTicksAndWait();





//// PULSES

// Estimated time of the next PULSE (in microseconds)
extern  uint32_t targetNextPulseTime;

// Number of microseconds between PULSES.  20833 is a tempo of 120 BPM (our default)
uint32_t getMicrosecsPerPulse();

// The number of PULSES so far.
extern  uint32_t pulseCount;

// Is a PULSE presently underway this TICK?
extern  uint8_t pulse;

///// How fast is our tempo?
#define MAXIMUM_BPM 999

///// SET PULSE RATE
///// Given a tempo in Beats Per Minute, sets the global variables such that the system issues
///// a PULSE at that rate.
void setPulseRate(uint16_t bpm);





//// NOTE PULSES

// The number of PULSES per NOTE PULSE.  By default this is 6, which is sixteenth notes.
//     But that's just a placeholder: this value will be updated immediately by loading
//     the options.noteSpeedType value and passing it to setNotePulseRate(...) (which
//     by default also is sixteenth notes).

extern uint8_t notePulseRate;

// The number of PULSES left before the next NOTE PULSE.   This value is updated to
//     the notePulseRate when it reaches zero.
extern uint8_t notePulseCountdown;

// Is a NOTE PULSE presently underway this TICK?
extern uint8_t notePulse;

// Are we potentially swinging this note?
extern uint8_t swingToggle;

// how much time should we delay?
extern uint32_t swingTime;

///// SET RAW NOTE PULSE RATE
///// Given a note speed type (various NOTE_SPEED_* values defined in LEDDisplay.h), sets up
///// the global variables such that the system issues a NOTE PULSE at that rate.
void setRawNotePulseRate(uint8_t rate, uint8_t sync);

///// SET NOTE PULSE RATE
///// Given a note speed type (various NOTE_SPEED_* values defined in LEDDisplay.h), sets up
///// the global variables such that the system issues a NOTE PULSE at that rate.
void setNotePulseRate(uint8_t _noteSpeedType);

///// GET NOTE PULSE RATE FOR
///// Returns what the note pulse rate would be for the given speed type.
uint8_t getNotePulseRateFor(uint8_t noteSpeedType);


//// BEATS

// MIDI clock standard defines 24 pulses per beat (quarter note).  We'll stick to that.
#define PULSES_PER_BEAT 24

// The number of PULSES left before the next BEAT (quarter note).  This value is updated to
//      PULSES_PER_BEAT (24, dictated by MIDI spec) when it reaches zero.  
extern  uint8_t beatCountdown;

// Is a BEAT presently underway this TICK?
extern uint8_t beat;





//// THE CLOCK

//// These functions are called to start, stop, and continue the clock, and
//// also to pulse it.  FROMBUTTON means whether this method is being called
//// from an application or a user pressing a button, and can be ignored when
//// appropriate, as opposed to the method being called internally or via MIDI
//// clock (if we're listening to MIDI clock), which should not be ignored.

uint8_t pulseClock(uint8_t fromButton);	// note that fromButton is ignored.  It's just here so pulseClock looks the same as the others so we can call it as a function pointer
uint8_t stopClock(uint8_t fromButton);
uint8_t startClock(uint8_t fromButton);
uint8_t continueClock(uint8_t fromButton);

// This is a private-ish function but needs to be used in two different
// modules hence its definition here.
// Sends a start, stop, continue, or clock pulse signal out MIDI
// ONLY IF bypass is OFF and one of the following things is true:
// 1. IGNORE_MIDI_CLOCK or USE_MIDI_CLOCK and !fromButton
// 2. GENERATE_MIDI_CLOCK
// See Timing.cpp implementation for more details and reasoning
// behind this function.
void sendClock(midi::MidiType signal, uint8_t fromButton);


#define CLOCK_STOPPED   0       
#define CLOCK_RUNNING   1

//// Returns the current clock state, one of CLOCK_STOPPED and CLOCK_RUNNING
uint8_t getClockState();

void initializeClock();

void sendDividedClock();
void resetDividedClock();


//// Called by go() every iteration to update pulse, note pulse, and beat variables and trigger
//// stuff.  Does so considering swing and note division.
void updateTimers();

#endif

