#ifndef __KAWAI_K4_H__
#define __KAWAI_K4_H__

/*** NOTES ON THE KAWAI K4

NRPN MAPPING

PARAMETER
MSB		Parameter number (0...88)
LSB		Source 0=s1 1=s2 2=s3 3=s4 (if parameter is 0-69)
		or Drum Key 0-60 (if parameter is 70-81)
		or Submix/Output Channel 0-7 (if parameter is 82-88)

VALUE
LSB+MSB	many values have ranges over 0...127
***/

#define KAWAI_K4_COUNTDOWN			(0)

struct _kawaiK4Local
	{
	};

void stateSynthKawaiK4();

#endif