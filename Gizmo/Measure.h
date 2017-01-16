#ifndef __MEASURE_H__
#define __MEASURE_H__

struct _measureLocal
	{
	uint32_t initialTime;
	uint16_t beatsSoFar;
	uint8_t running;
	uint8_t displayElapsedTime;
	};

void stateMeasure();
void playMeasure();
void measureMenu();

#endif
