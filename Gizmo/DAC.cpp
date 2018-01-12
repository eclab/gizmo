////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

 

#include <Wire.h>
#include "All.h"

#ifdef INCLUDE_VOLTAGE

GLOBAL uint8_t VOLTAGE_GATE_mask;
GLOBAL volatile uint8_t *port_VOLTAGE_GATE;


// Sets the value of a DAC.  Legal DAC values range from 0...4095
// Values outside this range will silently fail.
void setValue(uint8_t dacI2C, uint16_t val)
    {
    if (val >= 4096) return;
    if (options.voltage)
        {
        Wire.beginTransmission(dacI2C);
        Wire.write( 0x40 );  //write data
        Wire.write( val >> 4 ); // upper 8 bits of 12-bit data
        Wire.write(( val & 0x10 ) << 4 ); // lower 8 bits of 12 bit data shifted to top of byte
        Wire.endTransmission();
        }
    }
        
// Called in setup() to set up the DAC
void initDAC()
    {
    // Make I2C go faster (by default it's 100Hz). 
    // Note: Display.cpp does this too
    //Wire.setClock(400000L);

    // set up gate output    
    pinMode(VOLTAGE_GATE, OUTPUT);
    VOLTAGE_GATE_mask = digitalPinToBitMask(VOLTAGE_GATE);
    port_VOLTAGE_GATE = portOutputRegister(digitalPinToPort(VOLTAGE_GATE));
    digitalWrite(VOLTAGE_GATE, 0);
    }

#endif
