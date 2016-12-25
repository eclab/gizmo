////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License







#ifndef __LED_DISPLAY_H_
#define __LED_DISPLAY_H_


#include <Arduino.h>



//// INTRODUCTION
////
//// This library is designed to do write various graphics to a single 8x8 LED matrix
//// or to two 8x8 matrices arranged horizontally.  Unlike existing libraries (such as
//// Adafruit's otherwise cutely-designed AdafruitGFX package), this library is meant
//// to do so RAPIDLY.



///// WHAT TYPE OF SCREEN ARE WE USING?

#define SCREEN_TYPE_ADAFRUIT_16x8_BACKPACK

// We no longer support the following two options, though we have some gunk in the cpp file
// regarding them.
//#define SCREEN_TYPE_ADAFRUIT_8x8_BACKPACK
//#define SCREEN_TYPE_SPARKFUN_8x8_KIT



// SCROLLING DELAY

// Lots of text scrolls in this system.  The INITIAL delay before scrolling starts is
// FIRST_SCROLL_DELAY, that is after 1 second (3125 / 4 = 781 ticks).  Thereafter the DEFAULT
// delay before the next column is scrolled in is 60ms, thus about 13 ticks.
#define SCROLL_DEFAULT_DELAY 26
#define NO_SCROLLING 65535                      // never scroll




/// LED Constants
#define LED_WIDTH 8
#define LED_HEIGHT 8


/// LED Pins for the SparkFun 8x8 Kit
#define PIN_LED_CLK 10  
#define PIN_LED_CS 9
#define PIN_LED_DIN 8








//// FIXED WIDTH GLYPHS
////
//// Fixed with glyphs come in 5 widths: 2, 3, 4, 5, and 8.
////
//// These glyphs are used as follows:
////
//// WIDTH 2:   1 2 3 5 C E F L S ( )
//// 1 is its standard form.  2, 3, 5, C, E, F, L, S are "skinny" versions useful for
//// squeezing into a tighter space.  The parentheses ( and ) are the standard forms
//// of these characters.
////
//// WIDTH 3:   0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
////            MINUS FLAT(b) UP DOWN UPDOWN
//// The numbers 0...9 an letters A...F are standard forms when used in numerical
//// or hex form.  We include 1 to make it easy to use in combinations with the other
//// numbers.  The letters A...Z are narrow 3x5 versions of these letters, such that four
//// can be placed in a 16x5 row.  We also use these letters for scrolling, except for 
//// M N Q V W, which are basically unreadable 3x5, and so we use width 5 versions of 
//// those instead. We also have MINUS (-) and FLAT (b), as well as up, down, and up-down
//// arrows.
////
//// WIDTH 4:   10 11 12 13 14 15 16 17 18 19 -1 -2 -3 -4 -5 -6 -7 -8 -9
//// 10...19 are "squished" versions of these numbers intended to fit in the far left
//// columns of a number.  For example, on an 8x8 display, we can use 12 plus 7 to make
//// 127.  On a 16x8 display, we could use 16 plus 355 to make 16355. The numbers -1...-9
//// are also "squished" versions of these numbers, again intended to fit in the far left
//// columns of a number.  For example, on an 8x8 display, we can use -4 plus 5 to make -45.
//// On a 16x8 display we can use -7 plus 925 to make -7925.
////
//// WIDTH 5: M N Q V W
//// These five letters look terrible in 3x5, so we have 5x5 versions of them which look good.
//// We use these particularly in scrolling.
////
//// WIDTH 8:   EIGHTH TRIPLET (1/96 note), QUARTER TRIPLET (1/48 note), THIRTY-SECOND NOTE, 
////            HALF TRIPLET (1/24 note), SIXTEENTH NOTE, TRIPLET (1/12 note), EIGHTH NOTE, 
////            QUARTER NOTE, QUARTER NOTE TIED TO TRIPLET, DOTTED QUARTER NOTE, HALF NOTE,
////            HALF NOTE TIED TO TWO TRIPLETS, DOTTED HALF NOTE, WHOLE NOTE, DOTTED WHOLE NOTE,
////            DOUBLE WHOLE NOTE, INFINITY, 1/8, 1/6, 1/4, 1/3, 1/2
////
//// We provide 16 different kinds of note symbols.  We also provide the infinity symbol,
//// plus five common fractions.


// Emacs indentation has a bug in whitesmiths.  This semicolon
// handles it sort of
;

/// GLYPH NAMES
/// This doesn't include the alphabet in font_alphabet5x5 

//#define GLYPH_2x5_1             0

#define GLYPH_2x5_LEFT_PAREN    0
#define GLYPH_2x5_RIGHT_PAREN   1
#define GLYPH_3x5_UP            2               // associated with character '*'
#define GLYPH_3x5_PLUS          3
#define GLYPH_3x5_DOWN          4               // associated with character ','
#define GLYPH_3x5_MINUS         5
#define GLYPH_3x5_PERIOD        6
#define GLYPH_3x5_UP_DOWN       7               // associated with character '/'
#define GLYPH_3x5_0             8
#define GLYPH_3x5_1             9
#define GLYPH_3x5_2             10
#define GLYPH_3x5_3             11
#define GLYPH_3x5_4             12
#define GLYPH_3x5_5             13
#define GLYPH_3x5_6             14
#define GLYPH_3x5_7             15
#define GLYPH_3x5_8             16
#define GLYPH_3x5_9             17
#define GLYPH_3x5_A             18
#define GLYPH_3x5_B             19
#define GLYPH_3x5_C             20
#define GLYPH_3x5_D             21
#define GLYPH_3x5_E             22
#define GLYPH_3x5_F             23
#define GLYPH_3x5_G             24
#define GLYPH_3x5_H             25
#define GLYPH_3x5_I             26
#define GLYPH_3x5_J             27
#define GLYPH_3x5_K             28
#define GLYPH_3x5_L             29
#define GLYPH_3x5_M             30
#define GLYPH_3x5_N             31
#define GLYPH_3x5_O             32
#define GLYPH_3x5_P             33
#define GLYPH_3x5_Q             34
#define GLYPH_3x5_R             35
#define GLYPH_3x5_S             36
#define GLYPH_3x5_T             37
#define GLYPH_3x5_U             38
#define GLYPH_3x5_V             39
#define GLYPH_3x5_W             40
#define GLYPH_3x5_X             41
#define GLYPH_3x5_Y             42
#define GLYPH_3x5_Z             43
#define GLYPH_3x5_FLAT          44
#define GLYPH_3x5_BLANK         45


#define GLYPH_4x5_10    0
#define GLYPH_4x5_11    1
#define GLYPH_4x5_12    2
#define GLYPH_4x5_13    3
#define GLYPH_4x5_14    4
#define GLYPH_4x5_15    5
#define GLYPH_4x5_16    6
#define GLYPH_4x5_17    7
#define GLYPH_4x5_18    8
#define GLYPH_4x5_19    9
#define GLYPH_4x5_NEGATIVE_1    10
#define GLYPH_4x5_NEGATIVE_2    11
#define GLYPH_4x5_NEGATIVE_3    12
#define GLYPH_4x5_NEGATIVE_4    13
#define GLYPH_4x5_NEGATIVE_5    14
#define GLYPH_4x5_NEGATIVE_6    15
#define GLYPH_4x5_NEGATIVE_7    16
#define GLYPH_4x5_NEGATIVE_8    17
#define GLYPH_4x5_NEGATIVE_9    18


#define GLYPH_8x5_EIGHTH_TRIPLET        0
#define GLYPH_8x5_QUARTER_TRIPLET       1
#define GLYPH_8x5_THIRTY_SECOND 2
#define GLYPH_8x5_HALF_TRIPLET  3
#define GLYPH_8x5_SIXTEENTH     4
#define GLYPH_8x5_TRIPLET       5
#define GLYPH_8x5_EIGHTH                6
#define GLYPH_8x5_QUARTER       7
#define GLYPH_8x5_QUARTER_TIED_TO_TRIPLET       8
#define GLYPH_8x5_DOTTED_QUARTER        9
#define GLYPH_8x5_HALF  10
#define GLYPH_8x5_HALF_TIED_TO_TWO_TRIPLETS     11
#define GLYPH_8x5_DOTTED_HALF   12
#define GLYPH_8x5_WHOLE 13
#define GLYPH_8x5_DOTTED_WHOLE  14
#define GLYPH_8x5_DOUBLE_WHOLE  15
#define GLYPH_8x5_INFINITY              16
#define GLYPH_8x5_ONE_EIGHTH    17
#define GLYPH_8x5_ONE_SIXTH             18
#define GLYPH_8x5_ONE_FOURTH    19
#define GLYPH_8x5_ONE_THIRD             20
#define GLYPH_8x5_ONE_HALF              21
#define GLYPH_8x5_SURE_PT1                              22
#define GLYPH_8x5_SURE_PT2                              23
#define GLYPH_8x5_GIZMO_PT1                             24
#define GLYPH_8x5_GIZMO_PT2                             25



///// CHARACTER REPRESENTATIONS OF GLYPHS
///// These are used in strings 


#define STR_UP "*"
#define STR_DOWN ","
#define STR_UP_DOWN "/"


//// OUR FONTS

//extern const char PROGMEM font_1x5[3][1];
//extern const char PROGMEM font_2x5[1][2];
extern const char PROGMEM font_3x5[46][3];
extern const char PROGMEM font_4x5[19][4];
extern const char PROGMEM font_5x5[5][5];
//extern const char PROGMEM font_alphabet5x5[26][5];
extern const char PROGMEM font_8x5[26][8];



// Initializes the LED.  Call this in setup()
void initLED(void);

// Rotation Directions

#define DIR_NONE (0)
#define DIR_CLOCKWISE_90 (1)
#define DIR_180 (2)
#define DIR_COUNTERCLOCKWISE_90 (3)

// Sets the rotation of the scren.  For a single 8x8 matrix, all four rotations
// make sense.  For a 16x8 matrix, only DIR_180 and DIR_NONE make sense; other
// rotations will be ignored.
void setRotation(uint8_t dir);

// Sends one or more matrices to the LED
// matrix2 can be NULL only if we're using the 8x8 screens
void sendMatrix(unsigned char* matrix, unsigned char* matrix2);

// Rotates a matrix in the given direction.
void rotateMatrix(unsigned char* in, uint8_t dir);

// Inverts a matrix (light becomes unlit and vice versa)
void invertMatrix(unsigned char* mat);

// Clears a matrix (sets all to unlit)
void clearMatrix(unsigned char* mat);

// Sets a point (to lit)
void setPoint(unsigned char* mat, uint8_t x, uint8_t y);

// Sets a point to unlit
unsigned char* clearPoint(unsigned char* mat, uint8_t x, uint8_t y);

// Sets the number of send(...) messages which pass before
// blink elements blink on and off respectively.  
// Note that blinkOn must be < blink off.
void setBlinkOnOff(uint8_t on, uint8_t off);

// Sets a point to blink.  This works as follows.  Each time sendMatrix is called,
// it increments a counter.  blinkPoint uses this counter to determine, based on the
// duty cycle above, whether to XOR the point or to not touch it.  
void blinkPoint(unsigned char* mat, uint8_t x, uint8_t y);

// Use this to create lines of lengths from 1...7.
// A line of length 8 is of course 256.
// You may wish to shift the line as well, in which case you can 
// use LINE_OF_LENGTH(x) << 3 or whatnot
#define LINE_OF_LENGTH(x) ((((unsigned char)0x1) << x) - 1)

// Sets a vertical line (to lit)
void setVerticalLine(unsigned char* mat, uint8_t x, unsigned char line);

// Clears a vertical line (sets it to unlit)
void clearVerticalLine(unsigned char* mat, uint8_t x, unsigned char line);

// Blinks a vertical line
void blinkVerticalLine(unsigned char* mat, uint8_t x, unsigned char line);

// Writes a 2x5 glyph of the given WIDTH, starting at column X (which may be a value 0...6)
//void write2x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x);

// Writes a 3x5 glyph of the given WIDTH, starting at column X (which may be a value 0...5)
void write3x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x);

// Writes a 4x5 glyph of the given WIDTH, starting at column X (which may be a value 0...3)
void write4x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x);

// Writes a 5x5 glyph of the given WIDTH, starting at column X (which may be a value 0...2)
void write5x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x);

// Writes an 8x5 glyph of the given WIDTH to the matrix MAT
void write8x5Glyph(unsigned char* mat, uint8_t glyph);

void blink3x5Glyph(unsigned char* mat, uint8_t glyph, uint8_t x);

// Prints a number from -99 to 127 inclusive to the matrix.
// If the number is outside this range, nothing will be printed and this
// function will silently fail.
//
// If leftJustify is TRUE then numbers -9...99 will be shifted to the
// left by one pixel.  The point of this is to make a little room between
// this number and the number to its right if we're writing two numbers.
void writeShortNumber(unsigned char* mat1, int8_t val, uint8_t leftJustify);

// Writes a number (-9999...19999) to two matrices.
// The numbers are all written straight [though squished]
void writeNumber(unsigned char* mat1, unsigned char* mat2, int16_t val);

// Writes a number (-9999...19999) as a string to the given buffer (which must be 6 in size)
void numberToString(char* buffer, int16_t val);

// Writes a hex number (00...FF) to the matrix
void writeHex(unsigned char* mat, unsigned char val);

// Note values
#define NOTE_C 0
#define NOTE_Db 1
#define NOTE_D 2
#define NOTE_Eb 3
#define NOTE_E 4
#define NOTE_F 5
#define NOTE_Gb 6
#define NOTE_G 7
#define NOTE_Ab 8
#define NOTE_A 9
#define NOTE_Bb 10
#define NOTE_B 11

// Prints a note to the matrix.  If the note is
// not in the range NOTE_C ... NOTE_B  (0 ... 11), nothing
// is printed and this function will silently fail.
// Notes are written left justified.
void writeNote(unsigned char* mat, unsigned char note);

// Prints a note to the matrix plus a range underneath indicating the octave.
void writeNotePitch(unsigned char* mat, unsigned char note);

// Note speed values
#define NOTE_SPEED_EIGHTH_TRIPLET 0
#define NOTE_SPEED_QUARTER_TRIPLET 1
#define NOTE_SPEED_THIRTY_SECOND 2
#define NOTE_SPEED_HALF_TRIPLET 3
#define NOTE_SPEED_SIXTEENTH 4
#define NOTE_SPEED_TRIPLET 5
#define NOTE_SPEED_EIGHTH 6
#define NOTE_SPEED_QUARTER 7
#define NOTE_SPEED_QUARTER_TIED_TO_TRIPLET  8
#define NOTE_SPEED_DOTTED_QUARTER 9
#define NOTE_SPEED_HALF 10
#define NOTE_SPEED_HALF_TIED_TO_TWO_TRIPLETS 11
#define NOTE_SPEED_DOTTED_HALF 12
#define NOTE_SPEED_WHOLE 13
#define NOTE_SPEED_DOTTED_WHOLE 14
#define NOTE_SPEED_DOUBLE_WHOLE 15

// Prints a note speed to the matrix.  
// If the value is greater than DOUBLE_WHOLE (11), 
// Nothing is printed and this function silently fails.
void writeNoteSpeed(unsigned char* mat, uint8_t val);

// Scroll buffer maximum length.  This is enough to fill in a string of 28 4-wide chars.
// Maybe this isn't big enough?
#define MAX_BUFFER_LENGTH (28 * 7)


// Clears the scroll, resets its current length to 0, and resets
// the currently-displaying portion to position 0
void clearBuffer();

// Resets the currently-displaying portion of the scroll to 0
void resetBuffer(uint8_t doFirstDelay);



#define NOT_STARTED 0
#define NOT_SCROLLED 1
#define SCROLLED 2
#define SCROLL_DONE 3
 
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
uint8_t scrollBuffer(unsigned char* mat, unsigned char* mat2);

// Set the very *first* delay for which the scroll is written,
// and the *default* delay used thereafter.
void setScrollDelays(uint16_t firstDelay, uint16_t defaultDelay);

// Returns the total buffer length at present
uint8_t getBufferLength();

// Prints as much of the buffer as can be printed, left-justified, to the matrices
// mat1 and mat2.
void writeBuffer(unsigned char* mat1, unsigned char* mat2);

// Loads into the buffer the given string
// A string can have any of a-z or A-Z (which are always entered as A-Z),
// 0-9, hyphen, space, parentheses, or STRING_UP or STRING_DOWN (^ or &),
// which are interpreted as up arrows or down arrows.
// Adds initial padding to right-justify when buffer is reset
// Automatically adds two spaces after any characters
void addToBuffer(const char* val, uint8_t extra = 0);

// Adds the given glyph to the scroll, of the provided width, and adding
// some number of spaces (single empty columns) afterwards.
// Does NOT add initial padding to right justify when buffer is reset
void addGlyphToBuffer(const char* glyph, uint8_t width, uint8_t forScrolling, uint8_t progmem);


#define MAXIMUM_BRIGHTNESS 15
#define MINIMUM_BRIGHTNESS 0

// Sets the screen brightness to a value between 0 (minimum) and 15 (maximum)
// inclusive.
void setScreenBrightness(uint8_t brightness);

/// Draws a series of points horizontally corresponding to a value from 0...(total-1)
/// This series draws to the right, then undraws to the right, and starts at column X
/// in row Y.  Generally, total should be an even number.
///
/// An example.  Let us say that TOTAL is 8.  Then we will draw up to 4 points like this:
/// (an X is a point, a . is an unlit LED)
///
/// 0:  X...
/// 1:  XX..
/// 2:  XXX.
/// 3:  XXXX
///     4:      .XXX
/// 5:  ..XX
/// 6:  ...X
/// 7:  ....
void drawRange(uint8_t *mat, uint8_t x, uint8_t y, uint8_t total, uint8_t val);


#endif




