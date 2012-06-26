
#include "roomstatus.h"

void RoomStatus::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    beenhere = getw(fp);
    numobj = getw(fp);
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        obj[i].ReadFromFile(fp);
    }
    fread(flagstates, sizeof(short), MAX_FLAGS, fp);
    // might need to skip 2 if MAX_FLAGS is odd
    fseek(fp, 2*(MAX_FLAGS%2), SEEK_CUR);
    tsdatasize = getw(fp);
    tsdata = (char *) getw(fp);
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        intrHotspot[i].ReadFromFile(fp);
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        intrObject[i].ReadFromFile(fp);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        intrRegion[i].ReadFromFile(fp);
    }
    intrRoom.ReadFromFile(fp);
    fread(hotspot_enabled, sizeof(char), MAX_HOTSPOTS, fp);
    fread(region_enabled, sizeof(char), MAX_REGIONS, fp);
    fread(walkbehind_base, sizeof(short), MAX_OBJ, fp);
    fseek(fp, 4 - ((MAX_HOTSPOTS+MAX_REGIONS+2*MAX_OBJ)%4), SEEK_CUR);
    fread(interactionVariableValues, sizeof(int), MAX_GLOBAL_VARIABLES, fp);
//#else
//    throw "RoomStatus::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
void RoomStatus::WriteToFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    char pad[4];
    putw(beenhere, fp);
    putw(numobj, fp);
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        obj[i].WriteToFile(fp);
    }
    fwrite(flagstates, sizeof(short), MAX_FLAGS, fp);
    // might need to skip 2 if MAX_FLAGS is odd
    fwrite(pad, sizeof(char), 2*(MAX_FLAGS%2), fp);
    putw(tsdatasize, fp);
    putw((int)tsdata, fp);
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        intrHotspot[i].WriteToFile(fp);
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        intrObject[i].WriteToFile(fp);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        intrRegion[i].WriteToFile(fp);
    }
    intrRoom.WriteToFile(fp);
    fwrite(hotspot_enabled, sizeof(char), MAX_HOTSPOTS, fp);
    fwrite(region_enabled, sizeof(char), MAX_REGIONS, fp);
    fwrite(walkbehind_base, sizeof(short), MAX_OBJ, fp);
    fwrite(pad, sizeof(char), 4 - ((MAX_HOTSPOTS+MAX_REGIONS+2*MAX_OBJ)%4), fp);
    fwrite(interactionVariableValues, sizeof(int), MAX_GLOBAL_VARIABLES, fp);
//#else
//    throw "RoomStatus::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
