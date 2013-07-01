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
#include "ac/interaction.h"
#include "ac/messageinfo.h"
#include "ac/point.h"
#include "game/customproperties.h"
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

v.3.4.0 alpha - persistent flag; removed password
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
    kRoomVersion_340_alpha,
    kRoomVersion_Current    = kRoomVersion_340_alpha
};

enum RoomBaseOptions
{
    kRoomBaseOpt_StartUpMusic,
    kRoomBaseOpt_SaveLoadDisabled,
    kRoomBaseOpt_PlayerCharacterDisabled,
    kRoomBaseOpt_PlayerCharacterView,
    kRoomBaseOpt_MusicVolume,
    MAX_ROOM_BASE_OPTIONS   = 10
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

struct RoomBackgroundInfo
{
    Bitmap      *Graphic;
    color       Palette[256];
    char        PaletteShared; // used internally by engine atm

    RoomBackgroundInfo();
    RoomBackgroundInfo(const RoomBackgroundInfo &bkg_info);
    ~RoomBackgroundInfo();
};

struct RoomEdgeInfo
{
    int16_t Left;
    int16_t Right;
    int16_t Top;
    int16_t Bottom;
};

struct RoomHotspotInfo
{
    String          Name;
    String          ScriptName;
    CustomProperties Properties;
    EventHandler    EventHandlers; 

    _Point          WalkToPoint;
};

struct RoomObjectInfo
{
    int16_t         SpriteIndex;
    int16_t         X;
    int16_t         Y;
    int16_t         RoomIndex;
    bool            IsOn;

    EventHandler    EventHandlers;
    int32_t         Baseline; // or -1 (use bottom of object graphic)
    int16_t         Flags;
    String          Name;
    String          ScriptName;
    CustomProperties Properties;

    RoomObjectInfo();
    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out) const;
};

struct RoomRegionInfo
{
    String          Name;
    String          ScriptName;
    CustomProperties Properties;
    EventHandler    EventHandlers; 

    int16_t         Light;
    int32_t         Tint;

    RoomRegionInfo();
};

struct WalkAreaInfo
{
    int16_t     ShadingView; // walkable area-specific view number
    PolyPoints  WallPoints; // not used anywhere
    int16_t     Zoom;       // 0 = 100%, 1 = 101%, -1 = 99%
    int16_t     Zoom2;      // for vector scaled areas
    int16_t     Light;      // 0 = normal, + lighter, - darker
    int16_t     Top;        // top YP of area
    int16_t     Bottom;     // bottom YP of area

    WalkAreaInfo();
};

struct WalkBehindInfo
{
    int16_t Baseline;

    WalkBehindInfo();
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

    // TODO: temporarily made public
    void SaveScriptConfiguration(Common::Stream *out) const;
    void SaveGraphicalScripts(Common::Stream *out) const;
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
    
    // Graphical scripts are from very old AGS versions and not supported yet
    void LoadGraphicalScripts(Common::Stream *in);


// TODO: all members are currently public; hide them later
public:
    int16_t                 LoadedVersion; // when loaded from file
    int32_t                 GameId;

    // in 320x200 terms (scrolling room size)
    int16_t                 Width;
    int16_t                 Height;
    int16_t                 Resolution; // 1 = 320x200, 2 = 640x400
    int32_t                 BytesPerPixel;
    color                   Palette[256];
    
    ObjectArray<RoomBackgroundInfo> Backgrounds;
    int32_t                 BkgSceneAnimSpeed;
    RoomEdgeInfo            Edges;

    Bitmap                  *HotspotMask;
    Bitmap                  *RegionMask;
    Bitmap                  *WalkAreaMask;
    Bitmap                  *WalkBehindMask;
    ObjectArray<RoomHotspotInfo> Hotspots;
    ObjectArray<RoomObjectInfo>  Objects;
    ObjectArray<RoomRegionInfo>  Regions;
    ObjectArray<WalkAreaInfo>    WalkAreas;
    ObjectArray<WalkBehindInfo>  WalkBehinds;

    bool                    IsPersistent;
    int8_t                  Options[MAX_ROOM_BASE_OPTIONS];
    ObjectArray<String>     Messages;
    ObjectArray<MessageInfo> MessageInfos;

    EventHandler            EventHandlers;
    Array<InteractionVariable> LocalVariables;
    CustomProperties        Properties;

    char                    *TextScript;
    ccScript                *CompiledScript;
    bool                    CompiledScriptShared;
    int32_t                 CompiledScriptSize;

    // TODO: remove these, after refactoring the game load process
    // and making separate structs for every room entity type
    int16_t WalkBehindCount;
    int16_t ObjectCount;
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

#endif // __AGS_CN_GAME__AGSROOM_H
