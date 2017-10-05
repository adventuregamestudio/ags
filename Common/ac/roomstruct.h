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

/* room file versions history
8:  final v1.14 release
9:  intermediate v2 alpha releases
10:  v2 alpha-7 release
11:  final v2.00 release
12:  v2.08, to add colour depth byte
13:  v2.14, add walkarea light levels
14:  v2.4, fixed so it saves walkable area 15
15:  v2.41, supports NewInteraction
16:  v2.5
17:  v2.5 - just version change to force room re-compile for new charctr struct
18:  v2.51 - vector scaling
19:  v2.53 - interaction variables
20:  v2.55 - shared palette backgrounds
21:  v2.55 - regions
22:  v2.61 - encrypt room messages
23:  v2.62 - object flags
24:  v2.7  - hotspot script names
25:  v2.72 - game id embedded
26:  v3.0 - new interaction format, and no script source
27:  v3.0 - store Y of bottom of object, not top
28:  v3.0.3 - remove hotspot name length limit
29:  v3.0.3 - high-res coords for object x/y, edges and hotspot walk-to point
30:  v3.4.0.4 - tint luminance for regions
31:  v3.4.1.5 - removed room object and hotspot name length limits
*/
enum RoomFileVersion
{
    kRoomVersion_Undefined  = 0,
    kRoomVersion_pre114_3   = 3,  // exact version unknown
    kRoomVersion_pre114_4   = 4,  // exact version unknown
    kRoomVersion_pre114_5   = 5,  // exact version unknown
    kRoomVersion_pre114_6   = 6,  // exact version unknown
    kRoomVersion_114        = 8,
    kRoomVersion_200_alpha  = 9,
    kRoomVersion_200_alpha7 = 10,
    kRoomVersion_200_final  = 11,
    kRoomVersion_208        = 12,
    kRoomVersion_214        = 13,
    kRoomVersion_240        = 14,
    kRoomVersion_241        = 15,
    kRoomVersion_250a       = 16,
    kRoomVersion_250b       = 17,
    kRoomVersion_251        = 18,
    kRoomVersion_253        = 19,
    kRoomVersion_255a       = 20,
    kRoomVersion_255b       = 21,
    kRoomVersion_261        = 22,
    kRoomVersion_262        = 23,
    kRoomVersion_270        = 24,
    kRoomVersion_272        = 25,
    kRoomVersion_300a       = 26,
    kRoomVersion_300b       = 27,
    kRoomVersion_303a       = 28,
    kRoomVersion_303b       = 29,
    kRoomVersion_3404       = 30,
    kRoomVersion_3415       = 31,
    kRoomVersion_Current    = kRoomVersion_3415
};

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
    short         flagstates[MAX_FLAGS];
    FullAnimation anims[MAXANIMS];
    short         numanims;
    short         shadinginfo[16];    // walkable area-specific view number
    // new version 2 roommake stuff below
    int           numwalkareas;
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

#define BLOCKTYPE_MAIN        1
#define BLOCKTYPE_SCRIPT      2
#define BLOCKTYPE_COMPSCRIPT  3
#define BLOCKTYPE_COMPSCRIPT2 4
#define BLOCKTYPE_OBJECTNAMES 5
#define BLOCKTYPE_ANIMBKGRND  6
#define BLOCKTYPE_COMPSCRIPT3 7     // new CSCOMP script instead of SeeR
#define BLOCKTYPE_PROPERTIES  8
#define BLOCKTYPE_OBJECTSCRIPTNAMES 9
#define BLOCKTYPE_EOF         0xff

struct room_file_header {
    RoomFileVersion version;
    void ReadFromFile(Common::Stream *in);
    void WriteFromFile(Common::Stream *out);
};

extern int _acroom_bpp;  // bytes per pixel of currently loading room

extern void load_room(const char *files, RoomStruct *rstruc);


// Those are, in fact, are project-dependent and are implemented in runtime and AGS.Native
extern void load_script_configuration(Common::Stream *in);
extern void save_script_configuration(Common::Stream *out);
extern void load_graphical_scripts(Common::Stream *in, RoomStruct *);
extern void save_graphical_scripts(Common::Stream *out, RoomStruct *);
//

#endif // __AC_ROOMSTRUCT_H