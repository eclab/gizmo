////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

////// ALL.H
//////
////// This is just distributing #include file.  Everyone #includes All.h


// You'll see GLOBAL here and there.  It means nothing -- but it makes it easy
// for me to grep for global variables.
#define GLOBAL

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

// This lets everyone have access to the MIDI global, not just
// the .ino file
extern midi::MidiInterface<HardwareSerial> MIDI; 

