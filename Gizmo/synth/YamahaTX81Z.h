////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __SYNTH_YAMAHA_TX81Z_H__
#define __SYNTH_YAMAHA_TX81Z_H__


#include "../TopLevel.h"
#include <Arduino.h>

#define YAMAHA_TX81Z_SYSEX_LENGTH (7)
struct _yamahaTX81ZLocal
	{
	uint16_t lastParameter;
	uint16_t lastValue;
	uint16_t countDown;
	char data[YAMAHA_TX81Z_SYSEX_LENGTH];
	uint8_t dataLength;
	};


#define YAMAHA_TX81Z_BAD_PARAMETER	(5000)
#define YAMAHA_TX81Z_COUNTDOWN		(32)

void stateSynthYamahaTX81Z();

#endif

