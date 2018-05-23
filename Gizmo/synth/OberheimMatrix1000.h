////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __SYNTH_OBERHEIM_MATRIX_1000_H__
#define __SYNTH_OBERHEIM_MATRIX_1000_H__


#include "../TopLevel.h"
#include <Arduino.h>

struct _oberheimMatrix1000Local
	{
	};

// Incoming CC values are 3 to 5 times faster than outgoing NRPN 
// (we never send the LSB because we only have 100 NRPN paramters)
// So we need that much breathing room AT LEAST, ideally twice that.
// 5 CC messages is 15 bytes.  So we'll set this to 32.

#define OBERHEIM_MATRIX_1000_COUNTDOWN		(32)

void stateSynthOberheimMatrix1000();

#endif

