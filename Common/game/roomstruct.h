//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// RoomStruct, a class describing initial room data.
//
// Because of the imperfect implementation there is inconsistency in how
// this data is interpreted at the runtime. 
// Some of that data is never supposed to be changed at runtime. Another
// may be changed, but these changes are lost as soon as room is unloaded.
// The changes that must remain in memory are kept as separate classes:
// see RoomStatus, RoomObject etc.
// 
// Partially this is because same class was used for both engine and editor,
// while runtime code was not available for the editor.
//
// This is also the reason why some classes here are named with the "Info"
// postfix. For example, RoomObjectInfo is the initial object data, and
// there is also RoomObject runtime-only class for mutable data.
//
//=============================================================================
#ifndef __AGS_CN_GAME__ROOMINFO_H
#define __AGS_CN_GAME__ROOMINFO_H
#include <memory>
#include <allegro.h> // RGB
#include "ac/common_defines.h"
#include "game/scripteventtable.h"
#include "gfx/gfx_def.h"
#include "script/cc_script.h"
#include "util/error.h"
#include "util/geometry.h"
#include "util/string_types.h"

// TODO: move the following enums under AGS::Common namespace
// later, when more engine source is put in AGS namespace and
// refactored.

// Room's area mask type
enum RoomAreaMask
{
    kRoomAreaNone = 0,
    kRoomAreaHotspot,
    kRoomAreaWalkBehind,
    kRoomAreaWalkable,
    kRoomAreaRegion,
    kNumRoomAreaTypes,

    kRoomArea_First = kRoomAreaHotspot,
    kRoomArea_Last  = kRoomAreaRegion
};

// Extended room boolean options
enum RoomFlags
{
    kRoomFlag_BkgFrameLocked = 0x01
};

// Flag tells that walkable area does not have continious zoom
#define NOT_VECTOR_SCALED  -10000
// Flags tells that room is not linked to particular game ID
#define NO_GAME_ID_IN_ROOM_FILE 16325

#define MAX_ROOM_BGFRAMES  5   // max number of frames in animating bg scene

#define MAX_ROOM_HOTSPOTS  50  // v2.62: 20 -> 30; v2.8: -> 50
#define MAX_ROOM_OBJECTS_v300 40 // for some legacy logic support
#define MAX_ROOM_OBJECTS   256 // v3.6.0: 40 -> 256 (now limited by room format)
#define MAX_ROOM_REGIONS   16
#define MAX_WALK_AREAS     16
#define MAX_WALK_BEHINDS   16

#define MAX_MESSAGES       100
// Max length of a serialized room message prior to 2.61
#define MAX_MESSAGE_PRE261_LEN 3000


namespace AGS
{
namespace Common
{

class Bitmap;
class Stream;

typedef std::shared_ptr<Bitmap> PBitmap;

// Room event indexes;
// these are used after resolving events map read from room file
enum RoomEventID
{
    // room edge crossing
    kRoomEvent_EdgeLeft = 0,
    kRoomEvent_EdgeRight = 1,
    kRoomEvent_EdgeBottom = 2,
    kRoomEvent_EdgeTop = 3,
    // first time enters room
    kRoomEvent_FirstEnter = 4,
    // load room; aka before fade-in
    kRoomEvent_BeforeFadein = 5,
    // room's rep-exec
    kRoomEvent_Repexec = 6,
    // after fade-in
    kRoomEvent_AfterFadein = 7,
    // leave room (before fade-out)
    kRoomEvent_BeforeFadeout = 8,
    // unload room; aka after fade-out
    kRoomEvent_AfterFadeout = 9,
};

// Hotspot event indexes
enum HotspotEventID
{
    // an interaction with any cursor mode that normally has a event
    kHotspotEvent_AnyClick = 0,
    // cursor is over hotspot
    kHotspotEvent_MouseOver = 1,
    // player stands on hotspot
    kHotspotEvent_StandOn = 2,
};

// Room object event indexes
enum RoomObjectEventID
{
    // an interaction with any cursor mode that normally has a event
    kRoomObjectEvent_AnyClick = 0,
    kRoomObjectEvent_OnFrameEvent
};

// Region event indexes
enum RegionEventID
{
    kRegionEvent_Standing = 0,
    kRegionEvent_WalkOn = 1,
    kRegionEvent_WalkOff = 2,
};

// Various room options
struct RoomOptions
{
    // If player character is turned off in the room
    bool PlayerCharOff;
    // Apply player character's normal view when entering this room
    int  PlayerView;
    // Optional character facing dir ratio (y / x), 0 = ignore
    float FaceDirectionRatio;
    // A collection of RoomFlags
    int  Flags;

    RoomOptions();
};

// Single room background frame
struct RoomBgFrame
{
    PBitmap     Graphic;
    // Palette is only valid in 8-bit games
    RGB         Palette[256];
    // Tells if this frame should keep previous frame palette instead of using its own
    bool        IsPaletteShared;

    RoomBgFrame();
};

// Describes room edges (coordinates of four edges)
struct RoomEdges
{
    int32_t Left;
    int32_t Right;
    int32_t Top;
    int32_t Bottom;

    RoomEdges();
    RoomEdges(int l, int r, int t, int b);
};

// A (possibly) temporary struct made for sharing event tables;
// replace or expand later. Also, we have a similar thing for GUIObject,
// so maybe merge these too.
struct RoomObjectBase
{
public:
    RoomObjectBase(const ScriptEventSchema *schema)
        : _events(schema)
    {}

    // Provides a script events table
    const ScriptEventTable &GetEvents() const { return _events; }
    AGS::Common::ScriptEventTable &GetEvents() { return _events; }
    // Clears all handlers from assigned functions
    void ClearEventHandlers() { _events.ClearHandlers(); }

protected:
    // Common events
    ScriptEventTable _events = {};
};

// Room hotspot description
struct RoomHotspot : public RoomObjectBase
{
    String      Name;
    String      ScriptName;
    // Custom properties
    StringIMap  Properties;
    // Interaction events (cursor-based)
    ScriptEventHandlers Interactions = {};

    // Player will automatically walk here when interacting with hotspot
    Point       WalkTo;

    RoomHotspot() : RoomObjectBase(&RoomHotspot::_eventSchema)
    {
    }

    static ScriptEventSchema &GetEventSchema() { return _eventSchema; }
    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

private:
    // Script events schema
    static ScriptEventSchema _eventSchema;
};

// Room object description
struct RoomObjectInfo : public RoomObjectBase
{
    int32_t         Room;
    int32_t         X;
    int32_t         Y;
    int32_t         Sprite;
    Common::BlendMode BlendMode;
    // Object's z-order in the room, or -1 (use Y)
    int32_t         Baseline;
    int32_t         Flags;
    String          Name;
    String          ScriptName;
    // Custom properties
    StringIMap      Properties;
    // Interaction events (cursor-based)
    ScriptEventHandlers Interactions = {};

    RoomObjectInfo();

    static ScriptEventSchema &GetEventSchema() { return _eventSchema; }
    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

private:
    // Script events schema
    static ScriptEventSchema _eventSchema;
};

// Room region description
struct RoomRegion : public RoomObjectBase
{
    // Light level (-100 -> +100) or Tint luminance (0 - 255)
    int32_t         Light;
    // Tint setting (R-B-G-S)
    int32_t         Tint;
    // Custom properties
    StringIMap      Properties;
    // Interaction events (old-style event storage, kept of loading old data)
    ScriptEventHandlers Interactions = {};

    RoomRegion();

    static ScriptEventSchema &GetEventSchema() { return _eventSchema; }
    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

private:
    // Script events schema
    static ScriptEventSchema _eventSchema;
};

// Walkable area description
struct WalkArea
{
    // Apply player character's normal view on this area
    int32_t     CharacterView;
    // Character's scaling (-100 -> +100 %)
    // General scaling, or scaling at the farthest point
    int32_t     ScalingFar;
    // Scaling at the nearest point, or NOT_VECTOR_SCALED for uniform scaling
    int32_t     ScalingNear;
    // Optional override for player character view
    int32_t     PlayerView;
    // Optional character face direction ratio, 0 = ignore
    float       FaceDirectionRatio;
    // Top and bottom Y of the area
    int32_t     Top;
    int32_t     Bottom;
    // Custom properties
    StringIMap  Properties;

    WalkArea();
};

// Walk-behind description
struct WalkBehind
{
    // Object's z-order in the room
    int32_t Baseline;

    WalkBehind();
};

// Room's legacy resolution type
// The meaning of this value is bit complicated. In a usual case, it seems,
// it should be either 1 or 2, meaning low-res or high-res, in the same
// sense as the legacy game resolution may be low-res or high-res type.
// If game's resolution type is different, the room's background will have
// to be adjusted for it by scaling up or down correspondingly.
// But rare games could have it higher than 2, which would mean "above
// high res", in which case the room bg would need to be downscaled
// even though the game is already high-res.
enum RoomResolutionType
{
    kRoomResolution_Real        = 0, // room should always be treated as-is
    kRoomResolution_Low         = 1, // created for low-resolution game
    kRoomResolution_High        = 2, // created for high-resolution game
    kRoomResolution_OverHigh    = 3, // created for high-res game, but bigger (must downscale)
};

//
// Description of a single room.
// This class contains initial room data. Some of it may still be modified
// at the runtime, but then these changes get lost as soon as room is unloaded.
//
class RoomStruct : public RoomObjectBase
{
public:
    // Mask resolution auto-assigned for high-res rooms in very old versions
    static const int LegacyMaskHiresFactor = 2;

    RoomStruct();
    ~RoomStruct();

    // Releases room resources
    void    Free();
    // Init default room state
    void    InitDefaults();

    static const ScriptEventSchema &GetEventSchema() { return _eventSchema; }
    // Remaps old-format interaction list into new event table
    void    RemapOldInteractions();

    // Gets this room's human-readable name (description)
    const String &GetName() const { return Name; }
    // Gets this room's script name
    const String &GetScriptName() const { return ScriptName; }

    // Gets bitmap of particular mask layer
    Bitmap *GetMask(RoomAreaMask mask) const;
    // Sets bitmap for the particular mask layer;
    // WARNING: takes the ownership of the given bitmap
    void    SetMask(RoomAreaMask mask, Bitmap *bmp);
    // Gets mask's scale relative to the room's background size
    float   GetMaskScale(RoomAreaMask mask) const;
    // Replaces contents of a bitmap for the particular mask layer
    void    SetMask(RoomAreaMask mask, const Bitmap *bitmap);

    // TODO: see later whether it may be more convenient to move these to the Region class instead.
    // Gets if the given region has light level set
    bool HasRegionLightLevel(int id) const;
    // Gets if the given region has a tint set
    bool HasRegionTint(int id) const;
    // Gets region's light level in -100 to 100 range value; returns 0 (default level) if region's tint is set
    int  GetRegionLightLevel(int id) const;
    // Gets region's tint luminance in 0 to 100 range value; returns 0 if region's light level is set
    int  GetRegionTintLuminance(int id) const;

// TODO: all members are currently public because they are used everywhere; hide them later
public:
    // Game's unique ID, corresponds to GameSetupStructBase::uniqueid.
    // If this field has a valid value and does not match actual game's id,
    // then engine will refuse to start this room.
    // May be set to NO_GAME_ID_IN_ROOM_FILE to let it run within any game.
    int32_t                 GameID;
    // Loaded room file's data version. This value may be used to know when
    // the room must have behavior specific to certain version of AGS.
    int32_t                 DataVersion;

    // This room's name (description)
    String                  Name;
    // This room's scriptname. This is a reserved field atm.
    String                  ScriptName;
    // Room region masks resolution. Defines the relation between room and mask units.
    // Mask point is calculated as roompt / MaskResolution. Must be >= 1.
    int32_t                 MaskResolution;
    // Size of the room, in logical coordinates (= pixels)
    int32_t                 Width;
    int32_t                 Height;
    // Primary room palette (8-bit games)
    RGB                     Palette[256];

    // Basic room options
    RoomOptions             Options;

    // Background frames
    int32_t                 BackgroundBPP; // bytes per pixel
    uint32_t                BgFrameCount;
    RoomBgFrame             BgFrames[MAX_ROOM_BGFRAMES];
    // Speed at which background frames are changing, 0 - no auto animation
    int32_t                 BgAnimSpeed;
    // Edges
    RoomEdges               Edges;
    // Region masks
    PBitmap                 HotspotMask;
    PBitmap                 RegionMask;
    PBitmap                 WalkAreaMask;
    PBitmap                 WalkBehindMask;
    // Room entities
    uint32_t                HotspotCount;
    RoomHotspot             Hotspots[MAX_ROOM_HOTSPOTS];
    std::vector<RoomObjectInfo> Objects;
    uint32_t                RegionCount;
    RoomRegion              Regions[MAX_ROOM_REGIONS];
    uint32_t                WalkAreaCount;
    WalkArea                WalkAreas[MAX_WALK_AREAS];
    uint32_t                WalkBehindCount;
    WalkBehind              WalkBehinds[MAX_WALK_BEHINDS];

    // Custom properties
    StringIMap              Properties;
    // Interaction events (old-style event storage, kept of loading old data)
    ScriptEventHandlers     Interactions = {};
    // Compiled room script
    UScript                 CompiledScript;
    // Various extended options with string values, meta-data etc
    StringMap               StrOptions;

private:
    // Script events schema
    static ScriptEventSchema _eventSchema;
};


// Ensures that all existing room masks match room background size and
// MaskResolution property, resizes mask bitmaps if necessary.
void FixRoomMasks(RoomStruct *room);
// Adjusts bitmap size if necessary and returns either new or old bitmap.
PBitmap FixBitmap(PBitmap bmp, int dst_width, int dst_height);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__ROOMINFO_H
