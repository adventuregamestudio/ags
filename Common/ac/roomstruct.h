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

#ifndef __AC_ROOMSTRUCT_H
#define __AC_ROOMSTRUCT_H

#include "ac/common_defines.h"
#include "ac/messageinfo.h"
#include "ac/animationstruct.h"
#include "ac/point.h"
#include "game/customproperties.h"
#include "game/interactions.h"
#include "script/cc_script.h"       // ccScript
#include "util/wgt2allg.h" // color (allegro RGB)

namespace AGS { namespace Common { class Stream; } }
namespace AGS { namespace Common { class Bitmap; } }
using AGS::Common::Stream;
using AGS::Common::Bitmap;
using AGS::Common::InteractionScripts;

// thisroom.options[0] = startup music
// thisroom.options[1] = can save/load on screen (0=yes, 1=no)
// thisroom.options[2] = player character disabled? (0=no, 1=yes)
// thisroom.options[3] = player special view (0=normal)
//                 [4] = music volume (0=normal, <0 quiter, >0 louder)

const int ST_TUNE = 0, ST_SAVELOAD = 1, ST_MANDISABLED = 2, ST_MANVIEW = 3, ST_VOLUME = 4;

enum RoomVolumeAdjustment
{
    kRoomVolumeQuietest = -3,
    kRoomVolumeQuieter  = -2,
    kRoomVolumeQuiet    = -1,
    kRoomVolumeNormal   =  0,
    kRoomVolumeLoud     =  1,
    kRoomVolumeLouder   =  2,
    kRoomVolumeLoudest  =  3,
    // These two options are only settable at runtime by SetMusicVolume()
    kRoomVolumeExtra1   =  4,
    kRoomVolumeExtra2   =  5,

    kRoomVolumeMin      = kRoomVolumeQuietest,
    kRoomVolumeMax      = kRoomVolumeExtra2,
};

#pragma pack(1)
struct sprstruc {
    short sprnum  ;   // number from array
    short x,y     ;   // x,y co-ords
    short room    ;   // room number
    short on      ;
    sprstruc() { on = 0; }

    void ReadFromFile(Common::Stream *in);
};
#pragma pack()

#define NOT_VECTOR_SCALED -10000
#define LEGACY_TINT_IS_ENABLED 0x80000000
struct RoomStruct {
    Common::Bitmap *        walls, *object, *lookat;          // 'object' is the walk-behind
    Common::Bitmap *        regions;
    color         pal[256];
    short         numobj;                         // num hotspots, not sprites
    short         objyval[MAX_OBJ];               // baselines of walkbehind areas
    // obsolete v2.00 action editor stuff below
    short         whataction[NUM_CONDIT+3];       // what to do if condition appears
    short         val1[NUM_CONDIT+3];             // variable, eg. screen number to go to
    short         val2[NUM_CONDIT+3];             // 2nd var, optional, eg. which side of screen to come on
    short         otcond[NUM_CONDIT+3];           // do extra misc condition
    char          points[NUM_CONDIT+3];           // extra points for doing it
    // end obsolete v2.00 action editor
    short         left,right,top,bottom;          // to walk off screen
    short         numsprs,nummes;                 // number of initial sprites and messages
    sprstruc      sprs[MAX_INIT_SPR];             // structures for each sprite
    //Interaction  *intrObject[MAX_INIT_SPR];// CLNUP old interactions
    InteractionScripts **objectScripts;
    int           objbaseline[MAX_INIT_SPR];                // or -1 (use bottom of object graphic)
    short         objectFlags[MAX_INIT_SPR];
    AGS::Common::String objectnames[MAX_INIT_SPR];
    AGS::Common::String objectscriptnames[MAX_INIT_SPR];
    AGS::Common::StringIMap objProps[MAX_INIT_SPR];
    char          password[11];
    char          options[10];                    // [0]=startup music
    char          *message[MAXMESS];
    MessageInfo   msgi[MAXMESS];
    short         wasversion;                     // when loaded from file
    short         flagstates[MAX_FLAGS]; // unused!
    FullAnimation anims[MAXANIMS]; // no longer supported
    short         numanims;
    short         shadinginfo[16];    // walkable area-specific view number
    // new version 2 roommake stuff below
    // CLNUP very old polygonal walkable areas (unknown version)
    int           numpolyareas;
    PolyPoints    wallpoints[MAX_WALK_AREAS];
    int           numhotspots;
    _Point        hswalkto[MAX_HOTSPOTS];
    AGS::Common::String hotspotnames[MAX_HOTSPOTS];
    AGS::Common::String hotspotScriptNames[MAX_HOTSPOTS];
    // CLNUP old interactions
    //Interaction  *intrHotspot[MAX_HOTSPOTS];
    //Interaction  *intrRoom;
    //Interaction  *intrRegion[MAX_REGIONS];
    InteractionScripts **hotspotScripts;
    InteractionScripts **regionScripts;
    InteractionScripts *roomScripts;
    int           numRegions;
    short         regionLightLevel[MAX_REGIONS];
    int           regionTintLevel[MAX_REGIONS];
    short         width,height;                             // in 320x200 terms (scrolling room size)
    //short         resolution;                               // [DEPRECATED] 1 = 320x200, 2 = 640x400
    int           numwalkareas;
    short         walk_area_zoom[MAX_WALK_AREAS + 1];       // 0 = 100%, 1 = 101%, -1 = 99%
    short         walk_area_zoom2[MAX_WALK_AREAS + 1];      // for vector scaled areas
    short         walk_area_light[MAX_WALK_AREAS + 1];      // 0 = normal, + lighter, - darker
    short         walk_area_top[MAX_WALK_AREAS + 1];     // top YP of area
    short         walk_area_bottom[MAX_WALK_AREAS + 1];  // bottom YP of area
    char          *scripts;
    PScript       compiled_script;
    int           cscriptsize;
    int           num_bscenes, bscene_anim_speed;
    int           bytes_per_pixel;
    Common::Bitmap *        ebscene[MAX_BSCENE];
    color         bpalettes[MAX_BSCENE][256];
    //InteractionVariable *localvars;// CLNUP old interactions
    int           numLocalVars;
    char          ebpalShared[MAX_BSCENE];  // used internally by engine atm
    AGS::Common::StringIMap roomProps;
    AGS::Common::StringIMap hsProps[MAX_HOTSPOTS];
    int           gameId;

    RoomStruct();
    void freemessage();
    void freescripts();

    // Gets if the given region has light level set
    bool has_region_lightlevel(int id) const;
    // Gets if the given region has a tint set
    bool has_region_tint(int id) const;
    // Gets region's light level in -100 to 100 range value; returns 0 (default level) if region's tint is set
    int  get_region_lightlevel(int id) const;
    // Gets region's tint luminance in 0 to 100 range value; returns 0 if region's light level is set
    int  get_region_tintluminance(int id) const;
};

extern void load_room(const char *files, RoomStruct *rstruc);


// Those are, in fact, are project-dependent and are implemented in runtime and AGS.Native
extern void load_script_configuration(Common::Stream *in);
extern void save_script_configuration(Common::Stream *out);
extern void load_graphical_scripts(Common::Stream *in, RoomStruct *);
extern void save_graphical_scripts(Common::Stream *out, RoomStruct *);
//

#endif // __AC_ROOMSTRUCT_H