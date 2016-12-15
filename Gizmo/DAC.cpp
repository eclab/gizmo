////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#include <Wire.h>
#include "All.h"

// Sets the value of a DAC.  Legal DAC values range from 0...4095
// Values outside this range will silently fail.
void setValue(uint8_t dacI2C, uint16_t val)
    {
#if defined(__AVR_ATmega2560__)
    if (val >= 4096) return;
    if (options.voltage)
        {
        Wire.beginTransmission(dacI2C);
        Wire.write( 0x40 );  //write data
        Wire.write( val >> 4 ); // upper 8 bits of 12-bit data
        Wire.write(( val & 0x10 ) << 4 ); // lower 8 bits of 12 bit data shifted to top of byte
        Wire.endTransmission();
        }
#endif
    }

// Sets the value of a DAC to a value corresponding to a potentiometer ranging from 0...1023
// Values outside this range will silently fail.
void setPot(uint8_t dacI2C, uint16_t value)
    {
    if (value >= 1024) return;
    setValue(dacI2C, value * 4);
    }
        
// Sets the value of a DAC to a value corresponding to a note.  Typically octaves are 1V each.
// Our DAC is 5B, meaning that we may have a range of 60 notes.  We assume that middle C 
// (MIDI note 60) is value 24 in this range.  Thus our legal range is from 36...95.
// Values outside of this range will silently fail.
void setNote(uint8_t dacI2C, uint8_t note)
    {
    if (note < 36 || note > 95) return;
    setValue(dacI2C, (4096 * (uint16_t) note) / 60);
    }
        
// Called in setup() to set up the DAC
void initDAC()
    {
    // Make I2C go faster (by default it's 100Hz). 
    // Note: Display.cpp does this too
    //Wire.setClock(400000L);
    }

