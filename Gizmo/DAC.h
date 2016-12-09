////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __DAC_H_
#define __DAC_H_

#include <Wire.h>


// Writing to the DACs is quite slow; you may have very slight MIDI lag issues.

// I2C address of DAC A
#define DAC_A   0x62

// I2C address of DAC B
#define DAC_B   0x63

// Sets the value of a DAC.  Legal DAC values range from 0...4095
// Values outside this range will silently fail.
void setValue(uint8_t dacI2C, uint16_t value);

// Sets the value of a DAC to a value corresponding to a potentiometer ranging from 0...1023
// Values outside this range will silently fail.
void setPot(uint8_t dacI2C, uint16_t value);

// Sets the value of a DAC to a value corresponding to a note.  Typically octaves are 1V each.
// Our DAC is 5B, meaning that we may have a range of 60 notes.  We assume that middle C 
// (MIDI note 60) is value 24 in this range.  Thus our legal range is from 36...95.
// Values outside of this range will silently fail.
void setNote(uint8_t dacI2C, uint8_t note);

// Called in setup() to set up the DAC
void initDAC();
        
#endif

