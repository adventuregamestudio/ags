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
// TODO: actually, should move this to the Engine side.
// 
//=============================================================================
#ifndef __AGS_CN_GAME__ROOMSTRUCT_H
#define __AGS_CN_GAME__ROOMSTRUCT_H

#include "game/roomdata.h"
#include "gfx/bitmap.h"

#include "game/roomdata.h"
#include "gfx/bitmap.h"

namespace AGS
{
namespace Common
{

typedef std::shared_ptr<Bitmap> PBitmap;

// TODO: review this later, probably not a good idea to publicly inherit RoomData?
class RoomStruct : public RoomData
{
public:
    RoomStruct() = default;
    // Construct a RoomStruct object by copying data from RoomData
    RoomStruct(const RoomData &src);
    // Construct a RoomStruct object by moving (owning) data from RoomData
    RoomStruct(RoomData &&src);
    ~RoomStruct();

    // Reinitialize RoomStruct by copying data from RoomData
    RoomStruct &operator =(const RoomData &src);
    // Reinitialize RoomStruct by moving (owning) data from RoomData
    RoomStruct &operator =(RoomData &&src);

    // Initializes bitmaps from RoomData's pixel buffers.
    // CHECKME: this should be a private method; currently only accessed by a
    // (possibly temporary) code in the Editor.
    void    InitBitmaps();
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
    PBitmap BgImages[MAX_ROOM_BGFRAMES];

    // Region masks
    PBitmap HotspotMask;
    PBitmap RegionMask;
    PBitmap WalkAreaMask;
    PBitmap WalkBehindMask;
};


// Ensures that all existing room masks match room background size and
// MaskResolution property, resizes mask bitmaps if necessary.
void FixRoomMasks(RoomStruct *room);
// Adjusts bitmap size if necessary and returns either new or old bitmap.
PBitmap FixBitmap(PBitmap bmp, int dst_width, int dst_height);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__ROOMSTRUCT_H
