
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOMSTATUS_H
#define __AGS_EE_AC__ROOMSTATUS_H

#include "ac/roomobject.h"
#include "ac/ac_interaction.h"

// This struct is saved in the save games - it contains everything about
// a room that could change
struct RoomStatus {
    int   beenhere;
    int   numobj;
    RoomObject obj[MAX_INIT_SPR];
    short flagstates[MAX_FLAGS];
    int   tsdatasize;
    char* tsdata;
    NewInteraction intrHotspot[MAX_HOTSPOTS];
    NewInteraction intrObject [MAX_INIT_SPR];
    NewInteraction intrRegion [MAX_REGIONS];
    NewInteraction intrRoom;
    // [IKM] 2012-06-22: not used anywhere
#ifdef UNUSED_CODE
    EventBlock hscond[MAX_HOTSPOTS];
    EventBlock objcond[MAX_INIT_SPR];
    EventBlock misccond;
#endif
    char  hotspot_enabled[MAX_HOTSPOTS];
    char  region_enabled[MAX_REGIONS];
    short walkbehind_base[MAX_OBJ];
    int   interactionVariableValues[MAX_GLOBAL_VARIABLES];

    RoomStatus() { beenhere=0; numobj=0; tsdatasize=0; tsdata=NULL; }

    void ReadFromFile(FILE *fp);
    void WriteToFile(FILE *fp);
};

#endif // __AGS_EE_AC__ROOMSTATUS_H
