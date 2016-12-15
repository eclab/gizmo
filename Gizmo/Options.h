////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __OPTIONS_H__
#define __OPTIONS_H__


struct _optionsLocal
    {
    uint32_t lastTempoTapTime;
    };



////// Options struct data type.  At present, 35 bytes.
//
// ON THE UNO    We have 78 bytes available
// ON THE MEGA   We have only 36 bytes available!  We will need to eliminate a slot.
// Alternatively we could merge a bunch of these: stuff like recorderRepeat is 1 bit
// and screenBrightness is 4 bits and channelIn is 4 bits etc.  But it'd require complex
// math and would increase the code size and right now code size is our problem.

struct _options
    {
    // 16-bit stuff first
    uint16_t tempo ;                             // in Beats Per Minute
    uint16_t leftKnobControlNumber;
    uint16_t rightKnobControlNumber; 
    uint16_t middleButtonControlNumber;
    uint16_t selectButtonControlNumber;

    // then 8-bit stuff
    uint8_t screenBrightness;
    uint8_t clock ;                     // Chosen option for handling the MIDI clock
    uint8_t noteSpeedType ;             // Type of note speed the user has chosen (see LEDDisplay.h for a list of them)
    uint8_t swing;
    uint8_t channelIn ;                          // MIDI Channel I'm listening on.  0 means no channel in.  17 means ALL CHANNELS (OMNI).
    uint8_t channelOut ;                         // MIDI Channel I'm sending to by default.    0 means no default channel out.
    uint8_t channelControl ;                     // MIDI Channel to control the device via NRPN messages etc.  0 means no control channel.  16 means Channel In
    uint8_t leftKnobControlType ;
    uint8_t rightKnobControlType  ;
    uint8_t middleButtonControlType ;
    uint8_t middleButtonControlOn ;                            // 0 is off, n is value n+1, and 129 is "decrement" (128)
    uint8_t middleButtonControlOff  ;                            // 0 is off, n is value n+1, and 129 is "decrement" (128)
    uint8_t selectButtonControlType ;
    uint8_t selectButtonControlOn ;                            // 0 is off, n is value n+1, and 129 is "increment" (128)
    uint8_t selectButtonControlOff ;                            // 0 is off, n is value n+1, and 129 is "increment" (128)
    uint8_t noteLength;
    uint8_t arpeggiatorPlayOctaves;
    uint8_t arpeggiatorPlayVelocity;
    uint8_t arpeggiatorLatch;
    //uint8_t stepSequencerNoEcho;
    //uint8_t recorderRepeat;
    uint8_t click;
    uint8_t clickVelocity;

#if defined(__AVR_ATmega2560__)
    uint8_t voltage;
    uint8_t menuDelay ;
    int8_t transpose;
    uint8_t volume;
    uint8_t splitControls;                                          // = 0 by default, SPLIT_RIGHT
    uint8_t splitChannel;
    uint8_t splitNote;
    uint8_t splitLayerNote;
    uint8_t thruExtraNotes;
    uint8_t thruNumDistributionChannels;
#endif  // defined(__AVR_ATmega2560__)

    };

// The options struct which is saved and loaded and used
extern struct _options options;

// Before modifying the options struct, you should back it up here.
// Then if you want to revert (cancel) the changes the user has made,
// You only need to set options = backupOptions
extern struct _options backupOptions;

// Loads the current options from flash.
void loadOptions();

// Saves the current options to flash.
void saveOptions();

// resets the current options in memory to default values(does not save them).
void resetOptions();

#endif

