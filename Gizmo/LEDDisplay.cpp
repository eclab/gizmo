////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License





// You'll see GLOBAL here and there.  It means nothing -- but it makes it easy
// for me to grep for global variables.
#define GLOBAL

#include "LEDDisplay.h"
#include "Division.h"
#include "nbw/NonBlockingWire.h"

//// MODIFING THE I2C BUFFERS
//// Wire uses 217 bytes!!!
//// The I2C buffers are very large.  We need them to be 16 in size at most (for the 
//// 16x8 backpack) and 8 in size (for the 8x8 backpack).

// We might need some memory from them.  To do this we can reduce their
// size by changing the following defines (in the include files):

// BUFFER_LENGTH        In Wire.h
// TWI_BUFFER_LENGTH    In twi.h

// Changing this from 32 to 16 would save us 80 bytes!

#define LED_BRIGHTNESS_I2C  ((uint8_t) 0xE0)



//////////////////  FONTS


/// Defines which make it easy to write bitmap fonts
// The << 3 shifts the 5 bits to the top of the LED screen

#define A00000A  ((char)(0 << 3))
#define A10000A  ((char)(1 << 3))
#define A01000A  ((char)(2 << 3))
#define A11000A  ((char)(3 << 3))
#define A00100A  ((char)(4 << 3))
#define A10100A  ((char)(5 << 3))
#define A01100A  ((char)(6 << 3))
#define A11100A  ((char)(7 << 3))
#define A00010A  ((char)(8 << 3))
#define A10010A  ((char)(9 << 3))
#define A01010A  ((char)(10 << 3))
#define A11010A  ((char)(11 << 3))
#define A00110A  ((char)(12 << 3))
#define A10110A  ((char)(13 << 3))
#define A01110A  ((char)(14 << 3))
#define A11110A  ((char)(15 << 3))
#define A00001A  ((char)(16 << 3))
#define A10001A  ((char)(17 << 3))
#define A01001A  ((char)(18 << 3))
#define A11001A  ((char)(19 << 3))
#define A00101A  ((char)(20 << 3))
#define A10101A  ((char)(21 << 3))
#define A01101A  ((char)(22 << 3))
#define A11101A  ((char)(23 << 3))
#define A00011A  ((char)(24 << 3))
#define A10011A  ((char)(25 << 3))
#define A01011A  ((char)(26 << 3))
#define A11011A  ((char)(27 << 3))
#define A00111A  ((char)(28 << 3))
#define A10111A  ((char)(29 << 3))
#define A01111A  ((char)(30 << 3))
#define A11111A  ((char)(31 << 3))


// This semicolon is critical to clue Emacs into properly
// indenting the following array.  Go figure.
;

const char PROGMEM font_2x5[1][2] = {
        { //1
        A00010A,
        A11111A
        },
    };


const char PROGMEM font_3x5[47][3] = {
        { // (
        A01110A,
        A10001A,
        A00000A,
        },
        { // )
        A00000A,
        A10001A,
        A01110A,
        },
        { //    UP
        A00010A,
        A11111A,
        A00010A,
        },  
        { // +  PLUS
        A00100A,
        A01110A,
        A00100A
        },
        { //    DOWN 
        A01000A,
        A11111A,
        A01000A,
        },
        { // -  MINUS 
        A00100A,
        A00100A,
        A00100A,
        },
        { //  . PERIOD
        A00000A,
        A10000A,
        A00000A
        },
        { // /  SLASH
        A11000A,
        A01110A,
        A00011A,
        },
        { //0
        A01110A,
        A10001A,
        A01110A
        },
        { //1
        A10010A,
        A11111A,
        A10000A,
        },
        { //2
        A11001A,
        A10101A,
        A10010A
        },
        { //3
        A10001A,
        A10101A,
        A11111A,
        },
        { //4
        A00111A,
        A00100A,
        A11111A
        },
        { //5
        A10111A,
        A10101A,
        A01001A
        },
        { //6
        A11111A,
        A10101A,
        A11101A
        },
        { //7
        A11001A,
        A00101A,
        A00011A
        },
        { //8
        A11111A,
        A10101A,
        A11111A
        },
        { //9
        A10111A,
        A10101A,
        A11111A
        },
        { //A
        A11111A,
        A00101A,
        A11111A
        },
        { //B
        A11111A,
        A10101A,
        A01010A
        },
        { //C
        A01110A,
        A10001A,
        A10001A
        },
        { //D
        A11111A,
        A10001A,
        A01110A
        },
        { //E
        A11111A,
        A10101A,
        A10101A
        },
        { //F
        A11111A,
        A00101A,
        A00101A
        },
        { //G
        A01110A,
        A10001A,
        A01101A,
        },
        { //H
        A11111A,
        A00100A,
        A11111A
        },
        { //I
        A10001A,
        A11111A,
        A10001A
        },
        { //J
        A01000A,
        A10000A,
        A01111A
        },
        { //K
        A11111A,
        A00100A,
        A11011A
        },
        { //L
        A11111A,
        A10000A,
        A10000A
        },
        { //M               // Unreadable
        A11111A,
        A00110A,
        A11111A
        },
        { //N
        A11110A,
        A00001A,
        A11110A
        },
        { //O
        A01110A,
        A10001A,
        A01110A
        },
        { //P
        A11111A,
        A00101A,
        A00111A
        },
        { //Q               // Tough to Read
        A00110A,
        A01001A,
        A10110A
        },
        { //R
        A11111A,
        A01101A,
        A10111A
        },
        { //S
        A10010A,
        A10101A,
        A01001A,
        },
        { //T
        A00001A,
        A11111A,
        A00001A
        },
        { //U
        A11111A,
        A10000A,
        A11111A,
        },
        { //V               // Tough to read
        A01111A,
        A10000A,
        A01111A,
        },
        { //W               // Unreadable
        A11111A,
        A01100A,
        A11111A
        },
        { //X
        A11011A,
        A00100A,
        A11011A
        },
        { //Y
        A00011A,
        A11100A,
        A00011A
        },
        { //Z
        A11001A,
        A10101A,
        A10011A
        },
        { // b (flat)
        A11111A,
        A10100A,
        A01100A
        },
        { // /  UP-DOWN
        A01010A,
        A11111A,
        A01010A,
        },
        { // BLANK
        A00000A,
        A00000A,
        A00000A,
        },
    };



const char PROGMEM font_4x5[19][4] = {
        { //10   // not super recognizable
        A11111A,
        A01110A,
        A10001A,
        A01110A
        },
        { //11
        A11111A,
        A00000A,
        A00000A,
        A11111A,
        },
        { //12
        A11111A,
        A00000A,
        A11101A,
        A10111A
        },  
        { //13
        A11111A,
        A00000A,
        A10101A,
        A11111A
        },  
        { //14          // Not very readable
        A11111A,
        A00111A,
        A00100A,
        A11111A
        },  
        { //15
        A11111A,
        A00000A,
        A10111A,
        A11101A
        },  
        { //16          // Not very readable
        A11111A,
        A01110A,
        A10101A,
        A01101A
        },  
        { //17
        A11111A,
        A00000A,
        A11101A,
        A00011A
        },  
        { //18          // Not very readable
        A11111A,
        A01110A,
        A10101A,
        A01110A
        },  
        { //19          // Not very readable
        A11111A,
        A00010A,
        A00101A,
        A11111A
        },  
        { // -1
        A00100A,
        A00100A,
        A00010A,
        A11111A
        },
        { // -2
        A00100A,
        A00000A,
        A11101A,
        A10111A
        },  
        { // -3
        A00100A,
        A00000A,
        A10101A,
        A11111A
        },
        { // -4
        A00100A,
        A00011A,
        A00100A,
        A11111A
        },
        { // -5
        A00100A,
        A00000A,
        A10111A,
        A11101A
        },
        { // -6
        A00100A,
        A01110A,
        A10101A,
        A11101A
        },
        { // -7
        A00100A,
        A00001A,
        A11101A,
        A00011A
        },
        { // -8
        A00100A,
        A11011A,
        A10101A,
        A11011A
        },
        { // -9
        A00100A,
        A00011A,
        A00101A,
        A11111A
        },
    };
    

// interestingly, the AVR preprocessor for GCC
// does NOT REQUIRE the equals sign below.  This was
// a gotcha for me that wasn't caught.  :-(
const char PROGMEM font_5x5[5][5] = {
        { //M
        A11111A,
        A00010A,
        A00100A,
        A00010A,
        A11111A
        },
        { //N
        A11111A,
        A00010A,
        A00100A,
        A01000A,
        A11111A
        },
        { //Q
        A01110A,
        A10001A,
        A10001A,
        A11110A,
        A10000A
        },
        { //V
        A00011A,
        A01100A,
        A10000A,
        A01100A,
        A00011A
        },
        { //W
        A01111A,
        A10000A,
        A01100A,
        A10000A,
        A01111A
        },
    };

  
/////   Fixed width 8-wide glyphs
/////
/////   Many of these are representations of different note lengths:
/////   Eighth-Triplet (1/96), Quarter-Triplet (1/48), 32nd note (1/32) Half-Triplet (1/24), 1/16, 1/12 (triplet), 1/8, 1/4, 1/3 (1/4 + triplet), 1/2, 3/4 (dotted half), 1 (whole), 1.5 (dotted whole), 2 (double whole)
/////
/////   Additionally, we have infinity, 1/8, 1/6, 1/4, 1/3, and 1/2.
/////
/////   Plus we have some two-part glyphs: "SURE?", "GIZMO x" (for various numbers x: 8 bits high), and some waves that I'm not using right now.

const char PROGMEM font_8x5[
#ifdef INCLUDE_EXTENDED_FONT        
    40
#else
    28
#endif
    ][8] = {
        { // 1/96 note (eighth-triplet) [1 MIDI clock beat]
        A10000A,
        A11111A,
        A01111A,
        A11111A,
        A11111A,
        A01111A,
        A11111A,
        A11111A,
        },
            { // 1/48 note (quarter-triplet) [2 MIDI clock beats]
            A10000A,
            A11111A,
            A00111A,
            A10111A,
            A11111A,
            A00111A,
            A10111A,
            A11111A,
            },
            { // 1/32 note [3 MIDI clock beats]
            A00000A,
            A10000A,
            A11111A,
            A00111A,
            A00111A,
            A10111A,
            A11111A,
            A00000A,
            },
            { // 1/24 note (half-triplet) [4 MIDI clock beats]
            A10000A,
            A11111A,
            A00101A,
            A10101A,
            A11111A,
            A00101A,
            A10101A,
            A11111A,
            },
            { // 1/16 note  [6 MIDI clock beats]
            A00000A,
            A10000A,
            A11111A,
            A00101A,
            A00101A,
            A10101A,
            A11111A,
            A00000A,
            },
            { // 1/12 note (triplet)  [8 MIDI clock beats]
            A11000A,
            A11111A,
            A00011A,
            A11011A,
            A11111A,
            A00011A,
            A11011A,
            A11111A,
            },
            { // 1/8   [12 MIDI beats]
            A00000A,
            A11000A,
            A11111A,
            A00011A,
            A00011A,
            A11011A,
            A11111A,
            A00000A,
            },
            { // 1/4 (triplet)  [16 MIDI beats]
            A11000A,
            A11000A,
            A11111A,
            A00000A,
            A00000A,
            A00001A,
            A00111A,
            A00001A,
            },
            { // Dotted 1/8   [18 MIDI beats]
            A11000A,
            A11111A,
            A00011A,
            A00110A,
            A00000A,
            A00000A,
            A10000A,
            A00000A,
            },
            { // 1/4   [24 MIDI beats]
            A11000A,
            A11000A,
            A11111A,
            A00000A,
            A00000A,
            A00000A,
            A00000A,
            A00000A,
            },
        /*
          { // 1/4 tied to triplet (32 MIDI beats)  [half note triplet]
          A11000A,
          A11000A,
          A11111A,
          A00000A,
          A10000A,
          A00001A,
          A00111A,
          A00001A,
          },
        */
            { // Dotted 1/4 note (36 MIDI beats)
            A11000A,
            A11000A,
            A11111A,
            A00000A,
            A10000A,
            A00000A,
            A00000A,
            A00000A,
            },
            { // Half note (48 MIDI beats)
            A11100A,
            A10100A,
            A11111A,
            A00000A,
            A00000A,
            A00000A,
            A00000A,
            A00000A,
            },
        /*
          { // 1/2 tied to two triplets (64 MIDI beats) [whole note triplet
          A11100A,
          A10100A,
          A11111A,
          A00000A,
          A10000A,
          A00001A,
          A00111A,
          A00001A,
          },
        */
            { // Dotted half note (72 MIDI beats)
            A11100A,
            A10100A,
            A11111A,
            A00000A,
            A10000A,
            A00000A,
            A00000A,
            A00000A,
            },
            { // Whole note (96 MIDI beats)
            A11100A,
            A10100A,
            A10100A,
            A11100A,
            A00000A,
            A00000A,
            A00000A,
            A00000A,
            },
            { // Dotted whole note (144 MIDI beats)
            A11100A,
            A10100A,
            A10100A,
            A11100A,
            A00000A,
            A10000A,
            A00000A,
            A00000A,
            },
            { // Double whole note (198 MIDI beats)
            A11100A,
            A00000A,
            A11100A,
            A10100A,
            A10100A,
            A11100A,
            A00000A,
            A11100A,
            },
            { // Infinity
            A00000A,
            A00100A,
            A01010A,
            A01010A,
            A00100A,
            A01010A,
            A01010A,
            A00100A
            },
            { // 1/8
            A11111A,
            A00000A,
            A11100A,
            A00111A,
            A00000A,
            A11111A,
            A10101A,
            A11111A
            },
            { // 1/6
            A11111A,
            A00000A,
            A11100A,
            A00111A,
            A00000A,
            A11111A,
            A10101A,
            A11101A,
            },
            { // 1/4
            A11111A,
            A00000A,
            A11100A,
            A00111A,
            A00000A,
            A00111A,
            A00100A,
            A11111A
            },
            { // 1/3
            A11111A,
            A00000A,
            A11100A,
            A00111A,
            A00000A,
            A10101A,
            A10101A,
            A11111A
            },
            { // 1/2
            A11111A,
            A00000A,
            A11100A,
            A00111A,
            A00000A,
            A11001A,
            A10101A,
            A10010A
            },
            { // SURE? pt 1
            A10111A,
            A11101A,
            A00000A,
            A11111A,
            A10000A,
            A11111A,
            A00000A,
            A11111A
            },
            { // SURE? pt 2
            A00101A,
            A11010A,
            A00000A,
            A11111A,
            A10101A,
            A00000A,
            A10101A,
            A00011A
            },
            { // EXIT? pt 1
            A11111A,
            A10101A,
            A10101A,
            A00000A,
            A11011A,
            A00100A,
            A11011A,
            A00000A,
            },
            { // EXIT? pt 2
            A11111A,
            A00000A,
            A00001A,
            A11111A,
            A00001A,
            A00000A,
            A10101A,
            A00011A
            },

        // NOTE we're using gcc's 0b.... syntax.
        // This is not portable of course, but who cares.
        // It's also, for our purposes, *backwards*.
        // So note that the strings should be flipped
        // horizontally to make much sense of them.



            { // GIZMO VERSION x... pt 1
            0b01100111,
            0b10010100,
            0b10110010,
            0b00000100,
            0b11110111,
            0b00000000,
            0b10010111,
            0b10110101,
            },
        
            { // GIZMO VERSION 10 pt 2
            0b11010101,
            0b10010111,
            0b00000000,
            0b01111100,
            0b00000000,
            0b00111000,
            0b01000100,
            0b00111000,
            },

        //            { // GIZMO VERSION 9 pt 2
        //            0b11010101,
        //            0b10010111,
        //            0b00000000,
        //            0b00000000,
        //            0b01110100,
        //            0b01010100,
        //            0b01111100,
        //            0b00000000,
        //            },

        //            { // GIZMO VERSION 8 pt 2
        //            0b11010101,
        //            0b10010111,
        //            0b00000000,
        //            0b00000000,
        //            0b01111100,
        //            0b01010100,
        //            0b01111100,
        //            0b00000000,
        //            },

        //            { // GIZMO VERSION 7 pt 2
        //            0b11010101,
        //            0b10010111,
        //            0b00000000,
        //            0b00000000,
        //            0b01001100,
        //            0b01010000,
        //            0b01100000,
        //            0b00000000,
        //            },

        //            { // GIZMO VERSION 6 pt 2
        //            0b11010101,
        //            0b10010111,
        //            0b00000000,
        //            0b00000000,
        //            0b01111100,
        //            0b01010100,
        //            0b01011100,
        //            0b00000000,
        //            },

        //            { // GIZMO VERSION 5 pt 2
        //            0b11010101,
        //            0b10010111,
        //            0b00000000,
        //            0b00000000,
        //            0b01110100,
        //            0b01010100,
        //            0b01001000,
        //            0b00000000,
        //            },

        //            { // GIZMO VERSION 4 pt 2
        //            0b11010101,
        //            0b10010111,
        //            0b00000000,
        //            0b00000000,
        //            0b01110000,
        //            0b00010000,
        //            0b01111100,
        //            0b00000000,
        //            },

        //        { // GIZMO VERSION 3 pt 2
        //        0b11010101,
        //        0b10010111,
        //        0b00000000,
        //        0b00000000,
        //        0b01000100,
        //        0b01010100,
        //        0b01111100,
        //        0b00000000,
        //        },

        //        { // GIZMO VERSION 2 pt 2
        //        0b11010101,
        //        0b10010111,
        //        0b00000000,
        //        0b00000000,
        //        0b01001100,
        //        0b01010100,
        //        0b00100100,
        //        0b00000000,
        //        },

        //        { // GIZMO VERSION 1 pt 2
        //        0b11010101,
        //        0b10010111,
        //        0b00000000,
        //        0b00000000,
        //        0b00100000,
        //        0b01111100,
        //        0b00000000,
        //        0b00000000,
        //        },

#ifdef INCLUDE_EXTENDED_FONT        
            {
            A00110A,
            A00001A,
            A00001A,
            A01110A,
            A10000A,
            A10000A,
            A01100A,
            A00000A
            },      
            {
            A00001A,
            A00001A,
            A01110A,
            A10000A,
            A10000A,
            A01110A,
            A00001A,
            A00001A,
            },
            {
            A01100A,
            A10000A,
            A10000A,
            A01110A,
            A00001A,
            A00001A,
            A00110A,
            A00000A
            },
            {
            A10000A,
            A10000A,
            A01110A,
            A00001A,
            A00001A,
            A01110A,
            A10000A,
            A10000A,
            },
            {
            A00100A,
            A00010A,
            A00001A,
            A00010A,
            A00100A,        
            A01000A,
            A10000A,
            A01000A,
            },
            {
            A00001A,
            A00010A,
            A00100A,
            A01000A,
            A10000A,
            A01000A,
            A00100A,
            A00010A,
            },
            {
            A00100A,
            A01000A,
            A10000A,
            A01000A,
            A00100A,
            A00010A,
            A00001A,
            A00010A,
            },
            {
            A10000A,
            A01000A,
            A00100A,
            A00010A,
            A00001A,
            A00010A,
            A00100A,
            A01000A,
            },      
            {
            A11111A,
            A10000A,
            A01000A,
            A00100A,
            A00010A,
            A00001A,
            A11111A,
            A00000A,
            },
            {
            A11111A,
            A00001A,
            A00010A,
            A00100A,
            A01000A,
            A10000A,
            A11111A,
            A00000A,
            },
            {
            A11111A,
            A00001A,
            A00001A,
            A11111A,
            A10000A,
            A10000A,
            A11111A,
            A00000A,
            },
            {
            A11111A,
            A10000A,
            A10000A,
            A11111A,
            A00001A,
            A00001A,
            A11111A,
            A00000A,
            },
#endif
    };




////////////////// BLINKING


/// This increases from 0...255 as a counter to indicate when we should
/// turn blinked items on or off
GLOBAL static uint8_t blinkToggle = 0;

/// BlinkToggle >= than blinkOn will turn the blink on
GLOBAL static uint8_t blinkOn = 127;

// blinkToggle > blinkOff will reset to zero (and thus turn the blink off)
GLOBAL static uint8_t blinkOff = 255; 



// Sets the number of send(...) messages which pass before
// blink elements blink on and off respectively.  
// Note that blinkOn must be < blink off.
void setBlinkOnOff(uint8_t on, uint8_t off)
    {
    if (on >= off)  // uh oh
        return;
    blinkOn = on;
    blinkOff = off; 
    }

//////////////////  CONTROLLING THE LED



/// Internal rotation when we send a matrix
GLOBAL uint8_t rotation = DIR_NONE;


// Sets the rotation of the screen.  For a single 8x8 matrix, all four rotations
// make sense.  For a 16x8 matrix, only DIR_180 and DIR_NONE make sense; other
// rotations will be ignored.
void setRotation(uint8_t dir)
    {
    rotation = dir;
    }



// Sends an the matrix to the LED  [that is, matrix must be 8 bytes]
// matrix2 can be NULL only if we're using the 8x8 screens
#ifdef SCREEN_TYPE_ADAFRUIT_16x8_BACKPACK
// i2cAddress2 is ignored
void sendMatrix(unsigned char* matrix, unsigned char* matrix2, uint8_t i2cAddress = I2C_ADDRESS_1, uint8_t i2cAddress2 = I2C_ADDRESS_2)
    {
    // rotate as necessary
    uint8_t mat[8];
    uint8_t mat2[8];

#ifdef ROTATE_WHOLE_SCREEN
    // swap the two matrices. We'll rotate each one 180 later. This results in a full rotation of the combined screen
    unsigned char* temp = matrix;
    matrix = matrix2;
    matrix2 = temp;
#endif ROTATE_WHOLE_SCREEN

    // the display of the matrices is rotated 90 degrees, so we need to tweak here
    if (rotation >= DIR_180)  // rotate to 180 if we're DIR_180 or DIR_COUNTERCLOCKWISE_90, nothing else
        {
        memcpy(mat, matrix, 8);
        memcpy(mat2, matrix2, 8);
        rotateMatrix(mat, DIR_COUNTERCLOCKWISE_90);
        rotateMatrix(mat2, DIR_COUNTERCLOCKWISE_90);
        matrix2 = mat;  // note we're flipping them
        matrix = mat2;
        }
    else    // other rotations are considered DIR_NONE
        {
        memcpy(mat, matrix, 8);
        memcpy(mat2, matrix2, 8);
        rotateMatrix(mat, DIR_CLOCKWISE_90);
        rotateMatrix(mat2, DIR_CLOCKWISE_90);
        matrix = mat;  
        matrix2 = mat2;
        }

#ifdef ROTATE_WHOLE_SCREEN
    // rotate the two matrices. We swapped them earlier. This results in a full rotation of the combined screen
    rotateMatrix(matrix, DIR_180);
    rotateMatrix(matrix2, DIR_180);
#endif ROTATE_WHOLE_SCREEN

    Wire.beginTransmission(i2cAddress);
    Wire.write(0);

    for (uint8_t i=0; i<8; i++) 
        {
        Wire.write(matrix2[i]);    
        Wire.write(matrix[i]);    
        }
    Wire.endTransmissionNonblocking();  
    blinkToggle++;
    if (blinkToggle > blinkOff)
        blinkToggle = 0;
    }
#endif SCREEN_TYPE_ADAFRUIT_16x8_BACKPACK


// Sends an the matrix to the LED  [that is, matrix must be 8 bytes]
// matrix2 can be NULL only if we're using the 8x8 screens
#ifdef SCREEN_TYPE_ADAFRUIT_16x8_FEATHERWING_BACKPACK
// i2cAddress2 is ignored
void sendMatrix(unsigned char* matrix, unsigned char* matrix2, uint8_t i2cAddress = I2C_ADDRESS_1, uint8_t i2cAddress2 = I2C_ADDRESS_2)
    {
    // rotate as necessary
    uint8_t mat[8];
    uint8_t mat2[8];

#ifdef ROTATE_WHOLE_SCREEN
    // swap the two matrices. We'll rotate each one 180 later. This results in a full rotation of the combined screen
    unsigned char* temp = matrix;
    matrix = matrix2;
    matrix2 = temp;
#endif ROTATE_WHOLE_SCREEN

    // the display of the matrices is rotated 90 degrees, so we need to tweak here
    if (rotation >= DIR_180)  // rotate to 180 if we're DIR_180 or DIR_COUNTERCLOCKWISE_90, nothing else
        {
        memcpy(mat, matrix, 8);
        memcpy(mat2, matrix2, 8);
        rotateMatrix(mat, DIR_NONE);
        rotateMatrix(mat2, DIR_NONE);
        matrix2 = mat;  // note we're flipping them
        matrix = mat2;
        }
    else    // other rotations are considered DIR_NONE
        {
        memcpy(mat, matrix, 8);
        memcpy(mat2, matrix2, 8);
        rotateMatrix(mat, DIR_180);
        rotateMatrix(mat2, DIR_180);
        matrix2 = mat;  
        matrix = mat2;
        }

#ifdef ROTATE_WHOLE_SCREEN
    // rotate the two matrices. We swapped them earlier. This results in a full rotation of the combined screen
    rotateMatrix(matrix, DIR_180);
    rotateMatrix(matrix2, DIR_180);
#endif ROTATE_WHOLE_SCREEN

    Wire.beginTransmission(i2cAddress);
    Wire.write(0);

    for (uint8_t i=0; i<8; i++) 
        {
        Wire.write(matrix2[i]);    
        Wire.write(matrix[i]);    
        }
    Wire.endTransmissionNonblocking();  
    blinkToggle++;
    if (blinkToggle > blinkOff)
        blinkToggle = 0;
    }
#endif SCREEN_TYPE_ADAFRUIT_16x8_FEATHERWING_BACKPACK



// Sends an the matrix to the LED  [that is, matrix must be 8 bytes]
// matrix2 can be NULL only if we're using the 8x8 screens
#ifdef SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
// i2cAddress2 is used for second backpack
void sendMatrix(unsigned char* matrix, unsigned char* matrix2, uint8_t i2cAddress = I2C_ADDRESS_1, uint8_t i2cAddress2 = I2C_ADDRESS_2)
    {
    uint8_t mat[8];
    if (rotation != DIR_NONE)
        {
        memcpy(mat, matrix, 8);
        rotateMatrix(mat, rotation);
        matrix = mat;

        memcpy(mat, matrix2, 8);
        rotateMatrix(mat, rotation);
        matrix2 = mat;
        }       

#ifdef ROTATE_WHOLE_SCREEN
    // rotate the two matrices. We swapped them earlier. This results in a full rotation of the combined screen
    rotateMatrix(matrix, DIR_180);
    rotateMatrix(matrix2, DIR_180);
#endif ROTATE_WHOLE_SCREEN

    Wire.beginTransmission(i2cAddress);
    Wire.write(0);

    for (uint8_t i=0; i<8; i++) 
        {
        // A misfeature in the 8x8 display requires that we do a full left shift
        // of one bit
        uint8_t c = matrix[i];
        c = (c << 7) | (c >> 1);

        Wire.write(0);   
        Wire.write(matrix[i]); 
        }        
    Wire.endTransmissionNonblocking();  

    Wire.beginTransmission(i2cAddress2);
    Wire.write(0);

    for (uint8_t i=0; i<8; i++) 
        {
        // A misfeature in the 8x8 display requires that we do a full left shift
        // of one bit
        uint8_t c = matrix2[i];
        c = (c << 7) | (c >> 1);

        Wire.write(0);   
        Wire.write(matrix2[i]); 
        }        
    Wire.endTransmissionNonblocking();  

    blinkToggle++;
    if (blinkToggle > blinkOff)
        blinkToggle = 0;
    }
#endif SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS


#ifdef SCREEN_TYPE_MJKDZ_16x8_BACKPACK
// i2cAddress2 is ignored
void sendMatrix(unsigned char* matrix, unsigned char* matrix2, uint8_t i2cAddress = I2C_ADDRESS_1, uint8_t i2cAddress2 = I2C_ADDRESS_2)
    {
    // rotate as necessary
    uint8_t mat[8];
    uint8_t mat2[8];

#ifdef ROTATE_WHOLE_SCREEN
    // swap the two matrices. We'll rotate each one 180 later. This results in a full rotation of the combined screen
    unsigned char* temp = matrix;
    matrix = matrix2;
    matrix2 = temp;
#endif ROTATE_WHOLE_SCREEN

    // the display of the matrices is rotated 90 degrees, so we need to tweak here
    if (rotation >= DIR_180)  // rotate to 180 if we're DIR_180 or DIR_COUNTERCLOCKWISE_90, nothing else
        {
        memcpy(mat, matrix, 8);
        memcpy(mat2, matrix2, 8);
        rotateMatrix(mat, DIR_NONE);
        rotateMatrix(mat2, DIR_NONE);
        matrix2 = mat;  // note we're flipping them
        matrix = mat2;
        }
    else    // other rotations are considered DIR_NONE
        {
        memcpy(mat, matrix, 8);
        memcpy(mat2, matrix2, 8);
        rotateMatrix(mat, DIR_180);
        rotateMatrix(mat2, DIR_180);
        matrix2 = mat;  
        matrix = mat2;
        }

#ifdef ROTATE_WHOLE_SCREEN
    // rotate the two matrices. We swapped them earlier. This results in a full rotation of the combined screen
    rotateMatrix(matrix, DIR_180);
    rotateMatrix(matrix2, DIR_180);
#endif ROTATE_WHOLE_SCREEN

    flipMatrix(matrix, FLIP_X);
    flipMatrix(matrix2, FLIP_X);
    unsigned char* temp = matrix;
    matrix = matrix2;
    matrix2 = temp;

    Wire.beginTransmission(i2cAddress);
    Wire.write(0);

    for (uint8_t i=0; i<8; i++) 
        {
        Wire.write(matrix2[i]);    
        Wire.write(matrix[i]);    
        }
    Wire.endTransmissionNonblocking();  
    blinkToggle++;
    if (blinkToggle > blinkOff)
        blinkToggle = 0;
    }
#endif SCREEN_TYPE_MJKDZ_16x8_BACKPACK






// At present the KIT is about 3127 in 10000 ms
// The backpack at 100KHz is 3624 in 10000 ms :-(
// The backpack at 400KHz is 2828 in 10000ms
 
// Initializes the LED.  Call this in setup()
void _initLED(uint8_t i2cAddress)
    {
    Wire.begin();
    // Make I2C go faster (by default it's 100Hz).  The screens can handle it.
    Wire.setClock(400000L);
    //TWBR = 12; // 400 khz

    // It appears that all of the below is critical to get the screen up and running
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x21);  // Turn on the oscillator
    Wire.endTransmission();
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x81);  // Turn on the screen and turn OFF blinking
    Wire.endTransmission();
    }

void initLED()
    {
#ifdef TWO_SCREENS_VERTICAL
#ifdef SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
    _initLED(I2C_ADDRESS_1);
    _initLED(I2C_ADDRESS_2);
    _initLED(I2C_ADDRESS_3);
    _initLED(I2C_ADDRESS_4);
#else
    _initLED(I2C_ADDRESS_1);
    _initLED(I2C_ADDRESS_3);
#endif SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
#else
#ifdef SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
    _initLED(I2C_ADDRESS_1);
    _initLED(I2C_ADDRESS_2);
#else
#endif SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
    _initLED(I2C_ADDRESS_1);
#endif TWO_SCREENS_VERTICAL

    // Let's be dimmer
    setScreenBrightness(1);
    }


// Stolen from https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte/2602871
unsigned char reverseBits(unsigned char b) 
    {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
    }

// Flips a matrix in place, in either in the X or Y directions (flip == FLIP_X or FLIP_Y)
void flipMatrix(unsigned char* in, uint8_t flip)
    {
    if (flip == FLIP_X)
        {
        for(uint8_t i = 0; i < 4; i++)
            {
            unsigned char temp = in[i];
            in[i] = in[7 - i];
            in[7 - i] = temp;
            }
        }
    else            // FLIP_Y
        {
        for(uint8_t i = 0; i < 8; i++)
            {
            in[i] = reverseBits(in[i]);
            }
        }
    }

// Rotates a matrix in the given direction.
// There are lots of clever rotation code snippets out there, many derived
// from Hacker's Delight, but they're largely all 16- or 32-bit, and their
// tricks don't scale down to 8-bit in any speed improvement (that I know 
// of) over the brute-force approach below.
void rotateMatrix(unsigned char* in, uint8_t dir)
    {
    unsigned char rotateTemp[LED_WIDTH];
    if (dir == DIR_CLOCKWISE_90)
        {
        for(uint8_t i = 0; i < LED_WIDTH; i++)
            {
            uint8_t shift = (LED_WIDTH - 1 - i);
            rotateTemp[i] =  
                (((in[0] >> shift) & 0x01) << 0) |
                (((in[1] >> shift) & 0x01) << 1) |
                (((in[2] >> shift) & 0x01) << 2) |
                (((in[3] >> shift) & 0x01) << 3) |
                (((in[4] >> shift) & 0x01) << 4) |
                (((in[5] >> shift) & 0x01) << 5) |
                (((in[6] >> shift) & 0x01) << 6) |
                (((in[7] >> shift) & 0x01) << 7); 
            }
        }
    else if (dir == DIR_COUNTERCLOCKWISE_90)
        {     
        for(uint8_t i = 0; i < LED_WIDTH; i++)
            rotateTemp[i] =  
                (((in[0] >> i) & 0x01) << 7) |
                (((in[1] >> i) & 0x01) << 6) |
                (((in[2] >> i) & 0x01) << 5) |
                (((in[3] >> i) & 0x01) << 4) |
                (((in[4] >> i) & 0x01) << 3) |
                (((in[5] >> i) & 0x01) << 2) |
                (((in[6] >> i) & 0x01) << 1) |
                (((in[7] >> i) & 0x01) << 0);
        }
    else if (dir == DIR_180)
        {
        for(uint8_t i = 0; i < LED_WIDTH; i++)
            {
            uint8_t shifted = in[LED_WIDTH - 1 - i];
            rotateTemp[i] =  
                (((shifted >> 0) & 0x01) << 7) |
                (((shifted >> 1) & 0x01) << 6) |
                (((shifted >> 2) & 0x01) << 5) |
                (((shifted >> 3) & 0x01) << 4) |
                (((shifted >> 4) & 0x01) << 3) |
                (((shifted >> 5) & 0x01) << 2) |
                (((shifted >> 6) & 0x01) << 1) |
                (((shifted >> 7) & 0x01) << 0);
            }
        }
    else
        {
        // do nothing
        return;
        }
      
    // copy back
    memcpy(in, rotateTemp, LED_WIDTH);
    }

// Inverts a matrix (light becomes unlit and vice versa)
void invertMatrix(unsigned char* mat)
    {
    for(uint8_t i = 0; i < LED_WIDTH; i++)
        mat[i] =  0xFF ^ mat[i];
    }

// Clears a matrix (sets all to unlit)
void clearMatrix(unsigned char* mat)
    {
    memset(mat, 0, 8);
    }
  
// Sets a point (to lit)
void setPoint(unsigned char* mat, uint8_t x, uint8_t y)
    {
    mat[x] |= (0x01 << y);
    }

void setOrClearPoint(unsigned char* mat, uint8_t x, uint8_t y, uint8_t val)
    {
    if (val) 
        mat[x] |= (0x01 << y);
    else
        mat[x] &= ~(0x01 << y);
    }

// Clears a vertical line (sets it to unlit)
void clearVerticalLine(unsigned char* mat, uint8_t x, unsigned char line)
    {
    mat[x] &= ~line;
    }

// Sets a vertical line (to lit)
void setVerticalLine(unsigned char* mat, uint8_t x, unsigned char line)
    {
    mat[x] |= line;
    }

// Sets a point to blink.  This works as follows.  Each time sendMatrix is called,
// it increments a counter.  blinkPoint uses this counter to determine, based on the
// duty cycle above, whether to XOR the point or to not touch it.  
void blinkPoint(unsigned char* mat, uint8_t x, uint8_t y)
    {
    setOrClearPoint(mat, x, y, blinkToggle > blinkOn);
    //    if (blinkToggle > blinkOn)
    //        mat[x] |= (0x01 << y);
    //    else
    //        mat[x] &= ~(0x01 << y);
    }

// Blinks a vertical line
void blinkVerticalLine(unsigned char* mat, uint8_t x, unsigned char line)
    {
    if (blinkToggle > blinkOn)
        mat[x] |= line;
    else
        mat[x] &= ~line;
    }


// Sets a point to unlit
void clearPoint(unsigned char* mat, uint8_t x, uint8_t y)
    {
    mat[x] &= ~(0x01 << y);
    }

// Writes a 2x5 glyph of the given WIDTH, starting at column X (which may be a value 0...6)
//void write2x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x)
//    {
//    if (x < 0 || x > 6) return;
//    memcpy_P(mat + x, font_2x5[glyph], 2);
//    }

// Writes a 3x5 glyph of the given WIDTH, starting at column X (which may be a value 0...5)
void write3x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x)
    {
    if (x < 0 || x > 5) return;
    memcpy_P(mat + x, font_3x5[glyph], 3);
    }

void blink3x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x)
    {
    if (blinkToggle > blinkOn)
        write3x5Glyph(mat, glyph, x);
    }

// Writes a 4x5 glyph of the given WIDTH, starting at column X (which may be a value 0...4)
void write4x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x)
    {
    if (x < 0 || x > 4) return;
    memcpy_P(mat + x, font_4x5[glyph], 4);
    }

// Writes a 5x5 glyph of the given WIDTH, starting at column X (which may be a value 0...3)
void write5x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x)
    {
    if (x < 0 || x > 3) return;
    memcpy_P(mat + x, font_5x5[glyph], 4);
    }


// Writes an 8x5 glyph of the given WIDTH to the matrix MAT
void write8x5Glyph(unsigned char* mat, uint8_t glyph)
    {
    memcpy_P(mat, font_8x5[glyph], 8);
    }


// Prints a number from -99 to 129 inclusive to the matrix.
// The values 128 and 129 are represented as -128 and -127
// respectively.
// If the number is outside this range, nothing will be printed and this
// function will silently fail.
//
// If leftJustify is TRUE then numbers 0...99 will be shifted to the
// left by one pixel.  The point of this is to make a little room between
// this number and the number to its right if we're writing two numbers.
void writeShortNumber(unsigned char* mat1, int8_t val, uint8_t leftJustify)
    {
    if (val > 129 || (val < -99 && val > -127)) return;

    uint8_t neg = 0;
    uint8_t val2 = val;                         // note that -128 will convert to 128 and 
    if (val2 > 129)                                     // implies that val is a negative number
        {
        val2 = -val;
        neg = 1;
        }
                
    uint16_t d10 = div10(val2);
    uint16_t d1 = DIV10_REMAINDER(d10, val2);
   
    uint8_t next = (leftJustify ? 4 : 5);

    if (neg)
        {
        if (d10 > 0)
            {
            memcpy_P(mat1, font_4x5[GLYPH_4x5_NEGATIVE_1 + d10 - 1], 2);
            next = 5;
            }
        else
            {
            memcpy_P(mat1 + !leftJustify, font_3x5[GLYPH_3x5_MINUS], 3);
            }
        }
    else
        {
        if (d10 >= 10)
            {
            memcpy_P(mat1, font_4x5[GLYPH_4x5_10 + d10 - 10], 4);
            next = 5;
            }
        else if (d10 > 0)
            {
            memcpy_P(mat1 + !leftJustify, font_3x5[GLYPH_3x5_0 + d10], 3);          
            }
        }
                        
    memcpy_P(mat1 + next, font_3x5[GLYPH_3x5_0 + d1], 3);
    }

        
// Writes a number (-9999...19999) as a string to the given buffer (which must be 6 in size)
void numberToString(char* buffer, int16_t val)
    {
    strcpy_P(buffer, PSTR("     "));  // 5 spaces. This will get put in dynamic memory :-(

    if (val < -9999 || val > 19999) { return; }    

    uint16_t m = 0;
    uint16_t m2 = 0;
    uint16_t d1 = 0;
    uint16_t d10 = 0;
    uint16_t d100 = 0;
    uint16_t d1000 = 0;
    uint16_t d10000 = 0;
    
    if (val > 9999)  // gotta handle the most significant 1 specially because div10 only goes to about 10000
        {
        d10000 = 1;
        val -= 10000;
        }
        
    uint16_t v = (uint16_t)(val < 0 ? -val : val);
    m = div10(v);
    d1 = DIV10_REMAINDER(m, v);
    if (m > 0)
        {
        m2 = div10(m);
        d10 = DIV10_REMAINDER(m2, m);
        if (m2 > 0)
            {
            d1000 = div10(m2);
            d100 = DIV10_REMAINDER(d1000, m2);
            }
        }
                
    uint8_t zerofill = 0;
    
    if (val >= 0)
        {
        if (d10000)
            {
            buffer[0] = d10000 + '0';
            zerofill = 1;
            }
        if (d1000 > 0 || zerofill)
            {
            buffer[1] = d1000 + '0';
            zerofill = 1;
            }
        if (d100 > 0 || zerofill)
            {
            buffer[2] = d100 + '0';
            zerofill = 1;
            }
        if (d10 > 0 || zerofill)
            {
            buffer[3] = d10 + '0';
            zerofill = 1;
            }
        // ones digit
        buffer[4] = d1 + '0';
        }
    else
        {
        uint8_t drewMinus = 0;
        if (d1000 > 0)
            {
            buffer[0] = '-';
            buffer[1] = d1000 + '0';
            zerofill = 1;
            drewMinus = 1;
            }
        if (d100 > 0 || zerofill)
            {
            if (!drewMinus)  // need to add a minus
                { buffer[1] = '-' ; drewMinus = 1; }
            buffer[2] = d100 + '0';
            zerofill = 1;
            }
        if (d10 > 0 || zerofill)
            {
            if (!drewMinus)  // need to add a minus
                { buffer[2] = '-' ; drewMinus = 1; }
            buffer[3] = d10 + '0';
            zerofill = 1;
            }
        if (d1 > 0 || zerofill)
            {
            if (!drewMinus)  // need to add a minus
                { buffer[3] = '-' ; }   // no need for this: // drewMinus = 1; }
            buffer[4] = d1 + '0';
            // zerofill = 1;    // no need for this
            }
        }
    }



// Writes a number (-9999...19999) to two matrices.
// The numbers are all written straight [though squished]
void writeNumber(unsigned char* mat1, unsigned char* mat2, int16_t val)
    {
    char b[6];
    numberToString(b, val);
    uint8_t neg = 0;
        
    if (b[0] == '1')
        memcpy_P(mat2, font_4x5[GLYPH_4x5_10 + b[1] - '0'], 4);
    else if (b[0] == '-')
        { memcpy_P(mat2, font_4x5[GLYPH_4x5_NEGATIVE_1 + b[1] - '0' - 1], 4); neg = 1; }
    else if (b[1] >= '0' && b[1] <= '9')
        memcpy_P(mat2 + 1, font_3x5[GLYPH_3x5_0 + b[1] - '0'], 3);
    else if (b[1] == '-')
        { memcpy_P(mat2 + 1, font_3x5[GLYPH_3x5_MINUS], 3); neg = 1; }

    if (b[2] >= '0' && b[2] <= '9')
        memcpy_P(mat2 + 5, font_3x5[GLYPH_3x5_0 + b[2] - '0'], 3);
    else if (b[2] == '-')
        { memcpy_P(mat2 + 5, font_3x5[GLYPH_3x5_MINUS], 3); } // neg = 1; } // no need for this

    if (b[3] >= '0' && b[3] <= '9')
        memcpy_P(mat1 + 1, font_3x5[GLYPH_3x5_0 + b[3] - '0'], 3);
    else if (b[3] == '-')
        { memcpy_P(mat1 + 1, font_3x5[GLYPH_3x5_MINUS], 3); }   // no need for this: // neg = 1; }

    if (b[4] != ' ')
        memcpy_P(mat1 + 5, font_3x5[GLYPH_3x5_0 + b[4] - '0'], 3);
    }
        
        
 

// Writes a hex number (00...FF) to the matrix
void writeHex(unsigned char* mat, unsigned char val)
    {
    uint8_t m = val >> 4;     // / 16;  // compiler should shift this but apparently doesn't
    uint8_t l = val & 0xF;
    memcpy_P(mat + 1, font_3x5[GLYPH_3x5_0 + m], 3);
    memcpy_P(mat + 5, font_3x5[GLYPH_3x5_0 + l], 3);
    }

void writeFlat(unsigned char* mat)
    {
    memcpy_P(mat + 4, font_3x5[GLYPH_3x5_FLAT], 3);
    }

// Prints a note to the matrix.  If the note is
// not in the range NOTE_C ... NOTE_B  (0 ... 11), nothing
// is printed and this function will silently fail.
// Notes are written left justified.
void writeNote(unsigned char* mat, unsigned char note)
    {
    if (note > NOTE_B) return;
    
    switch(note)
        {
        case NOTE_C:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_C], 3);
            }
        break;
        case NOTE_Db:
            {
            writeFlat(mat);
            }
        // fall thru
        case NOTE_D:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_D], 3);
            }
        break;
        case NOTE_Eb:
            {
            writeFlat(mat);
            }
        // fall thru
        case NOTE_E:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_E], 3);
            }
        break;
        case NOTE_F:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_F], 3);
            }
        break;
        case NOTE_Gb:
            {
            writeFlat(mat);
            }
        // fall thru
        case NOTE_G:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_G], 3);
            }
        break;
        case NOTE_Ab:
            {
            writeFlat(mat);
            }
        // fall thru
        case NOTE_A:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_A], 3);
            }
        break;
        case NOTE_Bb:
            {
            writeFlat(mat);
            }
        // fall thru
        case NOTE_B:
            {
            memcpy_P(mat + 0, font_3x5[GLYPH_3x5_B], 3);
            }
        break;
        }
    }


/// Draws a series of points horizontally corresponding to a value from 0...(total-1)
/// This series draws to the right, then undraws to the right, and starts at column X
/// in row Y.  TOTAL ought to be an even number, up to 52, even if you don't use all of it.
///
/// An example.  Let us say that TOTAL is 8.  Then we will draw up to 4 points like this:
/// (an X is a point, a . is an unlit LED)
///
/// 0:  X...
/// 1:  XX..
/// 2:  XXX.
/// 3:  XXXX
/// 4:  .XXX
/// 5:  ..XX
/// 6:  ...X
/// 7:  ....
/// 
/// If the total is 16, then we draw 0...15 as:

///  0: X.......
///  1: XX......
///  2: XXX.....
///  3: XXXX....
///  4: XXXXX...
///  5: XXXXXX..
///  6: XXXXXXX.
///  7: XXXXXXXX
///  8: .XXXXXXX
///  9: ..XXXXXX
/// 10: ...XXXXX
/// 11: ....XXXX
/// 12: .....XXX
/// 13: ......XX
/// 14: .......X
/// 15: ........

/// If the total is > 16, then we draw 16...26 as:

/// 16: X.X.....
/// 17: X.XX....
/// 18: X.XXX...
/// 19: X.XXXX..
/// 20: X.XXXXX.
/// 21: X.XXXXXX
/// 22: X..XXXXX
/// 23: X...XXXX
/// 24: X....XXX
/// 25: X.....XX
/// 26: X......X

/// If the total is > 27, then we draw 27...35 as:

/// 27: XX.X....
/// 28: XX.XX...
/// 29: XX.XXX..
/// 30: XX.XXXX.
/// 31: XX.XXXXX
/// 32: XX..XXXX
/// 33: XX...XXX
/// 34: XX....XX
/// 35: XX.....X

/// If the total is > 36, then we draw 36...42 as:

/// 36: XXX.X...
/// 37: XXX.XX..
/// 38: XXX.XXX.
/// 39: XXX.XXXX
/// 40: XXX..XXX
/// 41: XXX...XX
/// 42: XXX....X

/// If the total is > 43, then we draw 43...48 as:

/// 43: XXXX.X..
/// 44: XXXX.XX.
/// 45: XXXX.XXX
/// 46: XXXX..XX
/// 47: XXXX...X

/// If the total is > 48, then we draw 48...50 as:

/// 48: XXXXX.X.
/// 49: XXXXX.XX
/// 50: XXXXX..X

/// If the total is > 51, then we draw 51 as:

/// 51: XXXXXX.X

void drawRange(uint8_t *mat, uint8_t x, uint8_t y, uint8_t total, uint8_t val)
    {
    if (total > 16)
        {
        if (val == 51)
            {
            for(uint8_t i = 0; i < 6; i++)
                {
                setPoint(mat, i + x, y);
                }
            setPoint(mat, 7 + x, y);
            }
        else if (val >= 48)
            {
            for(uint8_t i = 0; i < 5; i++)
                {
                setPoint(mat, i + x, y);
                }
            drawRange(mat, x + 6, y, 4, val - 48);
            }
        else if (val >= 43)
            {
            for(uint8_t i = 0; i < 4; i++)
                {
                setPoint(mat, i + x, y);
                }
            drawRange(mat, x + 5, y, 6, val - 43);
            }
        else if (val >= 36)
            {
            for(uint8_t i = 0; i < 3; i++)
                {
                setPoint(mat, i + x, y);
                }
            drawRange(mat, x + 4, y, 8, val - 36);
            }
        else if (val >= 27)
            {
            setPoint(mat, 0 + x, y);
            setPoint(mat, 1 + x, y);
            drawRange(mat, x + 3, y, 10, val - 27);
            }
        else if (val >= 16)
            {
            setPoint(mat, 0 + x, y);        
            drawRange(mat, x + 2, y, 12, val - 16);
            }
        else
            {
            drawRange(mat, x, y, 16, val);
            }
        }
    else
        {
        uint8_t halftotal = (total >> 1);
        if (val < halftotal)
            {
            for(uint8_t i = 0; i < val + 1; i++)
                {
                setPoint(mat, i + x, y);
                }
            }
        else
            {
            for(uint8_t i = val - halftotal + 1; i < halftotal; i++)
                setPoint(mat, i + x, y);
            } 
        }
    }


// Prints a note to the matrix plus a range underneath indicating the octave.
void writeNotePitch(unsigned char* mat, unsigned char note)
    {
    uint16_t octave = div12(note);
    uint16_t n = DIV12_REMAINDER(octave, note);
    writeNote(mat, (uint8_t) n);
    
    // though we only have 11 octaves, we specify
    // 12 here so it divides by 2 nicely
    
    drawRange(mat, 0, 1, 12, (uint8_t) octave);
    }

// Prints a note to one matrix and its octave to the other
void writeNotePitchLong(unsigned char* mat, unsigned char* mat2, unsigned char note)
    {
    uint16_t octave = div12(note);
    uint16_t n = DIV12_REMAINDER(octave, note);
    writeNote(mat, (uint8_t) n);
    writeShortNumber(mat2, octave, true);
    }




// Prints a note speed to the matrix.  
// If the value is greater than DOUBLE_WHOLE (11), 
// Nothing is printed and this function silently fails.
void writeNoteSpeed(unsigned char* mat, uint8_t val)
    {
    if (val > NOTE_SPEED_DOUBLE_WHOLE) return;
    memcpy_P(mat, font_8x5[NOTE_SPEED_EIGHTH_TRIPLET + val], 8);
    }

// The buffer for scrolling or displaying 
GLOBAL static unsigned char buffer[MAX_BUFFER_LENGTH];
// Current length of bytes in the buffer
GLOBAL static uint8_t bufferLength = 0;
// The position which corresponds to column zero of the RIGHT MATRIX
GLOBAL static int8_t bufferPos = 0;
// How long we need to wait before doing our next scroll increment
GLOBAL static uint16_t scrollDelay = 0;
// How long we should wait the very first time before beginning to scroll
GLOBAL static uint16_t firstScrollDelay = 256;
// How long we should wait AFTER the very first time to do further scrolls
GLOBAL static uint16_t defaultScrollDelay = 256;
// Have we begun scrolling?
GLOBAL static uint8_t scrollStarted = 0;


// Resets the currently-displaying portion of the scroll to 0
void resetBuffer(uint8_t doFirstDelay) { bufferPos = 0; scrollDelay = (doFirstDelay? firstScrollDelay : defaultScrollDelay); scrollStarted = 0;}


// Clears the scroll, resets its current length to 0, and resets
// the currently-displaying portion to position 0
void clearBuffer() { resetBuffer(1); bufferLength = 0; }

// Set the very *first* delay for which the scroll is written,
// and the *default* delay used thereafter.
// Delays are measured in 32/3125 ~ 1/100 second (because a display happens only once ever 32 ticks, 
// and a tick is 1/3125 sec)
// If a delay is NO_SCROLLING, then this is an infinite delay
void setScrollDelays(uint16_t firstDelay, uint16_t defaultDelay) { firstScrollDelay = firstDelay; defaultScrollDelay = defaultDelay; }

// Returns the total buffer length at present
uint8_t getBufferLength() { return bufferLength; }


void writeToMatrix(unsigned char* mat, int8_t _bufferPos)
    {
    if (mat == NULL) return;
    int8_t len;
    int8_t offset = 0;
    if (_bufferPos >= 0)
        {
        len = bufferLength - _bufferPos;
        if (len > 8) len = 8;
        }
    else
        {
        offset = -_bufferPos;
        if (offset > LED_WIDTH) offset = LED_WIDTH;
        len = LED_WIDTH - offset;
        }
    if (len > 0) 
        {
        memcpy(&mat[offset], &buffer[_bufferPos + offset], (uint8_t) len);
        }
    }

// Writes the currently-displaying portion of the scroll to the two matrcies,
// then if the scroll delay has counted down, prepares to scroll one column
// next time.
//
// Returns:
// NOT_STARTED  if displayed but NOT incremented afterwards, and we've not begun scrolling
// NOT_SCROLLED if displayed but NOT incremented afterwards
// SCROLLED     if displayed and incremented afterwards
// SCROLL_DONE  if displayed and incremented afterwards, and we have completed
//               the scroll and will next start a new scroll.
uint8_t scrollBuffer(unsigned char* mat, unsigned char* mat2, unsigned char* mat3 = NULL, unsigned char* mat4 = NULL)
    {
    // bufferPos refers to the location of the buffer with respect to column zero
    // in the LEFT LED (mat2)
    
//#ifdef TWO_SCREENS_VERTICAL
//    writeToMatrix(mat4, bufferPos);
//    writeToMatrix(mat3, bufferPos + LED_WIDTH);
//    writeToMatrix(mat2, bufferPos + LED_WIDTH * 2);
//    writeToMatrix(mat, bufferPos + LED_WIDTH * 3);
//#else
    writeToMatrix(mat2, bufferPos);
    writeToMatrix(mat, bufferPos + LED_WIDTH);
//#endif TWO_SCREENS_VERTICAL
        
    // Do we shift next time?
    if (scrollDelay < NO_SCROLLING)              // Maybe scroll
        {
        if (scrollDelay > 0)                   // Don't scroll until our countdown is complete
            {
            scrollDelay--;
            return (scrollStarted? NOT_SCROLLED : NOT_STARTED);  // don't update the buffer
            }
        }
    else return (scrollStarted? NOT_SCROLLED : NOT_STARTED);  // never update the buffer
    
    // should we bother scrolling at all?
    if (bufferLength <= LED_WIDTH * 2)
        return SCROLL_DONE;
    
    // At this point we will scroll
    scrollDelay = defaultScrollDelay;
    scrollStarted = 1;
    bufferPos++;  
    if (bufferPos >= bufferLength)
        { bufferPos = -(LED_WIDTH * 2); return SCROLL_DONE; }
    return SCROLLED;
    }


uint8_t scrollBuffer(unsigned char* mat, unsigned char* mat2)
    {
    return scrollBuffer(mat, mat2, NULL, NULL);
    }



// Prints as much of the buffer as can be printed, left-justified, to the matrices
// mat1 and mat2.
void writeBuffer(unsigned char* mat1, unsigned char* mat2)
    {
    uint8_t len = bufferLength;
    if (len > 0)
        {
#if defined(SCREEN_TYPE_ADAFRUIT_16x8_BACKPACK) || defined(SCREEN_TYPE_ADAFRUIT_16x8_FEATHERWING_BACKPACK)
        memcpy(mat2, buffer, (len > LED_WIDTH ? LED_WIDTH : len));
        len -= LED_WIDTH;
        if (len > 0)
            memcpy(mat1, buffer + LED_WIDTH, (len > LED_WIDTH ? LED_WIDTH : len));
#else
        memcpy(mat1, buffer, (len > LED_WIDTH ? LED_WIDTH : len));
#endif
        }
    }



// Adds the given glyph to the scroll, of the provided width, and adding
// some number of spaces (single empty columns) afterwards.
// Does NOT add initial padding to right justify when buffer is reset
void addGlyphToBuffer(const char* glyph, uint8_t width, uint8_t forScrolling, uint8_t progmem)
    {
    if (forScrolling)
        {
        if (bufferLength == 0)
            {
            // add padding
            memset(buffer + bufferLength, 0, (LED_WIDTH - width));
            bufferLength += (LED_WIDTH - width);
            }
        else
            {
            // add two spaces
            memset(buffer + bufferLength, 0, 2);
            bufferLength += 2;
            }
        }
    else if (bufferLength > 0)
        {
        // add a single space
        buffer[bufferLength++] = 0; 
        }
        
    // copy glyph to buffer
    if (progmem) 
        memcpy_P(buffer + bufferLength, glyph, width);
    else
        memcpy(buffer + bufferLength, glyph, width);
    bufferLength += width;
    }



// Loads into the buffer the given string
// A string can have any of a-z or A-Z (which are always entered as A-Z),
// 0-9, hyphen, space, parentheses, or STRING_UP or STRING_DOWN (^ or &),
// which are interpreted as up arrows or down arrows.
// Adds initial padding to right-justify when buffer is reset
// Automatically adds two spaces after any characters
void addToBuffer(const char* val, uint8_t extra) //  = 0)
    {
    uint8_t padded = 0;
    if (bufferLength == 0)
        {
        uint8_t maxLen = LED_WIDTH * 2 - extra;
        
        // compute padding
        
        uint8_t len = 0;
        uint8_t lastWasSpace;
        for(uint8_t i = 0; val[i] != '\0'; i++)
            {
            char c = val[i];
            if (c <= 'z' && c >= 'a')
                c = c - 'a' + 'A';

            if (c==' ') 
                {
                if (len + 1 <= maxLen)
                    {
                    len += 2;
                    lastWasSpace = 1;
                    }
                else break;
                }
            else 
                {
                if (c == 'M' || c == 'W' || c == 'N' || c == 'Q' || c == 'V' ) // 5-pixel wide letters
                    {
                    if (len + 5 <= maxLen)
                        len += 6;
                    else break;
                    }
                /*
                  else if (c == '1') // || c == '(' || c == ')')                // 2-pixel thin letters
                  {
                  if (len + 2 <= maxLen)
                  len += 3;
                  else break;
                  }
                */
                else                                                                                    // everything else is 3 pixels
                    {
                    if (len + 3 <= maxLen)
                        len += 4;
                    else break;
                    }
                lastWasSpace = 0;
                }
            }

        if (lastWasSpace) len -= 2;                     
        if (len > 0) len--;
                
        memset(buffer + bufferLength, 0, (LED_WIDTH * 2 - len));
        bufferLength += (LED_WIDTH * 2 - len);
        if (bufferLength > 0) padded = 1;
        }
      
    for(uint8_t i = 0; ; i++)
        {
        char c = val[i];
        
        // handle \0 specially
        if (c == 0)  // End of string
            {
            return;
            }
        
        // add a space 
        else if (i > 0 || (bufferLength > 0 && !padded)) 
            {
            buffer[bufferLength] = 0;
            bufferLength++;
            padded = 0;
            }
            
        if (c <= 'z' && c >= 'a')
            c = c - 'a' + 'A';

        if (c == 'M' || c == 'W' || c == 'N' || c == 'Q' || c == 'V' )   // wide letters
            {
            memcpy_P(buffer + bufferLength, font_5x5[
                    (c == 'M') ? 0 :
                    ((c == 'N') ? 1 :
                        ((c == 'Q') ? 2 :
                        ((c == 'V') ? 3 : 4)))
                    ], 5);
            bufferLength += 5;
            }
        else if (c >= 'A' && c <= ']')  // a letter or FLAT, UPDOWN, or BLANK
            {
            //uint8_t len = strnlen_P(font_alphabet5x5[c - 'A'], 5);
            memcpy_P(buffer + bufferLength, font_3x5[c - 'A' + GLYPH_3x5_A], 3);
            bufferLength += 3;
            }
        else if (c == ' ')  // spacebar
            {
            buffer[bufferLength] = 0;
            bufferLength += 1;
            }
        /*
          else if (c == '1')  // number
          {
          memcpy_P(buffer + bufferLength, font_2x5[GLYPH_2x5_1], 2);
          bufferLength += 2;
          }
          else if (c == '(' | c == ')')
          {
          memcpy_P(buffer + bufferLength, font_2x5[GLYPH_2x5_LEFT_PAREN + c - '('], 2);
          bufferLength += 2;
          }
        */
        else 
            {
            if (c >= '(' && c <= '9')   // one of ( ) UP PLUS DOWN MINUS PERIOD SLASH 0 1 2 3 4 5 6 7 8 9
                memcpy_P(buffer + bufferLength, font_3x5[c - '(' + GLYPH_2x5_LEFT_PAREN], 3);
            /*
              if (c >= '0' && c <= '9')  // number
              {
              memcpy_P(buffer + bufferLength, font_3x5[c - '0' + GLYPH_3x5_0], 3);
              }
              else if (c == '-')
              {
              memcpy_P(buffer + bufferLength, font_3x5[GLYPH_3x5_MINUS], 3);
              }
              else if (c >= CHAR_UP && c <= CHAR_UPDOWN)
              {
              memcpy_P(buffer + bufferLength, font_3x5[c - CHAR_UP + GLYPH_3x5_UP], 3);
              }
              // This is the character for UP
              else if (c == CHAR_UP)
              {
              memcpy_P(buffer + bufferLength, font_3x5[GLYPH_3x5_UP], 3);
              }
              // This is the character for DOWN (because it's next to UP)
              else if (c == CHAR_DOWN)
              {
              memcpy_P(buffer + bufferLength, font_3x5[GLYPH_3x5_DOWN], 3);
              }
              // This is the character for UPDOWN (because it's next to UP)
              else if (c == CHAR_UPDOWN)
              {
              memcpy_P(buffer + bufferLength, font_3x5[GLYPH_3x5_UP_DOWN], 3);
              }
            */
            bufferLength += 3;
            }
        } 
    }



///// SCREEN BRIGHTNESS

void _setScreenBrightness(uint8_t brightness, uint8_t i2cAddress)
    {
    Wire.beginTransmission(i2cAddress);
    Wire.write(LED_BRIGHTNESS_I2C | brightness);
    Wire.endTransmissionNonblocking();  
    }

// Sets the screen brightness to a value between 0 (minimum) and 15 (maximum)
// inclusive.
void setScreenBrightness(uint8_t brightness)
    {
    if (brightness > 15) return;
    
#ifdef TWO_SCREENS_VERTICAL
#ifdef SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
    _setScreenBrightness(brightness, I2C_ADDRESS_1);
    _setScreenBrightness(brightness, I2C_ADDRESS_2);
    _setScreenBrightness(brightness, I2C_ADDRESS_3);
    _setScreenBrightness(brightness, I2C_ADDRESS_4);
#else
    _setScreenBrightness(brightness, I2C_ADDRESS_1);
    _setScreenBrightness(brightness, I2C_ADDRESS_3);
#endif SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
#else
#ifdef SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
    _setScreenBrightness(brightness, I2C_ADDRESS_1);
    _setScreenBrightness(brightness, I2C_ADDRESS_2);
#else
#endif SCREEN_TYPE_TWO_ADAFRUIT_8x8_BACKPACKS
    _setScreenBrightness(brightness, I2C_ADDRESS_1);
#endif TWO_SCREENS_VERTICAL

    }
