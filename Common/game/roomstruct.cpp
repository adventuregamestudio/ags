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
#include "game/roomstruct.h"
#include "game/room_version.h"
#include "gfx/bitmap.h"

namespace AGS
{
namespace Common
{

RoomStruct::~RoomStruct()
{
    Free();
}

void RoomStruct::InitRuntimeData()
{
    for (size_t i = 0; i < (size_t)MAX_ROOM_BGFRAMES; ++i)
        BgImages[i].reset(new Bitmap(std::move(BgFrames[i].GraphicBuf)));
    HotspotMask.reset(new Bitmap(std::move(HotspotMaskBuf)));
    RegionMask.reset(new Bitmap(std::move(RegionMaskBuf)));
    WalkAreaMask.reset(new Bitmap(std::move(WalkAreaMaskBuf)));
    WalkBehindMask.reset(new Bitmap(std::move(WalkBehindMaskBuf)));
}

void RoomStruct::Free()
{
    RoomData::Free();

    for (size_t i = 0; i < (size_t)MAX_ROOM_BGFRAMES; ++i)
        BgImages[i].reset();
    HotspotMask.reset();
    RegionMask.reset();
    WalkAreaMask.reset();
    WalkBehindMask.reset();
}

Bitmap *RoomStruct::GetMask(RoomAreaMask mask) const
{
    switch (mask)
    {
    case kRoomAreaHotspot: return HotspotMask.get();
    case kRoomAreaWalkBehind: return WalkBehindMask.get();
    case kRoomAreaWalkable: return WalkAreaMask.get();
    case kRoomAreaRegion: return RegionMask.get();
    default: return nullptr;
    }
}

void RoomStruct::SetMask(RoomAreaMask mask, std::unique_ptr<Bitmap> &&bmp)
{
    switch (mask)
    {
    case kRoomAreaHotspot: HotspotMask.reset(bmp.release()); break;
    case kRoomAreaWalkBehind: WalkBehindMask.reset(bmp.release()); break;
    case kRoomAreaWalkable: WalkAreaMask.reset(bmp.release()); break;
    case kRoomAreaRegion: RegionMask.reset(bmp.release()); break;
    default: assert(0); break;
    }
}

void RoomStruct::CopyMask(RoomAreaMask mask, const Bitmap *bitmap)
{
    Bitmap *room_mask = GetMask(mask);
    if (room_mask)
    {
        room_mask->Clear(0); // in case sizes are different
        room_mask->Blit(bitmap);
    }
}


PBitmap FixBitmap(PBitmap bmp, int width, int height)
{
    Bitmap *new_bmp = BitmapHelper::AdjustBitmapSize(bmp.get(), width, height);
    if (new_bmp != bmp.get())
        return PBitmap(new_bmp);
    return bmp;
}

void UpscaleRoomBackground(RoomStruct *room, bool game_is_hires)
{
    if (room->DataVersion >= kRoomVersion_303b || !game_is_hires)
        return;
    for (size_t i = 0; i < room->BgFrameCount; ++i)
        room->BgImages[i] = FixBitmap(room->BgImages[i], room->Width, room->Height);
    FixRoomMasks(room);
}

void FixRoomMasks(RoomStruct *room)
{
    if (room->MaskResolution <= 0)
        return;
    Bitmap *bkg = room->BgImages[0].get();
    if (bkg == nullptr)
        return;
    // TODO: this issue is somewhat complicated. Original code was relying on
    // room->Width and Height properties. But in the engine these are saved
    // already converted to data resolution which may be "low-res". Since this
    // function is shared between engine and editor we do not know if we need
    // to upscale them.
    // For now room width/height is always equal to background bitmap.
    int base_width = bkg->GetWidth();
    int base_height = bkg->GetHeight();
    int low_width = base_width / room->MaskResolution;
    int low_height = base_height / room->MaskResolution;

    // Walk-behinds are always 1:1 of the primary background.
    // Other masks are 1:x where X is MaskResolution.
    room->WalkBehindMask = FixBitmap(room->WalkBehindMask, base_width, base_height);
    room->HotspotMask = FixBitmap(room->HotspotMask, low_width, low_height);
    room->RegionMask = FixBitmap(room->RegionMask, low_width, low_height);
    room->WalkAreaMask = FixBitmap(room->WalkAreaMask, low_width, low_height);
}

} // namespace Common
} // namespace AGS
