////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __DAC_H_
#define __DAC_H_

#include <Wire.h>
#include <Arduino.h>


// Writing to the DACs is quite slow; you may have very slight MIDI lag issues.

// I2C address of DAC A
#define DAC_A   0x62

// I2C address of DAC B
#define DAC_B   0x63

// Address of Digital Pin which goes LOW when Gate is opened (a note is played)
#define VOLTAGE_GATE  (23)
extern uint8_t VOLTAGE_GATE_mask;
extern volatile uint8_t *port_VOLTAGE_GATE;

// Sets the value of a DAC.  Legal DAC values range from 0...4095
// Values outside this range will silently fail.
void setValue(uint8_t dacI2C, uint16_t value);

// Called in setup() to set up the DAC
void initDAC();
        
#endif

