////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __SYNTH_WALDORF_BLOFELD_H__
#define __SYNTH_WALDORF_BLOFELD_H__


#include "../TopLevel.h"
#include <Arduino.h>

#define WALDORF_BLOFELD_SYSEX_LENGTH (8)
struct _waldorfBlofeldLocal
	{
	uint8_t id;
	uint16_t lastParameter;
	uint16_t lastValue;
	uint16_t countDown;
	char data[WALDORF_BLOFELD_SYSEX_LENGTH];
	};


#define WALDORF_BLOFELD_HIGH_PARAMETER	(382)
#define WALDORF_BLOFELD_BAD_PARAMETER	(399)
#define WALDORF_BLOFELD_ID_PARAMETER	(400)
#define WALDORF_BLOFELD_COUNTDOWN		(32)

void stateSynthWaldorfBlofeld();

#endif

