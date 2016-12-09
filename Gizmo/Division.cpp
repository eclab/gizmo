////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "Division.h"


//////////// UTILITY FUNCTIONS FOR DIVISION


uint32_t div100(uint32_t n) 
    {
    uint32_t q, r;
    q = (n >> 1) + (n >> 3) + (n >> 6) - (n >> 10) + (n >> 12) + (n >> 13) - (n >> 16);
    q = q + (q >> 20);
    q = q >> 6;
    r = n - q * 100;
// return q + ((r + 28) >> 7);          // this is another alternative
    return q + (r > 99);
    }


/// Below are two utility functions for doing software division by 10,
/// useful for printing digits etc. There is no hardware division on 
/// the Aduino, so it must be done in software, which runs at about
/// 68 times per ms.  You can speed this up to 241 times per second AND
/// get the remainder for free with the function below.  Also provided
/// Are methods for getting larger ranges at the cost of slower functions.


/// Software division of positive numbers by 10.  Returns the result
/// and places the remainder in the global variable <<remainder>>.
/// This function is about 220 times per ms, about 3 times faster 
/// than plain division on the arduino, and if you need mod as well, 
/// it's 6 times faster (since it provides both).  The range is only 
/// for numbers 0 ... 10929 inclusive.  If you need a (somewhat) larger
/// range, a slightly slower function is provided in comments in this code.
/// See http://stackoverflow.com/questions/5558492/divide-by-10-using-bit-shifts
uint16_t div10(uint16_t dividend)
    {
    uint32_t invDivisor = 0x1999;
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }

/// Slightly slower version with larger range.
/// This version runs 180 times per ms, but its range is 0 ... 43690
/// (for 16-bit ints).  See http://homepage.cs.uiowa.edu/~jones/bcd/divide.html
/// If this were converted from uint16_t to uint32_t, its range would be 
/// 0 ... 534890 but would be only 89 times per ms

// uint16_t div10(uint16_t A)
// {
// uint16_t Q = ((A >> 1) + A) >> 1;
//      Q = ((Q >> 4) + Q); 
//      Q = ((Q >> 8) + Q) >> 3;
//
//      uint16_t remainder = ((Q << 2) + Q) << 1;
//      remainder = A - remainder;
//      if (remainder >= 10) 
//              {
//              remainder = remainder - 10;
//              Q = Q + 1;
//              }
//      return Q;
//  }


// Range of 0...16391
uint16_t div12(uint16_t dividend)
    {
    uint32_t invDivisor = 0x1555; 
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }

// Range of 0...9369
uint16_t div9(uint16_t dividend)
    {
    uint32_t invDivisor = 0x1C71; 
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }
        
// Range of 0...32773
uint16_t div7(uint16_t dividend)
    {
    uint32_t invDivisor = 0x2492;
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }
                
// Range of 0...16385
uint16_t div6(uint16_t dividend)
    {
    uint32_t invDivisor = 0x2AAA;
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }

// Range of 0...65535   
uint16_t div5(uint16_t dividend)
    {
    uint32_t invDivisor = 0x3333;
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }

// Range of 0...65535
uint16_t div3(uint16_t dividend)
    {
    uint32_t invDivisor = 0x5555;
    uint16_t div = (uint16_t) ((invDivisor * (dividend + 1)) >> 16);
    return div;
    }

