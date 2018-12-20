//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/common.h"
#include "ac/roomstruct.h"
#include "game/room_file.h"
#include "gfx/bitmap.h"
#include "util/stream.h"

using namespace AGS::Common;

Bitmap *backups[5];
void sprstruc::ReadFromFile(Stream *in)
{
    sprnum = in->ReadInt16();
    x = in->ReadInt16();
    y = in->ReadInt16();
    room = in->ReadInt16();
    on = in->ReadInt16();
}

RoomStruct::RoomStruct() {
    ebscene[0] = NULL; walls = NULL; object = NULL; lookat = NULL; nummes = 0;
    left = 0; right = 317; top = 40; bottom = 199; numobj = MAX_OBJ; numsprs = 0; password[0] = 0;
    wasversion = kRoomVersion_Current; regions = NULL; numwalkareas = 0;
    numhotspots = 0;
    memset(&objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
    memset(&objectFlags[0], 0, sizeof(short) * MAX_INIT_SPR);
    width = 320; height = 200; scripts = NULL;
    cscriptsize = 0;
    memset(&walk_area_zoom[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
    memset(&walk_area_light[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
    resolution = 1; num_bscenes = 1; ebscene[0] = NULL;
    bscene_anim_speed = 5; bytes_per_pixel = 1;
    numLocalVars = 0;
    localvars = NULL;
    int i;
    for (i = 0; i <= MAX_WALK_AREAS; i++) {
        walk_area_zoom2[i] = NOT_VECTOR_SCALED;
        walk_area_top[i] = -1;
        walk_area_bottom[i] = -1;
    }
    for (i = 0; i < MAX_HOTSPOTS; i++) {
        intrHotspot[i] = new Interaction();
    }
    for (i = 0; i < MAX_INIT_SPR; i++)
        intrObject[i] = new Interaction();
    for (i = 0; i < MAX_REGIONS; i++)
        intrRegion[i] = new Interaction();
    intrRoom = new Interaction();
    gameId = 0;
    numRegions = 0;
    hotspotScripts = NULL;
    regionScripts = NULL;
    objectScripts = NULL;
    roomScripts = NULL;
}

void RoomStruct::freemessage() {
    for (int f = 0; f < nummes; f++) {
        if (message[f] != NULL)
            free(message[f]);
    }
}

void RoomStruct::freescripts()
{
    if (scripts != NULL)
    {
        delete scripts;
        scripts = NULL;
    }

    compiled_script.reset();

    if (roomScripts != NULL) 
    {
        delete roomScripts;
        roomScripts = NULL;
    }
    if (hotspotScripts != NULL)
    {
        for (int i = 0; i < numhotspots; i++)
	    {
            delete hotspotScripts[i];
        }
        delete[] hotspotScripts;
        hotspotScripts = NULL;
    }
    if (objectScripts != NULL)
    {
        for (int i = 0; i < numsprs; i++)
	    {
            delete objectScripts[i];
        }
        delete[] objectScripts;
        objectScripts = NULL;
    }
    if (regionScripts != NULL)
    {
        for (int i = 0; i < numRegions; i++)
        {
            delete regionScripts[i];
        }
        delete[] regionScripts;
        regionScripts = NULL;
    }
}

bool RoomStruct::has_region_lightlevel(int id) const
{
    if (id >= 0 && id < MAX_REGIONS)
        return regionTintLevel[id] == 0;
    return false;
}

bool RoomStruct::has_region_tint(int id) const
{
    if (id >= 0 && id < MAX_REGIONS)
        return regionTintLevel[id] != 0;
    return false;
}

int RoomStruct::get_region_lightlevel(int id) const
{
    if (id >= 0 && id < MAX_REGIONS)
        return has_region_lightlevel(id) ? regionLightLevel[id] : 0;
    return 0;
}

int RoomStruct::get_region_tintluminance(int id) const
{
    if (id >= 0 && id < MAX_REGIONS)
        return has_region_tint(id) ? (regionLightLevel[id] * 10) / 25 : 0;
    return 0;
}

void free_room(RoomStruct *rstruc)
{
    rstruc->width = 320;
    rstruc->height = 200;
    rstruc->resolution = 1;
    rstruc->numwalkareas = 0;
    rstruc->numhotspots = 0;

    memset(&rstruc->shadinginfo[0], 0, sizeof(short) * 16);
    memset(&rstruc->sprs[0], 0, sizeof(sprstruc) * MAX_INIT_SPR);
    memset(&rstruc->objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
    memset(&rstruc->objectFlags[0], 0, sizeof(short) * MAX_INIT_SPR);
    memset(&rstruc->hswalkto[0], 0, sizeof(_Point) * MAX_HOTSPOTS);
    memset(&rstruc->walk_area_zoom[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));
    memset(&rstruc->walk_area_light[0], 0, sizeof(short) * (MAX_WALK_AREAS + 1));

    rstruc->nummes = 0;
    memset(&rstruc->msgi[0], 0, sizeof(MessageInfo) * MAXMESS);

    for (size_t i = 0; i < (size_t)MAX_HOTSPOTS; ++i)
    {
        rstruc->hotspotScriptNames[i].Free();
        if (i == 0)
            rstruc->hotspotnames[i] = "No hotspot";
        else
            rstruc->hotspotnames[i].Format("Hotspot %d", i);
    }

    rstruc->freemessage();
    rstruc->freescripts();
  
    if (rstruc->num_bscenes > 1)
    {
        for (size_t i = 1; i < (size_t)rstruc->num_bscenes; ++i)
        {
            delete rstruc->ebscene[i];
            rstruc->ebscene[i] = NULL;
        }
    }

    rstruc->num_bscenes = 1;
    rstruc->bscene_anim_speed = 5;
    for (size_t i = 0; i < (size_t)MAX_INIT_SPR; ++i)
    {
        rstruc->objectnames[i].Free();
        rstruc->objectscriptnames[i].Free();
    }
    memset (&rstruc->regionLightLevel[0], 0, sizeof(short) * MAX_REGIONS);
    memset (&rstruc->regionTintLevel[0], 0, sizeof(int) * MAX_REGIONS);

    for (size_t i = 0; i <= (size_t)MAX_WALK_AREAS; i++)
    {
        rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
        rstruc->walk_area_top[i] = -1;
        rstruc->walk_area_bottom[i] = -1;
    }

    for (size_t i = 0; i < (size_t)rstruc->numhotspots; i++)
        rstruc->hsProps[i].clear();
    for (size_t i = 0; i < (size_t)rstruc->numsprs; i++)
        rstruc->objProps[i].clear();
    rstruc->roomProps.clear();

    // free old interactions
    delete rstruc->intrRoom;
    for (size_t i = 0; i < (size_t)MAX_HOTSPOTS; ++i)
    {
        delete rstruc->intrHotspot[i];
        rstruc->intrHotspot[i] = NULL;
    }
    for (size_t i = 0; i < (size_t)MAX_INIT_SPR; ++i)
    {
        delete rstruc->intrObject[i];
        rstruc->intrObject[i] = NULL;
    }
    for (size_t i = 0; i < (size_t)MAX_REGIONS; ++i)
    {
        delete rstruc->intrRegion[i];
        rstruc->intrRegion[i] = NULL;
    }

    if (rstruc->localvars != NULL)
        delete [] rstruc->localvars;
    rstruc->localvars = NULL;
    rstruc->numLocalVars = 0;

    memset(&rstruc->ebpalShared[0], 0, MAX_BSCENE);
}

void load_room(const char *files, RoomStruct *rstruc, bool gameIsHighRes, const std::vector<SpriteInfo> &sprinfos)
{
    free_room(rstruc);

    update_polled_stuff_if_runtime();

    RoomDataSource src;
    HRoomFileError err = OpenRoomFile(files, src);
    if (err)
    {
        update_polled_stuff_if_runtime();  // it can take a while to load the file sometimes
        err = ReadRoomData(rstruc, src.InputStream.get(), src.DataVersion);
        if (err)
            err = UpdateRoomData(rstruc, src.DataVersion, gameIsHighRes, sprinfos);
    }
    if (!err)
        quitprintf("Unable to load the room file '%s'.\n%s.", files, err->FullMessage().GetCStr());
}
