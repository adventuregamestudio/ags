
#include "roomstatus.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void RoomStatus::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    beenhere = in->ReadInt32();
    numobj = in->ReadInt32();
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        obj[i].ReadFromFile(in);
    }
    in->ReadArray(flagstates, sizeof(short), MAX_FLAGS);
    // might need to skip 2 if MAX_FLAGS is odd
    in->Seek(Common::kSeekCurrent, 2*(MAX_FLAGS%2));
    tsdatasize = in->ReadInt32();
    tsdata = (char *) in->ReadInt32();
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        intrHotspot[i].ReadFromFile(in);
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        intrObject[i].ReadFromFile(in);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        intrRegion[i].ReadFromFile(in);
    }
    intrRoom.ReadFromFile(in);
    in->ReadArray(hotspot_enabled, sizeof(char), MAX_HOTSPOTS);
    in->ReadArray(region_enabled, sizeof(char), MAX_REGIONS);
    in->ReadArray(walkbehind_base, sizeof(short), MAX_OBJ);
    in->Seek(Common::kSeekCurrent, get_padding(MAX_HOTSPOTS+MAX_REGIONS+2*MAX_OBJ));
    in->ReadArray(interactionVariableValues, sizeof(int), MAX_GLOBAL_VARIABLES);
//#else
//    throw "RoomStatus::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
void RoomStatus::WriteToFile(CDataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    char pad[4];
    out->WriteInt32(beenhere);
    out->WriteInt32(numobj);
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        obj[i].WriteToFile(out);
    }
    out->WriteArray(flagstates, sizeof(short), MAX_FLAGS);
    // might need to skip 2 if MAX_FLAGS is odd
    out->WriteArray(pad, sizeof(char), 2*(MAX_FLAGS%2));
    out->WriteInt32(tsdatasize);
    out->WriteInt32((int)tsdata);
    for (int i = 0; i < MAX_HOTSPOTS; ++i)
    {
        intrHotspot[i].WriteToFile(out);
    }
    for (int i = 0; i < MAX_INIT_SPR; ++i)
    {
        intrObject[i].WriteToFile(out);
    }
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
        intrRegion[i].WriteToFile(out);
    }
    intrRoom.WriteToFile(out);
    out->WriteArray(hotspot_enabled, sizeof(char), MAX_HOTSPOTS);
    out->WriteArray(region_enabled, sizeof(char), MAX_REGIONS);
    out->WriteArray(walkbehind_base, sizeof(short), MAX_OBJ);
    out->WriteArray(pad, sizeof(char), get_padding(MAX_HOTSPOTS+MAX_REGIONS+2*MAX_OBJ));
    out->WriteArray(interactionVariableValues, sizeof(int), MAX_GLOBAL_VARIABLES);
//#else
//    throw "RoomStatus::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
