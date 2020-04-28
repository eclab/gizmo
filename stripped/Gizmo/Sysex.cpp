////// Copyright 2020 by Sean Luke
////// Licensed under the Apache 2.0 License


#include "All.h"



#ifdef INCLUDE_SYSEX
        
void loadHeader(uint8_t bytes[])
    {
    bytes[0] = 0xF0;
    bytes[1] = 0x7D;
    bytes[2] = 'G';
    bytes[3] = 'I';
    bytes[4] = 'Z';
    bytes[5] = 'M';
    bytes[6] = 'O';
    bytes[7] = SYSEX_VERSION;
    }
        
void sendSlotSysex()
    {
    loadSlot(local.sysex.slot);

    // Our format is:
    // 0xF0
    // 0x7D                         [Private, Test, Educational Use]
    // G I Z M O
    // Version number       [Currently 0]
    // Sysex Type           [ 0 = slot, 1 = Arp]
    // Nybblized Data, high nybble first
    // checksum, just sum of data
    // 0xF7
    uint8_t bytes[sizeof(struct _slot) * 2 + 11];
    loadHeader(bytes);
    bytes[8] = SYSEX_TYPE_SLOT;
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _slot); i++)
        {
        bytes[9 + i * 2] = (uint8_t)((data.bytes[i] >> 4) & 0xF);  // unsigned char right shifts are probably logical shifts, but we mask anyway
        bytes[9 + i * 2 + 1] = (uint8_t)(data.bytes[i] & 0xF);
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];

        /*
        if (data.bytes[i] != ((uint8_t)(bytes[9 + i * 2] << 4) | (bytes[9 + i * 2 + 1] & 0xF)))
            debug(999);
        */
        }
    bytes[sizeof(struct _slot) * 2 + 11 - 2] = (sum & 127);
    bytes[sizeof(struct _slot) * 2 + 11 - 1] = 0xF7;
    MIDI.sendSysEx(sizeof(struct _slot) * 2 + 11, bytes, true);
    }        

void sendArpSysex()
    {
    LOAD_ARPEGGIO(local.sysex.slot);

    // Our format is:
    // 0xF0
    // 0x7D                         [Private, Test, Educational Use]
    // G I Z M O
    // Version number       [Currently 0]
    // Sysex Type           [ 0 = slot, 1 = Arp]
    // Nybblized Data, high nybble first
    // checksum, just sum of data
    // 0xF7
    uint8_t bytes[sizeof(struct _arp) * 2 + 11];
    loadHeader(bytes);
    bytes[8] = SYSEX_TYPE_ARP;
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _arp); i++)
        {
        bytes[9 + i * 2] = (uint8_t)((data.bytes[i] >> 4) & 0xF);  // unsigned char right shifts are probably logical shifts, but we mask anyway
        bytes[9 + i * 2 + 1] = (uint8_t)(data.bytes[i] & 0xF);
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];
        }
    bytes[sizeof(struct _arp) * 2 + 11 - 2] = (sum & 127);
    bytes[sizeof(struct _arp) * 2 + 11 - 1] = 0xF7;
    MIDI.sendSysEx(sizeof(struct _arp) * 2 + 11, bytes, true);
    }        
        
uint8_t receiveSlotSysex(unsigned char* bytes)
    {
    // verify checksum
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _slot); i++)
        {
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];
        }
    if ((sum & 127) != bytes[sizeof(struct _slot) * 2 + 11 - 2]) // second to last
        return false;
        
    // load
    for(uint16_t i = 0; i < sizeof(struct _slot); i++)
        {
        data.bytes[i] = (uint8_t)(bytes[9 + i * 2] << 4) | (bytes[9 + i * 2 + 1] & 0xF);
        }
    saveSlot(local.sysex.slot);
    return true;
    }

uint8_t receiveArpSysex(unsigned char* bytes)
    {
    // verify checksum
    uint8_t sum = 0;
    for(uint16_t i = 0; i < sizeof(struct _arp); i++)
        {
        sum += bytes[9 + i * 2];
        sum += bytes[9 + i * 2 + 1];
        }
    if ((sum & 127) != bytes[sizeof(struct _arp) * 2 + 11 - 2]) // second to last
        return false;
        
    // load
    for(uint16_t i = 0; i < sizeof(struct _arp); i++)
        {
        data.bytes[i] = (uint8_t)(bytes[9 + i * 2] << 4) | (bytes[9 + i * 2 + 1] & 0xF);
        }
    SAVE_ARPEGGIO(local.sysex.slot);
    return true;
    }


void handleSysex(unsigned char* bytes, int len)
    {
    // We only process incoming sysex *at all*, even if to generate an error message, if we're
    // in STATE_SYSEX_GO.
    if (state != STATE_SYSEX_GO) return;

    // Our format is:
    // 0xF0
    // 0x7D                         [Private, Test, Educational Use]
    // G I Z M O
    // Version number       [Currently 0]
    // Sysex Type           [ 0 = slot, 1 = Arp]
    // Nybblized Data, high nybble first
    // checksum, just sum of data
    // 0xF7
        
    if (bytes[1] == 0x7D && 
        bytes[2] == 'G' && 
        bytes[3] == 'I' && 
        bytes[4] == 'Z' && 
        bytes[5] == 'M' && 
        bytes[6] == 'O' && 
        bytes[7] == SYSEX_VERSION)  // it's me
        {
        if (local.sysex.type == SYSEX_TYPE_SLOT && bytes[8] == SYSEX_TYPE_SLOT)
            {
            if (len != sizeof(struct _slot) * 2 + 11 || !receiveSlotSysex(bytes))
                {
                local.sysex.received = RECEIVED_BAD;
                }
            else
                {
                local.sysex.received++;
                if (local.sysex.received <= 0)  // previous was BAD or WRONG, or we wrapped around
                    local.sysex.received = 1;
                }
            }
        else if (local.sysex.type == SYSEX_TYPE_ARP && bytes[8] == SYSEX_TYPE_ARP)
            {
            if (len != sizeof(struct _arp) * 2 + 11 | !receiveArpSysex(bytes))
                {
                local.sysex.received = RECEIVED_BAD;
                }
            else
                {
                local.sysex.received++;
                if (local.sysex.received <= 0)  // previous was BAD or WRONG, or we wrapped around
                    local.sysex.received = 1;
                }
            }
        else
            {
            local.sysex.received = RECEIVED_WRONG;  // we're not the right type (slot vs. arp) to receive
            }
        }
    else
        {
        local.sysex.received = RECEIVED_BAD;
        }
    }   


void stateSysexSlot()
	{
            local.sysex.type = SYSEX_TYPE_SLOT;
            
            // make sure that we're reset properly
            local.sysex.received = RECEIVED_NONE;
            uint8_t result = doNumericalDisplay(0, NUM_SLOTS - 1, 1, false, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    break;
                case MENU_SELECTED:
                    local.sysex.slot = currentDisplay;
                    goDownState(STATE_SYSEX_GO);
                    break;
                case MENU_CANCELLED:
                    goUpState(STATE_SYSEX);
                    break;
                }
	}
	
void stateSysexArp()
	{
            local.sysex.type = SYSEX_TYPE_ARP;

            // make sure that we're reset properly
            local.sysex.received = RECEIVED_NONE;
            uint8_t result = doNumericalDisplay(0, NUM_ARPS - 1, 1, false, GLYPH_NONE);
            switch (result)
                {
                case NO_MENU_SELECTED:
                    break;
                case MENU_SELECTED:
                    local.sysex.slot = currentDisplay;
                    goDownState(STATE_SYSEX_GO);
                    break;
                case MENU_CANCELLED:
                    goUpState(STATE_SYSEX);
                    break;
                }
	}
	
void stateSysexGo()
	{
            // display
            if (local.sysex.received == RECEIVED_NONE)
                {
                clearScreen();  // is this necessary?
                write3x5Glyphs(GLYPH_OFF);
                }
            else if (local.sysex.received == RECEIVED_WRONG)
                {
                clearScreen();  // is this necessary?
                write3x5Glyphs(GLYPH_SYSEX);
                }
            else if (local.sysex.received == RECEIVED_BAD)
                {
                clearScreen();  // is this necessary?
                write3x5Glyphs(GLYPH_FAIL);
                }
            else
                {
                clearScreen();
                writeShortNumber(led, ((uint8_t)local.sysex.received), false);
                }
                
            // handle buttons
            if (isUpdated(BACK_BUTTON, RELEASED))
                {
                goUpState(local.sysex.type == SYSEX_TYPE_SLOT ? STATE_SYSEX_SLOT : STATE_SYSEX_ARP);
                }
            else if (isUpdated(SELECT_BUTTON, PRESSED))
                {
                if (local.sysex.type == SYSEX_TYPE_SLOT)
                    {
                    sendSlotSysex();
                    }
                else
                    {
                    sendArpSysex();
                    }
                local.sysex.received++;
                if (local.sysex.received <= 0)  // previous was BAD or WRONG, or we wrapped around
                    local.sysex.received = 1;
                }
	}

#endif INCLUDE_SYSEX
