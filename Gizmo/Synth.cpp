////// Copyright 2017 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"

#ifdef INCLUDE_SYNTH

#ifdef INCLUDE_SYNTH_WALDORF_BLOFELD
#include "synth/WaldorfBlofeld.cpp"
#endif

#ifdef INCLUDE_SYNTH_KAWAI_K4
#include "synth/KawaiK4.cpp"
#endif

#ifdef INCLUDE_SYNTH_OBERHEIM_MATRIX_1000
#include "synth/OberheimMatrix1000.cpp"
#endif

#ifdef INCLUDE_SYNTH_KORG_MICROSAMPLER
#include "synth/KorgMicrosampler.cpp"
#endif

#ifdef INCLUDE_SYNTH_YAMAHA_TX81Z
#include "synth/YamahaTX81Z.cpp"
#endif

#endif