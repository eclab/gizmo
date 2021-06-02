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
// options.leftKnobControlType                          CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN
// options.rightKnobControlType                         CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN
// options.middleButtonControlType                      CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN
// options.selectButtonControlType                      CONTROL_TYPE_OFF, _PC, _CC, _NRPN, _RPN
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
//              Right Knob
//                      Off
//                      CC
//                              Parameter Number to send
//                      NRPN
//                              Parameter Number to send
//                      RPN
//                              Parameter Number to send
//                      PC
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




#include "TopLevel.h"
#include <Arduino.h>

// numerical value that means a button isn't sending a value but is rather incrementing 
#define MAXIMUM_PC_VALUE 127

struct _controlLocal
    {
    int16_t displayValue;
    uint8_t middleButtonToggle;
    uint8_t selectButtonToggle;  // perhaps these two could be compressed, they're just booleans
    uint8_t doIncrement; 		// This is set to 1 when we're doing NRPN or RPN, and doing buttons and need to display increment as an option
    uint8_t displayType;
    uint8_t learning;
    uint8_t waveEnvelopeIndex;
    int8_t wavePosition;
    uint8_t noteOnCount;
    uint16_t currentWaveControl;
    float fadeWaveControl;		// current wave control so we can set it to start if we're doing FADED
    float fadeStartControl;		// the very last wave control done prior to resetting the index to 0 for FADED 
    uint32_t waveStartTicks;		// also repurposed in stateControllerPCPlayWorking()
    uint32_t waveEndTicks;
    uint8_t waveCountDown;
    int8_t lastWavePosition;
    uint16_t startWaveControl;
    uint16_t endWaveControl;
    uint8_t randomKeyDownOnce;
	uint16_t potUpdateValue[4];
	uint32_t potUpdateTime[4];
	uint8_t potWaiting[4];
	uint8_t pcState;
	uint8_t pcMenu;		// 1 ... 12
	uint8_t pcChannel;	// 1 ... 16
	int8_t pcMSB;		// -1 ... 127
	int8_t pcLSB;		// -1 ... 127
	uint8_t pcStage;
    };

#define PC_STATE_NONE 0
#define PC_STATE_LEFT 1
#define PC_STATE_RIGHT 2
#define PC_STATE_A2 3
#define PC_STATE_A3 4

#define ENVELOPE_MODE_GATED 0
#define ENVELOPE_MODE_FADED 1
#define ENVELOPE_MODE_TRIGGERED 2
#define ENVELOPE_MODE_LOOPED 3
#define ENVELOPE_MODE_FREE 4
#define ENVELOPE_MODE_FAST_FADED 5

#define ENVELOPE_END 255
#define ENVELOPE_SIZE 8

#define MAX_RANDOM_TRIES (6)

#define RANDOM_LENGTH_FOREVER (255)

#define RANDOM_MODE_GATED 0
#define RANDOM_MODE_TRIGGERED 1
#define RANDOM_MODE_NOT_RESET 2
#define RANDOM_MODE_FREE 3
#define RANDOM_MODE_SH_GATED 4
#define RANDOM_MODE_SH_TRIGGERED 5
#define RANDOM_MODE_SH_NOT_RESET 6
#define RANDOM_MODE_SH_FREE 7

#define MAX_32 (4294967296 - 1)

#define NUM_PC_CHANGES (8)
#define NUM_PC_CHANNELS (16)
#define PC_UNSET (-1)
// This is obviously just for the bulk PC changes
struct _controller
    {
    int8_t msb[NUM_PC_CHANGES][NUM_PC_CHANNELS];		// -1 means no bank change
    int8_t lsb[NUM_PC_CHANGES][NUM_PC_CHANNELS];		// -1 means no bank change
    uint8_t pc[NUM_PC_CHANGES][NUM_PC_CHANNELS];
    uint8_t unused[3];
    };



// SET CONTROLLER TYPE
// Lets the user set a controller type.   This is stored in &type.  If the user has selected
// CC, NRPN, or RPN, and thus must specify a parameter number, then we switch to nextState.
// If the user has seleted BEND or AFTERTOUCH and is fromButon, then we switch to nextState.
// Otherwise, we switch to returnState (including OFF and cancelling).
void setControllerType(uint8_t &type, uint8_t nextState, uint8_t returnState, uint8_t fromButton);

// SET CONTROLLER NUMBER
// Lets the user set a controller number for the given controller type, given a button (or not).  This is stored in &number.
// The original type, determined by setControllerType, ise in 'type'.  Backups are provided.
// If the user selects an item, we go to nextState.  If the user cancels, we go to returnState.
void setControllerNumber(uint8_t type, uint16_t &number, uint8_t backupType, uint16_t backupNumber, uint8_t nextState, uint8_t returnState, uint8_t fromButton);


// SET CONTROLLER TYPE
// Lets the user set a controller type.   This is stored in &type.  If the user has selected
// CC, NRPN, or RPN, and thus must specify a parameter number, then we switch to nextState.
// If the user has seleted BEND or AFTERTOUCH and is fromButon, then we switch to nextState.
// Otherwise, we switch to returnState (including OFF and cancelling).
// value is stored in onOff and will be restored if the user cancels everything.
void setControllerButtonOnOff(uint16_t &onOff, int8_t nextState);


#define WAVE_COUNTDOWN (32)
#define MINIMUM_CONTROLLER_POT_DELAY (50000L)	// 50ms
void stateControllerSetWaveEnvelope();
void stateControllerSetWaveEnvelopeValue();
void stateControllerPlayWaveEnvelope();
void stateControllerModulationSetMode();
void stateControllerResetWaveEnvelopeValuesGo();
void stateControllerPlayRandom();
void stateControllerRandomSetMode();
void stateControllerPlay();


void stateControllerPCPlayLoad();
void stateControllerPCPlay();
void stateControllerPCPlayWorking();
void stateControllerPCEditLoad();
void stateControllerPCEdit();
void stateControllerPCEditBankMSB();
void stateControllerPCEditBankLSB();
void stateControllerPCEditPC();
void stateControllerPCEditSave();

#endif

