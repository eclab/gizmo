////// Copyright 2020 by Sean Luke
////// Licensed under the Apache 2.0 License



#ifndef __SYSEX_H__
#define __SYSEX_H__

#define SYSEX_VERSION 0

#define NO_SYSEX_SLOT (-1)
#define SYSEX_TYPE_SLOT 0
#define SYSEX_TYPE_ARP 1
#define RECEIVED_WRONG (-1)
#define RECEIVED_BAD (-2)
#define RECEIVED_NONE (0)

struct _sysexLocal
    {
    int8_t slot;
    uint8_t type;
    int8_t received;
    };

void sendSlotSysex();
void sendArpSysex();
void handleSysex(unsigned char* bytes, int len);
void stateSysexSlot();
void stateSysexArp();
void stateSysexGo();

#endif __SYSEX_H__
