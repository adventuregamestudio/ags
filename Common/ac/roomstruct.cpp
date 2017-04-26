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

#include <stdio.h>
#include "ac/roomstruct.h"
#include "ac/common.h"
#include "ac/wordsdictionary.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/compress.h"
#include "util/stream.h"
#include "core/assetmanager.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;

int _acroom_bpp = 1;  // bytes per pixel of currently loading room

void sprstruc::ReadFromFile(Common::Stream *in)
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
    wasversion = kRoomVersion_Current; numanims = 0; regions = NULL; numwalkareas = 0;
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
        hotspotnames[i] = NULL;
        hotspotScriptNames[i][0] = 0;
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
        free(scripts);
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

void room_file_header::ReadFromFile(Stream *in)
{
    version = (RoomFileVersion)in->ReadInt16();
}

void room_file_header::WriteFromFile(Common::Stream *out)
{
    out->WriteInt16(version);
}

int usesmisccond = 0;

void load_main_block(RoomStruct *rstruc, const char *files, Stream *in, room_file_header rfh) {
  int   i;
  char  buffer[3000];
  long  tesl;

  usesmisccond = 0;
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

  for (i = 0; i < MAX_HOTSPOTS; i++) {
    rstruc->hotspotScriptNames[i][0] = 0;
    if (rstruc->hotspotnames[i] != NULL)
      free(rstruc->hotspotnames[i]);

    rstruc->hotspotnames[i] = (char*)malloc(20);
    sprintf(rstruc->hotspotnames[i], "Hotspot %d", i);

    if (i == 0)
      strcpy(rstruc->hotspotnames[i], "No hotspot");
  }

/*  memset(&rstruc->hscond[0], 0, sizeof(EventBlock) * MAX_HOTSPOTS);
  memset(&rstruc->objcond[0], 0, sizeof(EventBlock) * MAX_INIT_SPR);
  memset(&rstruc->misccond, 0, sizeof(EventBlock));*/

  _acroom_bpp = in->ReadInt32();

  if (_acroom_bpp < 1)
    _acroom_bpp = 1;

  rstruc->bytes_per_pixel = _acroom_bpp;
  rstruc->numobj = in->ReadInt16();
  if (rstruc->numobj > MAX_OBJ)
    quit("!room newer than this version - too many walk-behinds");

  in->ReadArrayOfInt16(&rstruc->objyval[0], rstruc->numobj);

  rstruc->numhotspots = in->ReadInt32();
  if (rstruc->numhotspots == 0)
    rstruc->numhotspots = 20;
  if (rstruc->numhotspots > MAX_HOTSPOTS)
    quit("room has too many hotspots: need newer version of AGS?");

  // Points are a pair of shorts
  // [IKM] TODO: read/write member for _Point?
  in->ReadArrayOfInt16((int16_t*)&rstruc->hswalkto[0], 2*rstruc->numhotspots);

  for (i = 0; i < rstruc->numhotspots; i++)
  {
    free(rstruc->hotspotnames[i]);

    fgetstring_limit(buffer, in, 2999);
    rstruc->hotspotnames[i] = (char*)malloc(strlen(buffer) + 1);
    strcpy(rstruc->hotspotnames[i], buffer);
  }

  in->ReadArray(&rstruc->hotspotScriptNames[0], MAX_SCRIPT_NAME_LEN, rstruc->numhotspots);

  rstruc->numwalkareas = in->ReadInt32();
  for (int iteratorCount = 0; iteratorCount < rstruc->numwalkareas; ++iteratorCount)
  {
      rstruc->wallpoints[iteratorCount].ReadFromFile(in);
  }
  
  update_polled_stuff_if_runtime();

  rstruc->top = in->ReadInt16();
  rstruc->bottom = in->ReadInt16();
  rstruc->left = in->ReadInt16();
  rstruc->right = in->ReadInt16();

  rstruc->numsprs = in->ReadInt16();
  for (int iteratorCount = 0; iteratorCount < rstruc->numsprs; ++iteratorCount)
  {
      rstruc->sprs[iteratorCount].ReadFromFile(in);
  }


  rstruc->numLocalVars = in->ReadInt32();
  if (rstruc->numLocalVars > 0) {
    rstruc->localvars = (InteractionVariable*)malloc (sizeof(InteractionVariable) * rstruc->numLocalVars);

    for (int iteratorCount = 0; iteratorCount < rstruc->numLocalVars; ++iteratorCount)
    {
        rstruc->localvars[iteratorCount].Read(in);
    }
  }

  rstruc->numRegions = 0;

  if ((rstruc->numhotspots > MAX_HOTSPOTS) || (rstruc->numsprs > MAX_INIT_SPR))
    quit("load_room: room file created with newer version (too many hotspots/objects)");

  // free all of the old interactions
  for (i = 0; i < MAX_HOTSPOTS; i++) {
    if (rstruc->intrHotspot[i] != NULL) {
      delete rstruc->intrHotspot[i];
      rstruc->intrHotspot[i] = NULL;
    }
  }

  for (i = 0; i < MAX_INIT_SPR; i++) {
    if (rstruc->intrObject[i] != NULL) {
      delete rstruc->intrObject[i];
      rstruc->intrObject[i] = NULL;
    }
  }

  for (i = 0; i < MAX_REGIONS; i++) {
    if (rstruc->intrRegion[i] != NULL)
      delete rstruc->intrRegion[i];
    rstruc->intrRegion[i] = new Interaction();
  }

  rstruc->numRegions = in->ReadInt32();
  if (rstruc->numRegions > MAX_REGIONS)
    quit("load_room: needs newer version of AGS - too many regions");

  rstruc->hotspotScripts = new InteractionScripts*[rstruc->numhotspots];
  rstruc->objectScripts = new InteractionScripts*[rstruc->numsprs];
  rstruc->regionScripts = new InteractionScripts*[rstruc->numRegions];
  rstruc->roomScripts = InteractionScripts::CreateFromStream(in);

  for (i = 0; i < rstruc->numhotspots; i++) {
    rstruc->hotspotScripts[i] = InteractionScripts::CreateFromStream(in);
  }
  for (i = 0; i < rstruc->numsprs; i++) {
    rstruc->objectScripts[i] = InteractionScripts::CreateFromStream(in);
  }
  for (i = 0; i < rstruc->numRegions; i++) {
    rstruc->regionScripts[i] = InteractionScripts::CreateFromStream(in);
  }



  in->ReadArrayOfInt32(&rstruc->objbaseline[0], rstruc->numsprs);
  rstruc->width = in->ReadInt16();
  rstruc->height = in->ReadInt16(); 

  in->ReadArrayOfInt16(&rstruc->objectFlags[0], rstruc->numsprs);

  rstruc->resolution = in->ReadInt16();

  int num_walk_areas = MAX_WALK_AREAS;
  num_walk_areas = in->ReadInt32();
    
  if (num_walk_areas > MAX_WALK_AREAS + 1)
    quit("load_room: Too many walkable areas, need newer version");

  in->ReadArrayOfInt16(&rstruc->walk_area_zoom[0], num_walk_areas);
  in->ReadArrayOfInt16(&rstruc->walk_area_light[0], num_walk_areas);
  in->ReadArrayOfInt16(&rstruc->walk_area_zoom2[0], num_walk_areas);
  in->ReadArrayOfInt16(&rstruc->walk_area_top[0], num_walk_areas);
  in->ReadArrayOfInt16(&rstruc->walk_area_bottom[0], num_walk_areas);

  for (i = 0; i < num_walk_areas; i++) {
    // if they set a contiuously scaled area where the top
    // and bottom zoom levels are identical, set it as a normal
    // scaled area
    if (rstruc->walk_area_zoom[i] == rstruc->walk_area_zoom2[i])
      rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
  }
  

  in->Read(&rstruc->password[0], 11);
  in->Read(&rstruc->options[0], 10);
  rstruc->nummes = in->ReadInt16();

  rstruc->gameId = in->ReadInt32();

  for (int iteratorCount = 0; iteratorCount < rstruc->nummes; ++iteratorCount)
  {
      rstruc->msgi[iteratorCount].ReadFromFile(in);
  }

  for (i = 0;i < rstruc->nummes; i++) {
    read_string_decrypt(in, buffer);

    int buffer_length = strlen(buffer);

    rstruc->message[i] = (char *)malloc(buffer_length + 2);
    strcpy(rstruc->message[i], buffer);

    if ((buffer_length > 0) && (buffer[buffer_length-1] == (char)200)) {
      rstruc->message[i][strlen(buffer)-1] = 0;
      rstruc->msgi[i].flags |= MSG_DISPLAYNEXT;
    }
  }

  rstruc->numanims = in->ReadInt16();

  if (rstruc->numanims > 0)
    // [IKM] CHECKME later: this will cause trouble if structure changes
    in->Seek (sizeof(FullAnimation) * rstruc->numanims);
//    in->ReadArray(&rstruc->anims[0], sizeof(FullAnimation), rstruc->numanims);


  in->ReadArrayOfInt16(&rstruc->shadinginfo[0], 16);
  in->ReadArrayOfInt16 (&rstruc->regionLightLevel[0], rstruc->numRegions);
  in->ReadArrayOfInt32 (&rstruc->regionTintLevel[0], rstruc->numRegions);


  if (rfh.version < kRoomVersion_3404)
  {
    // Convert the old format tint saturation
    for (int i = 0; i < MAX_REGIONS; ++i)
    {
      if ((rstruc->regionTintLevel[i] & LEGACY_TINT_IS_ENABLED) != 0)
      {
        rstruc->regionTintLevel[i] &= ~LEGACY_TINT_IS_ENABLED;
        // older versions of the editor had a bug - work around it
        int tint_amount = (rstruc->regionLightLevel[i] > 0 ? rstruc->regionLightLevel[i] : 50);
        rstruc->regionTintLevel[i] |= (tint_amount & 0xFF) << 24;
        rstruc->regionLightLevel[i] = 255;
      }
    }
  }

  update_polled_stuff_if_runtime();
  tesl = load_lzw(in, rstruc->ebscene[0], rstruc->pal);
  rstruc->ebscene[0] = recalced;

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(in, &rstruc->regions, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(in, &rstruc->walls, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(in, &rstruc->object, rstruc->pal, tesl);

  update_polled_stuff_if_runtime();
  tesl = loadcompressed_allegro(in, &rstruc->lookat, rstruc->pal, tesl);

  for (i = 0; i < 11; i++)
    rstruc->password[i] += passwencstring[i];

}

extern bool load_room_is_version_bad(RoomStruct *rstruc);

void load_room(const char *files, RoomStruct *rstruc) {
  Common::Stream *opty; // CHECKME why "opty"??
  room_file_header  rfh;
  int i;

  rstruc->freemessage();
  rstruc->freescripts();
  
  if (rstruc->num_bscenes > 1) {
    int ff;

    for (ff = 1; ff < rstruc->num_bscenes; ff++) {
      delete rstruc->ebscene[ff];
      rstruc->ebscene[ff] = NULL;
    }
    update_polled_stuff_if_runtime();
  }

  rstruc->num_bscenes = 1;
  rstruc->bscene_anim_speed = 5;
  memset (&rstruc->objectnames[0][0], 0, MAX_INIT_SPR * MAXOBJNAMELEN);
  memset (&rstruc->objectscriptnames[0][0], 0, MAX_INIT_SPR * MAX_SCRIPT_NAME_LEN);
  memset (&rstruc->regionLightLevel[0], 0, sizeof(short) * MAX_REGIONS);
  memset (&rstruc->regionTintLevel[0], 0, sizeof(int) * MAX_REGIONS);

  for (i = 0; i <= MAX_WALK_AREAS; i++) {
    rstruc->walk_area_zoom2[i] = NOT_VECTOR_SCALED;
    rstruc->walk_area_top[i] = -1;
    rstruc->walk_area_bottom[i] = -1;
  }

  for (i = 0; i < rstruc->numhotspots; i++)
    rstruc->hsProps[i].clear();
  for (i = 0; i < rstruc->numsprs; i++)
    rstruc->objProps[i].clear();
  rstruc->roomProps.clear();

  if (rstruc->localvars != NULL)
    free (rstruc->localvars);
  rstruc->localvars = NULL;
  rstruc->numLocalVars = 0;

  memset(&rstruc->ebpalShared[0], 0, MAX_BSCENE);

  update_polled_stuff_if_runtime();

  opty = Common::AssetManager::OpenAsset(files);
  if (opty == NULL) {
    char errbuffr[500];
    sprintf(errbuffr,"Load_room: Unable to load the room file '%s'\n"
      "Make sure that you saved the room to the correct folder (it should be\n"
      "in your game's sub-folder of the AGS directory).\n"
      "Also check that the player character's starting room is set correctly.\n",files);
    quit(errbuffr);
  }
  update_polled_stuff_if_runtime();  // it can take a while to load the file sometimes

  rfh.ReadFromFile(opty);
  //fclose(opty);
  rstruc->wasversion = rfh.version;

  if (load_room_is_version_bad(rstruc))
  {
    delete opty;
    quit("Load_Room: Bad packed file. Either the file requires a newer or older version of\n"
      "this program or the file is corrupt.\n");
  }

  int   thisblock = 0;
  int   bloklen;

  while (thisblock != BLOCKTYPE_EOF) {
    update_polled_stuff_if_runtime();
    thisblock = opty->ReadByte();

    if (thisblock == BLOCKTYPE_EOF)
      break;

    bloklen = opty->ReadInt32();
    bloklen += opty->GetPosition();  // make it the new position for after block read

    if (thisblock == BLOCKTYPE_MAIN)
      load_main_block(rstruc, files, opty, rfh);
    else if (thisblock == BLOCKTYPE_SCRIPT) {
      int   lee;
      int   hh;

      lee = opty->ReadInt32();
      rstruc->scripts = (char *)malloc(lee + 5);
      // MACPORT FIX: swap
      opty->Read(rstruc->scripts, lee);
      rstruc->scripts[lee] = 0;

      for (hh = 0; hh < lee; hh++)
        rstruc->scripts[hh] += passwencstring[hh % 11];
    }
    else if (thisblock == BLOCKTYPE_COMPSCRIPT3) {
      rstruc->compiled_script.reset(ccScript::CreateFromStream(opty));
      if (rstruc->compiled_script == NULL)
        quit("Load_room: Script load failed; need newer version?");
    }
    else if ((thisblock == BLOCKTYPE_COMPSCRIPT) || (thisblock == BLOCKTYPE_COMPSCRIPT2))
#ifdef LOADROOM_ALLOWOLD
      rstruc->compiled_script = NULL;
#else
      quit("Load_room: old room format. Please upgrade the room.");
#endif
    else if (thisblock == BLOCKTYPE_OBJECTNAMES) {
      if (opty->ReadByte() != rstruc->numsprs)
        quit("Load_room: inconsistent blocks for object names");

      opty->ReadArray(&rstruc->objectnames[0][0], MAXOBJNAMELEN, rstruc->numsprs);
    }
    else if (thisblock == BLOCKTYPE_OBJECTSCRIPTNAMES) {
      if (opty->ReadByte() != rstruc->numsprs)
        quit("Load_room: inconsistent blocks for object script names");

      opty->ReadArray(&rstruc->objectscriptnames[0][0], MAX_SCRIPT_NAME_LEN, rstruc->numsprs);
    }
    else if (thisblock == BLOCKTYPE_ANIMBKGRND) {
      int   ct;
      long  fpos;

      rstruc->num_bscenes = opty->ReadByte();
      rstruc->bscene_anim_speed = opty->ReadByte();

      opty->Read(&rstruc->ebpalShared[0], rstruc->num_bscenes);

      fpos = opty->GetPosition();
//        fclose(opty);

      for (ct = 1; ct < rstruc->num_bscenes; ct++) {
        update_polled_stuff_if_runtime();
//          fpos = load_lzw(files,rstruc->ebscene[ct],rstruc->pal,fpos);
        fpos = load_lzw(opty, rstruc->ebscene[ct], rstruc->bpalettes[ct]);
        rstruc->ebscene[ct] = recalced;
      }
//        opty = Common::AssetManager::OpenAsset(files, "rb");
//        Seek(opty, fpos, SEEK_SET);
    }
    else if (thisblock == BLOCKTYPE_PROPERTIES) {
      // Read custom properties
      if (opty->ReadInt32() != 1)
        quit("LoadRoom: unknown Custom Properties block encountered");

      int errors = 0, gg;

      if (Properties::ReadValues(rstruc->roomProps, opty))
        quit("LoadRoom: error reading custom properties block");

      for (gg = 0; gg < rstruc->numhotspots; gg++)
        errors += Properties::ReadValues(rstruc->hsProps[gg], opty);
      for (gg = 0; gg < rstruc->numsprs; gg++)
        errors += Properties::ReadValues(rstruc->objProps[gg], opty);

      if (errors > 0)
        quit("LoadRoom: errors encountered reading custom props");
    }
    else if (thisblock == -1)
    {
      delete opty;
      quit("LoadRoom: unexpected end of file while loading room");
      return;
    }
    else {
      char  tempbfr[90];
      sprintf(tempbfr, "LoadRoom: unknown block type %d encountered in '%s'", thisblock, files);
      quit(tempbfr);
    }

    // The GetPosition call below has caused crashes
    if (opty->GetPosition() != bloklen)
        opty->Seek(bloklen, kSeekBegin);
  }

  // sync bpalettes[0] with room.pal
  memcpy (&rstruc->bpalettes[0][0], &rstruc->pal[0], sizeof(color) * 256);

  delete opty;

}
