////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License




////// ALL.H
//////
////// This is just distributing #include file.  Everyone #includes All.h


// You'll see GLOBAL here and there.  It means nothing -- but it makes it easy
// for me to grep for global variables.
#define GLOBAL


// If you don't want to use the SparkFun MIDI Shield, and perhaps use some other
// compatible MIDI shield without the buttons and pots, you can turn on these defines.
// Don't turn BOTH of them on at the same time!
//
// IF you turn on this define and then run the code on the Arduino, it will reset the options,
// display the splash screen, then display RSET and nothing else.  This makes it possible to do
// a reset without holding the three buttons down (because you have no buttons!) 
//#define HEADLESS_RESET
//
// If you turn on this define, Gizmo will eliminate the option of OFF for it's MIDI Control so you
// can't possibly lock yourself out by accident.  You can then control Gizmo entirely via NRPN
// without ever touching the knobs or buttons (which you might not have).
//#define HEADLESS

// If you want to do some development on the Arduino Uno (which has some space), you can remove
// the Note Recorder.  Here's a quick hack that will do the job.  You'll see "RECORDER" in the
// top level menu but it won't do anything and the Recorder code will have been removed, freeing up
// about 1.5K.  Not much but certainly enough to do something small.
// Don't do this on the Mega (plus its not particularly useful there)
//#define NO_RECORDER


#include <Arduino.h>
#include <EEPROM.h>
#include <MIDI.h>
#include <SoftReset.h>
#include "DAC.h"
#include "Utility.h"
#include "TopLevel.h"
#include "MidiShield.h"
#include "LEDDisplay.h"
#include "Division.h"
#include "Timing.h"
#include "Control.h"
#include "Storage.h"
#include "Arpeggiator.h"
#include "Recorder.h"
#include "Options.h"
#include "StepSequencer.h"
#include "Gauge.h"
#include "Split.h"
#include "Thru.h"


// This lets everyone have access to the MIDI global, not just
// the .ino file
extern midi::MidiInterface<HardwareSerial> MIDI; 

