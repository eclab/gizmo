////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"

/// LOAD DATA
/// Loads data from Flash starting at the given position and of the given length.
void loadData(char* data, uint16_t position, uint16_t length)
    {
    for (uint16_t i = 0; i < length; i++)
        *(data + i) = EEPROM.read(i + position);
    }

/// SAVE DATA
/// Saves data to Flash starting at the given position and of the given length.
void saveData(char* data, uint16_t position, uint16_t length)
    {
    for (uint16_t i = 0; i < length; i++)
        EEPROM.update(i + position, *(data + i));
    }

/// LOAD SLOT
/// Loads a slot of the given index.
void loadSlot(uint8_t index)
    {
    // converting this to a #define saves zero bytes :-(
    loadData((char*)(&(data.slot)), sizeof(struct _slot) * index + SLOT_OFFSET, sizeof(struct _slot));
    }

/// SAVE SLOT
/// Saves a slot of the given index.
void saveSlot(uint8_t index)
    {
    // converting this to a #define saves zero bytes :-(
    saveData((char*)(&(data.slot)), sizeof(struct _slot) * index  + SLOT_OFFSET, sizeof(struct _slot));
    }
    
/// GET SLOT TYPE
/// Returns the type of a given slot.
uint8_t getSlotType(uint8_t index)
    {
    uint8_t type;
    loadData((char*)&type, sizeof(struct _slot) * index + SLOT_OFFSET, 1);
    return type;
    }

uint8_t slotTypeForApplication(uint8_t application)
    {
    switch(application)
        {
#ifdef INCLUDE_STEP_SEQUENCER
        case STATE_STEP_SEQUENCER:
            return SLOT_TYPE_STEP_SEQUENCER;
#endif
#ifdef INCLUDE_RECORDER
        case STATE_RECORDER:
            return SLOT_TYPE_RECORDER;
#endif
        default:
            return SLOT_TYPE_EMPTY;
        }
    }
        

// Our loaded data
GLOBAL union _data data;





