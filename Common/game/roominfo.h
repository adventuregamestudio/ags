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
//
// RoomInfo, a class of static room data
//
//=============================================================================
#ifndef __AGS_CN_GAME__AGSROOM_H
#define __AGS_CN_GAME__AGSROOM_H

#include "ac/animationstruct.h"
#include "ac/customproperties.h"
#include "ac/interaction.h"
#include "ac/messageinfo.h"
#include "ac/point.h"
#include "util/array.h"
#include "util/string.h"
#include "util/wgt2allg.h"

struct ccScript;

// TODO: move the following enums under AGS::Common namespace
// later, when more engine source is put in AGS namespace and
// refactored.

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
    kRoomVersion_Current    = kRoomVersion_303b
};

enum RoomBaseOptions
{
    kRoomBaseOpt_StartUpMusic,
    kRoomBaseOpt_SaveLoadDisabled,
    kRoomBaseOpt_PlayerCharacterDisabled,
    kRoomBaseOpt_PlayerCharacterView,
    kRoomBaseOpt_MusicVolume
};

#define NOT_VECTOR_SCALED -10000
#define TINT_IS_ENABLED 0x80000000
#define NO_GAME_ID_IN_ROOM_FILE 16325

namespace AGS
{
namespace Common
{

class Bitmap;
class Stream;

struct RoomObjectInfo
{
    int16_t     Id;
    int16_t     X;
    int16_t     Y;
    int16_t     RoomIndex;
    bool        IsOn;

    RoomObjectInfo();
    void ReadFromFile(Common::Stream *in);
};

enum RoomInfoError
{
    kRoomInfoErr_NoError,
    kRoomInfoErr_InternalLogicError,
    kRoomInfoErr_UnexpectedEOF,
    kRoomInfoErr_FormatNotSupported,
    kRoomInfoErr_UnknownBlockType,
    kRoomInfoErr_OldBlockNotSupported,
    kRoomInfoErr_InconsistentDataForObjectNames,
    kRoomInfoErr_ScriptLoadFailed,
    kRoomInfoErr_PropertiesFormatNotSupported,
    kRoomInfoErr_PropertiesLoadFailed,
    kRoomInfoErr_InconsistentDataForObjectScriptNames,
};

enum RoomFormatBlock
{
    kRoomBlock_None             = 0,
    kRoomBlock_Main             = 1,
    kRoomBlock_Script           = 2,
    kRoomBlock_CompScript       = 3,
    kRoomBlock_CompScript2      = 4,
    kRoomBlock_ObjectNames      = 5,
    kRoomBlock_AnimBkg          = 6,
    kRoomBlock_CompScript3      = 7,
    kRoomBlock_Properties       = 8,
    kRoomBlock_ObjectScriptNames= 9,
    kRoomBlock_End              = 0xff
};

class RoomInfo
{
public:
    RoomInfo();
    ~RoomInfo();

    static bool     IsVersionSupported(int16_t version);
    static bool     Load(RoomInfo &room, const String &filename, bool game_is_hires);

    void            Free();
    RoomInfoError   ReadFromFile(Stream *in, bool game_is_hires, RoomFormatBlock *last_block = NULL);
    void            WriteToFile(Stream *out);

private:
    void            InitDefaults();
    RoomInfoError   ReadBlock(Stream *in, RoomFormatBlock block_type);
    RoomInfoError   ReadMainBlock(Stream *in);
    RoomInfoError   ReadScriptBlock(Stream *in);
    RoomInfoError   ReadObjectNamesBlock(Stream *in);
    RoomInfoError   ReadAnimBkgBlock(Stream *in);
    RoomInfoError   ReadScript3Block(Stream *in);
    RoomInfoError   ReadPropertiesBlock(Stream *in);
    RoomInfoError   ReadObjectScriptNamesBlock(Stream *in);
    void            ProcessAfterRead(bool game_is_hires);

    // Those are, in fact, are project-dependent and are implemented in runtime and AGS.Native
    void LoadScriptConfiguration(Common::Stream *in);
    void SaveScriptConfiguration(Common::Stream *out);
    // Graphical scripts are from very old AGS versions and not supported yet
    void LoadGraphicalScripts(Common::Stream *in);
    void SaveGraphicalScripts(Common::Stream *out);


// TODO: all members are currently public; hide them later
public:
    int32_t     GameId;
    // in 320x200 terms (scrolling room size)
    int16_t     Width;
    int16_t     Height;
    int16_t     Resolution; // 1 = 320x200, 2 = 640x400
    
    int16_t     LeftEdge;
    int16_t     RightEdge;
    int16_t     TopEdge;
    int16_t     BottomEdge;

    String                  Password;
    int8_t                  Options[10]; // [0]=startup music
    ObjectArray<String>     Messages;
    ObjectArray<MessageInfo> MessageInfos;
    int16_t                 LoadedVersion; // when loaded from file

    Bitmap      *WalkAreaMask;
    Bitmap      *WalkBehindMask;
    Bitmap      *HotspotMask;
    Bitmap      *RegionMask;
    color       Palette[256];
    
    ObjectArray<RoomObjectInfo> RoomObjects;
    Array<NewInteraction*>  RoomObjectInteractions;
    Array<InteractionScripts*> RoomObjectScripts;
    Array<int32_t>          RoomObjectBaselines; // or -1 (use bottom of object graphic)
    Array<int16_t>          RoomObjectFlags;
    ObjectArray<String>     RoomObjectNames;
    ObjectArray<String>     RoomObjectScriptNames;
    ObjectArray<CustomProperties> RoomObjectProperties;
    
    Array<int16_t>  WalkBehindBaselines;

    Array<int16_t>  WalkAreaShadingView; // walkable area-specific view number
    Array<int16_t>  WalkAreaZoom; // 0 = 100%, 1 = 101%, -1 = 99%
    Array<int16_t>  WalkAreaZoom2; // for vector scaled areas
    Array<int16_t>  WalkAreaLight; // 0 = normal, + lighter, - darker
    Array<int16_t>  WalkAreaTop; // top YP of area
    Array<int16_t>  WalkAreaBottom; // bottom YP of area
    // new version 2 roommake stuff below
    ObjectArray<PolyPoints> WallPoints; // not used anywhere
    
    ObjectArray<_Point>     HotspotWalkToPoints;
    ObjectArray<String>     HotspotNames;
    ObjectArray<String>     HotspotScriptNames;
    Array<NewInteraction*>  HotspotInteractions;
    Array<InteractionScripts*>  HotspotScripts;
    ObjectArray<CustomProperties> HotspotProperties;

    Array<int16_t>          RegionLightLevels;
    Array<int32_t>          RegionTintLevels;
    Array<NewInteraction*>  RegionInteractions;
    Array<InteractionScripts*> RegionScripts;

    NewInteraction*         RoomInteraction;
    InteractionScripts *    RoomScripts;
    
    char            *TextScripts;
    ccScript        *CompiledScript;
    bool            CompiledScriptShared;
    int32_t         CompiledScriptSize;
    
    int32_t         BkgSceneAnimSpeed;
    int32_t         BytesPerPixel;
    Array<Bitmap*>  BackgroundScenes;
    Array<color[256]> BkgScenePalettes;
    Array<char>     BkgScenePaletteShared;  // used internally by engine atm
    Array<InteractionVariable> LocalVariables;
    CustomProperties RoomProperties;

    // TODO: remove these, after refactoring the game load process
    // and making separate structs for every room entity type
    int16_t WalkBehindCount;
    int16_t RoomObjectCount;
    int16_t MessageCount;
    int16_t AnimationCount;
    int32_t WalkAreaCount;
    int32_t HotspotCount;
    int32_t RegionCount;
    int32_t BkgSceneCount;
    int32_t LocalVariableCount;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__AGSGAME_H
