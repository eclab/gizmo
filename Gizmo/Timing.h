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
//    ultimately gets translated microseconds per pulse -- or the pulses arrive via the 
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
// we increment it again by MICROSECS PER PULSE, then set the PULSE variable to 1 so people know
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





//// TICKS

// Microseconds per tick to maintain MIDI transmission speed.
// 1/3125 sec is exactly 320 microsec
#define TARGET_TICK_TIMESTEP 320

/// Estimated time of the next TICK (in microseconds)
extern uint32_t targetNextTickTime; 

/// The number of TICKS so far
extern uint32_t tickCount;




///// How fast is our tempo?
#define MAXIMUM_BPM 999



//// PULSES

// Estimated time of the next PULSE (in microseconds)
extern  uint32_t targetNextPulseTime;

// Number of microseconds between PULSES.  20833 is a tempo of 120 BPM (our default)
extern uint32_t microsecsPerPulse;  // 120 BPM by default
extern uint32_t externalMicrosecsPerPulse;

uint32_t getMicrosecsPerPulse();

// The number of PULSES so far.
extern  uint32_t pulseCount;

// Is a PULSE presently underway this TICK?
extern  uint8_t pulse;




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



//// BEATS

// MIDI clock standard defines 24 pulses per beat (quarter note).  We'll stick to that.
#define PULSES_PER_BEAT 24

// The number of PULSES left before the next BEAT (quarter note).  This value is updated to
//      PULSES_PER_BEAT (24, dictated by MIDI spec) when it reaches zero.  
extern  uint8_t beatCountdown;

// Is a BEAT presently underway this TICK?
extern uint8_t beat;




///// UPDATE TICKS AND WAIT
///// Called only by loop().  This does a delay the proper amount of time
///// in order sleep to the next tick, then updates the tick.
void updateTicksAndWait();

///// FIRE PULSE
///// This sets up global variables to alert everyone that a pulse has just arrived, either
///// internally or externally via a MIDI clock
void firePulse();

///// RESET PULSES
///// Resets the pulses to zero, and likewise resets note pulses and beats.  This occurs when
///// the MIDI clock issues a START or when we change our MIDI Clock options
void resetPulses();

///// SET PULSE RATE
///// Given a tempo in Beats Per Minute, sets the global variables such that the system issues
///// a PULSE at that rate.
void setPulseRate(uint16_t bpm);

///// SET NOTE PULSE RATE
///// Given a note speed type (various NOTE_SPEED_* values defined in LEDDisplay.h), sets up
///// the global variables such that the system issues a NOTE PULSE at that rate.
void setNotePulseRate(uint8_t _noteSpeedType);


void pulseClock();
uint8_t stopClock(uint8_t fromButton);
uint8_t startClock(uint8_t fromButton);
uint8_t continueClock(uint8_t fromButton);

#define CLOCK_STOPPED   0       
#define CLOCK_RUNNING   1

uint8_t getClockState();


#endif

