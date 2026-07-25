//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// RoomStruct, the Room data class prepared for use at runtime.
//
//=============================================================================
#ifndef __AGS_CN_GAME__ROOMSTRUCT_H
#define __AGS_CN_GAME__ROOMSTRUCT_H

#include "game/roomdata.h"
#include "gfx/bitmap.h"

namespace AGS
{
namespace Common
{

typedef std::shared_ptr<Bitmap> PBitmap;

class RoomStruct : public RoomData
{
public:
    RoomStruct() = default;
    ~RoomStruct();

    // Init runtime room resources
    void    InitRuntimeData();
    // Releases room resources
    void    Free();

    // Gets bitmap of particular mask layer
    Bitmap *GetMask(RoomAreaMask mask) const;
    // Assigns bitmap for the particular mask layer
    void    SetMask(RoomAreaMask mask, std::unique_ptr<Bitmap> &&bmp);
    // Copies contents of a provided bitmap onto the particular mask layer;
    // this is done by blitting; if bitmap is of different size than the room's mask,
    // then it's either cropped or remaining unfilled parts are erased to zero.
    void    CopyMask(RoomAreaMask mask, const Bitmap *bitmap);

    // Background bitmaps
    PBitmap                 BgImages[MAX_ROOM_BGFRAMES];

    // Region masks
    PBitmap                 HotspotMask;
    PBitmap                 RegionMask;
    PBitmap                 WalkAreaMask;
    PBitmap                 WalkBehindMask;
};

// Checks if it's necessary and upscales low-res room backgrounds and masks for the high resolution game
// NOTE: it does not upscale object coordinates, because that is usually done when the room is loaded
void UpscaleRoomBackground(RoomStruct *room, bool game_is_hires);
// Ensures that all existing room masks match room background size and
// MaskResolution property, resizes mask bitmaps if necessary.
void FixRoomMasks(RoomStruct *room);
// Adjusts bitmap size if necessary and returns either new or old bitmap.
PBitmap FixBitmap(PBitmap bmp, int dst_width, int dst_height);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__ROOMSTRUCT_H
