////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __SYNTH_YAMAHA_TX81Z_H__
#define __SYNTH_YAMAHA_TX81Z_H__


#include "../TopLevel.h"
#include <Arduino.h>


/**
	NRPN MAPPING
	
	LSB ONLY
	
	0...93		VCED Parameter
	94...117	ACED Parameter 0...22

	MSB ONLY
	
	0...127		Value

*/


struct _yamahaTX81ZLocal
	{
	};


// We need to pause about 50ms, sadly.
#define YAMAHA_TX81Z_COUNTDOWN		(150)

void stateSynthYamahaTX81Z();

#endif

