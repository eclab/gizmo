////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __Division_h__
#define __Division_h__

#include <Arduino.h>


//// FUNCTIONS FOR DIVIDING BY SPECIFIC QUOTIENTS
////
//// These functions are faster in general than a general software divide
//// function such as used on the arduino.
////
//// We here provide functions for dividing unsigned integers by 
//// 3, 5, 6, 7, 9, 10, 12, and 100. Obviously dividing by 2, 4, 8, 16, etc. 
//// just consist of doing a right shift.
////
//// In most cases the functions here assume 16-bit numbers with limited range.
//// div100 assumes 32-bit numbers but suggests options for 16-bit numbers which
//// will be faster.


// Divides dividend by 3, returning the quotient.
// Range of 0...65535
// You can get the remainder afterwards via DIV3_REMAINDER
// This is significantly faster than using / or % (or both!)
uint16_t div3(uint16_t dividend);

// Given the original dividend and the quotient resulting from div3(dividend),
// returns the remainder, that is, the result of dividend % 3
//
// Example usage: computes both 9234 / 3 and 9234 % 3
// uint16_t dividend 9234;
// uint16_t quotient = div3(dividend);
// uint16_t remainder = DIV3_REMAINDER(quotient, dividend);
#define DIV3_REMAINDER(quotient, dividend)     ((dividend) - 3 * (quotient))


// Divides dividend by 5, returning the quotient.
// Range of 0...5929
// You can get the remainder afterwards via DIV5_REMAINDER
// This is significantly faster than using / or % (or both!)
uint16_t div5(uint16_t dividend);

// Given the original dividend and the quotient resulting from div5(dividend),
// returns the remainder, that is, the result of dividend % 5
//
// Example usage: computes both 9234 / 5 and 9234 % 5
// uint16_t dividend 9234;
// uint16_t quotient = div5(dividend);
// uint16_t remainder = DIV5_REMAINDER(quotient, dividend);
#define DIV5_REMAINDER(quotient, dividend)     ((dividend) - 5 * (quotient))


// Divides dividend by 6, returning the quotient.
// Range of 0...16385
// You can get the remainder afterwards via DIV6_REMAINDER
// This is significantly faster than using / or % (or both!)
//
// Note that you could instead do div3(dividend) >> 1, which
// is only just slightly slower but would have a range of 0...65535
//
uint16_t div6(uint16_t dividend);

// Given the original dividend and the quotient resulting from div6(dividend),
// returns the remainder, that is, the result of dividend % 6
//
// Example usage: computes both 9234 / 6 and 9234 % 6
// uint16_t dividend 9234;
// uint16_t quotient = div6(dividend);
// uint16_t remainder = DIV6_REMAINDER(quotient, dividend);
#define DIV6_REMAINDER(quotient, dividend)     ((dividend) - 6 * (quotient))


// Divides dividend by 7, returning the quotient.
// Range of 0...32773
// You can get the remainder afterwards via DIV7_REMAINDER
// This is significantly faster than using / or % (or both!)
uint16_t div7(uint16_t dividend);

// Given the original dividend and the quotient resulting from div7(dividend),
// returns the remainder, that is, the result of dividend % 7
//
// Example usage: computes both 9234 / 7 and 9234 % 7
// uint16_t dividend 9234;
// uint16_t quotient = div7(dividend);
// uint16_t remainder = DIV7_REMAINDER(quotient, dividend);
#define DIV7_REMAINDER(quotient, dividend)     ((dividend) - 7 * (quotient))


// Divides dividend by 9, returning the quotient.
// Range of 0...9369
// You can get the remainder afterwards via DIV9_REMAINDER
// This is significantly faster than using / or % (or both!)
uint16_t div9(uint16_t dividend);

// Given the original dividend and the quotient resulting from div9(dividend),
// returns the remainder, that is, the result of dividend % 9
//
// Example usage: computes both 9234 / 9 and 9234 % 9
// uint16_t dividend 9234;
// uint16_t quotient = div9(dividend);
// uint16_t remainder = DIV9_REMAINDER(quotient, dividend);
#define DIV9_REMAINDER(quotient, dividend)     ((dividend) - 9 * (quotient))


// Divides dividend by 10, returning the quotient.
// Range of 0...10929
// You can get the remainder afterwards via DIV10_REMAINDER
// This is significantly faster than using / or % (or both!)
//
// Note that you could instead do div5(dividend) >> 1, which
// is only just slightly slower but would have a range of 0...65535
//
// Also note an alternative method in Division.cpp
uint16_t div10(uint16_t dividend);

// Given the original dividend and the quotient resulting from div10(dividend),
// returns the remainder, that is, the result of dividend % 10
//
// Example usage: computes both 9234 / 10 and 9234 % 10
// uint16_t dividend 9234;
// uint16_t quotient = div10(dividend);
// uint16_t remainder = DIV10_REMAINDER(quotient, dividend);
#define DIV10_REMAINDER(quotient, dividend)     ((dividend) - 10 * (quotient))


// Divides dividend by 12, returning the quotient.
// You can get the remainder afterwards via DIV12_REMAINDER
// This is significantly faster than using / or % (or both!)
// Range of 0...16391
//
// Note that you could instead do div6(dividend) >> 1, but it
// is slightly slower and has the same range.
//
uint16_t div12(uint16_t dividend);

// Given the original dividend and the quotient resulting from div12(dividend),
// returns the remainder, that is, the result of dividend % 12
//
// Example usage: computes both 9234 / 12 and 9234 % 12
// uint16_t dividend 9234;
// uint16_t quotient = div12(dividend);
// uint16_t remainder = DIV12_REMAINDER(quotient, dividend);
#define DIV12_REMAINDER(quotient, dividend)     ((dividend) - 12 * (quotient))


// Divides dividend by 100, returning the quotient.  Note that unlike other
// functions in this file, div100 is *32 bit*.  So it's not super fast.
// You can get the remainder afterwards via DIV100_REMAINDER
// This is significantly faster than using / or % (or both!)
// Range of 0...4294967291 (that is, the full 32 range)
//
// Note that you might be able to do div10(div10(dividend)) or
// div5(div5(dividend)) >> 2 for 16-bit numbers and it'd be a bit faster.
//
// This procedure is from the book "Hacker's Delight".
uint32_t div100(uint32_t n);


// Given the original dividend and the quotient resulting from div100(dividend),
// returns the remainder, that is, the result of dividend % 100
//
// Example usage: computes both 9234 / 100 and 9234 % 100
// uint16_t dividend 9234;
// uint16_t quotient = div100(dividend);
// uint16_t remainder = DIV100_REMAINDER(quotient, dividend);
#define DIV100_REMAINDER(quotient, dividend)     ((dividend) - 100 * (quotient))



#endif

