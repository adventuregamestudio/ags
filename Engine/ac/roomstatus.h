
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOMSTATUS_H
#define __AGS_EE_AC__ROOMSTATUS_H

#include "ac/roomobject.h"
#include "ac/interaction.h"

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

// Replaces all accesses to the roomstats array
RoomStatus* getRoomStatus(int room);
// Used in places where it is only important to know whether the player
// had previously entered the room. In this case it is not necessary
// to initialise the status because a player can only have been in
// a room if the status is already initialised.
bool isRoomStatusValid(int room);
void resetRoomStatuses();

#endif // __AGS_EE_AC__ROOMSTATUS_H
