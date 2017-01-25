////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __CONTROL_H__
#define __CONTROL_H__




/////// THE CONTROLLER
//
// The controller can do the following:
//
// 1. Map the two pots and the select and middle buttons to Program Change, any Channel Control, any NRPN, any RPN, or OFF (do nothing).
//
// 2. If VOLTAGE is #defined on, you can also map to one of two 5V DACs for voltage output.
//
// 3. Buttons can be mapped to provide a VALUE, to INCREMENT, or to NOT DO ANYTHING when they are pushed or released.
//    Buttons at present cannot send DECREMENT messages. [INCREMENT is only available for NRPN or RPN].
//
// The controller CANNOT send 14-bit RPN/NRPN values.  This is a restriction of the pots: though they have a resolution of 1024, they
// have an EFFECTIVE resolution, realistically, of about 256 due to noise.  Thus we only send 7-bit RPN/NRPN values (that is, MSB only).
// 
//
// NOTE
// 
// The most important function (STATE_CONTROLLER_PLAY) is inlined into the state machine in
// TopLevel.cpp to save code space.
//
// OPTIONS
//
// Permanent options special to the Controller are:
//
// options.leftKnobControlType                          CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN, [_VOLTAGE_A, and _VOLTAGE_B]
// options.rightKnobControlType                         CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN, [_VOLTAGE_A, and _VOLTAGE_B]
// options.middleButtonControlType                      CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN, [_VOLTAGE_A, and _VOLTAGE_B]
// options.selectButtonControlType                      CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN, [_VOLTAGE_A, and _VOLTAGE_B]
// options.leftKnobControlNumber                        Parameter number (CC, NRPN, and RPN only).  NRPN and RPN can be 14-bit
// options.rightKnobControlNumber                       Parameter number (CC, NRPN, and RPN only).  NRPN and RPN can be 14-bit
// options.middleButtonControlNumber            Parameter number (CC, NRPN, and RPN only).  NRPN and RPN can be 14-bit
// options.selectButtonControlNumber            Parameter number (CC, NRPN, and RPN only).  NRPN and RPN can be 14-bit
// options.middleButtonControlOn                        Value sent when middle button is pushed.  If 0, it's off (nothing sent).  If 0<n<129, then n-1 is sent.  If 129, then INCREMENT is sent.
// options.selectButtonControlOn                        Value sent when select button is pushed.  If 0, it's off (nothing sent).  If 0<n<129, then n-1 is sent.  If 129, then INCREMENT is sent.
// options.middleButtonControlOff                       Value sent when middle button is pushed.  If 0, it's off (nothing sent).  If 0<n<129, then n-1 is sent.  If 129, then INCREMENT is sent.
// options.selectButtonControlOff                       Value sent when select button is pushed.  If 0, it's off (nothing sent).  If 0<n<129, then n-1 is sent.  If 129, then INCREMENT is sent.
//
// Other permanent options affecting the Arpeggiator include:
//
// options.channelOut
//
//
// DISPLAY
// 
// As you play or record notes, a cursor moves across the screen to register NOTE ON messages.  With 64 messages, the
// cursor pass through the top four rows.  The next two rows are reserved for another cursor indicating the current
// measure.
//
//
// INTERFACE
//
// Root
//      Controller                              STATE_CONTROLLER
//              Go              STATE_CONTROLLER_PLAY
//              Left Knob
//                      Off
//                      CC
//                              Parameter Number to send
//                      NRPN
//                              Parameter Number to send
//                      RPN
//                              Parameter Number to send
//                      PC
//                      A VOLTAGE               [Only when voltage turned on]
//                      B VOLTAGE               [Only when voltage turned on]
//              Right Knob
//                      Off
//                      CC
//                              Parameter Number to send
//                      NRPN
//                              Parameter Number to send
//                      RPN
//                              Parameter Number to send
//                      PC
//                      A VOLTAGE               [Only when voltage turned on]
//                      B VOLTAGE               [Only when voltage turned on]
//              Middle Button
//                      Off
//                      CC
//                              Parameter Number to send
//                                      Button On Value to send [or OFF]
//                                              Button Off Value to send [or OFF]
//                      NRPN
//                              Parameter Number to send
//                                      Button On Value to send [or OFF, or Increment]
//                                              Button Off Value to send [or OFF, or Increment]
//                      RPN
//                              Parameter Number to send
//                                      Button On Value to send [or OFF, or Increment]
//                                              Button Off Value to send [or OFF, or Increment]
//                      PC
//                              Button On Value to send [or OFF]
//                                      Button Off Value to send [or OFF]
//                      A VOLTAGE               [Only when voltage turned on]
//                              Button On Value to send [or OFF]
//                                      Button Off Value to send [or OFF]
//                      B VOLTAGE               [Only when voltage turned on]
//                              Button On Value to send [or OFF]
//                                      Button Off Value to send [or OFF]
//              Right Button
//                      Off
//                      CC
//                              Parameter Number to send
//                                      Button On Value to send [or OFF]
//                                              Button Off Value to send [or OFF]
//                      NRPN
//                              Parameter Number to send
//                                      Button On Value to send [or OFF, or Increment]
//                                              Button Off Value to send [or OFF, or Increment]
//                      RPN
//                              Parameter Number to send
//                                      Button On Value to send [or OFF, or Increment]
//                                              Button Off Value to send [or OFF, or Increment]
//                      PC
//                              Button On Value to send [or OFF]
//                                      Button Off Value to send [or OFF]
//                      A VOLTAGE               [Only when voltage turned on]
//                              Button On Value to send [or OFF]
//                                      Button Off Value to send [or OFF]
//                      B VOLTAGE               [Only when voltage turned on]
//                              Button On Value to send [or OFF]
//                                      Button Off Value to send [or OFF]




#include "TopLevel.h"
#include <Arduino.h>

// These are the values that can be used in:
// options.middleButtonControlOn, options.middleButtonControlOff
// options.selectButtonControlOn, options.selectButtonControlOff
// options.leftKnobControlType
// options.rightKnobControlType

#define CONTROL_TYPE_OFF 0
#define CONTROL_TYPE_CC 1
#define CONTROL_TYPE_NRPN 2
#define CONTROL_TYPE_RPN 3
#define CONTROL_TYPE_PC 4
#if defined(__MEGA__)
#define CONTROL_TYPE_PITCH_BEND 5
#define CONTROL_TYPE_AFTERTOUCH 6
#define CONTROL_TYPE_VOLTAGE_A 7
#define CONTROL_TYPE_VOLTAGE_B 8
#endif

// numerical value that means a button isn't sending a value but is rather incrementing 
#define CONTROL_VALUE_INCREMENT 128
#define CONTROL_VALUE_DECREMENT 129
#define MAXIMUM_PC_VALUE 127

struct _controlLocal
    {
    int16_t displayValue;
    uint8_t middleButtonToggle;
    uint8_t selectButtonToggle;  // perhaps these two could be compressed, they're just booleans
    uint8_t doIncrement; 		// This is set to 1 when we're doing NRPN or RPN, and doing buttons and need to display increment as an option
    uint8_t displayType;
    };


// SEND CONTROLLER COMMAND
/// Sends a controller command when the user modifies a button or pot.  Command types can be any
/// of the CONTROL_TYPE_* constants above.  RPN and NRPN send 14-bit, so if you're sending a 
// 7-bit value, shift it left by 7 first.  RPN and NRPN have 14-bit command numbers of course.
// CC permits command numbers 0...119.  PC has no command number.  PC and CC have command values 
// 0...127.   Also sending to VOLTAGE assumes you're providing a value 0...1023, and the command 
// number is ignored.
void sendControllerCommand(uint8_t commandType, uint16_t commandNumber, uint16_t fullValue);

// SET CONTROLLER TYPE
// Lets the user set a controller type.   This is stored in &type.  When the user is finished
// this function will go to the provided nextState (typically to set the controller number).
void setControllerType(uint8_t &type, uint8_t nextState, uint8_t buttonOnState);

// SET CONTROLLER NUMBER
// Lets the user set a controller number for the given conroller type.   
// This is stored in &number.  The user can cancel everything and the type and
// number will be reset.
void setControllerNumber(uint8_t type, uint16_t &number, uint8_t backupType, uint16_t backupNumber, uint8_t nextState);


// SET CONTROLLER BUTTON ON OFF
// Lets the user specify a control value to be sent when the button is pressed on (or off).  This
// value is stored in onOff and will be restored if the user cancels everything.
void setControllerButtonOnOff(uint16_t &onOff, int8_t nextState);

#endif

