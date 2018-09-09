////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License
//////


#ifndef __KAWAI_K4_H__
#define __KAWAI_K4_H__

/*** NOTES ON THE KAWAI K4


101:	NUMBER	0=s1 1=s2 2=s3 3=s4  Source
				0=f1 1=f2 Filter
				0...60 Drum Key
				0...7 Submix/Output Channel
***/

#define KAWAI_K4_COUNTDOWN			(0)

#define KAWAI_K4_NUMBER_PARAM	(100)
#define KAWAI_K4_HIGH_PARAM		(101)

struct _kawaiK4Local
	{
	uint8_t number;
	uint8_t high;
	};

void stateSynthKawaiK4();

#endif