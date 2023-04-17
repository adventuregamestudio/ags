#include "NativeRoom.h"
#include "NativeUtils.h"
#include "game/room_file.h"

using AGSBitmap = AGS::Common::Bitmap;
using AGSString = AGS::Common::String;
using RoomStruct = AGS::Common::RoomStruct;
using AGS::Types::Room;
using AGS::Types::RoomAreaMaskType;
using AGS::Types::AGSEditorException;
using SysBitmap = System::Drawing::Bitmap;


AGSBitmap *CreateBlockFromBitmap(SysBitmap ^bmp, RGB *imgpal, bool fixColourDepth, bool keepTransparency, int *originalColDepth);
extern SysBitmap^ ConvertBlockToBitmap32(AGSBitmap *todraw, int width, int height);
extern SysBitmap^ ConvertBlockToBitmap(AGSBitmap *todraw);
extern void convert_room_from_native(const RoomStruct &rs, AGS::Types::Room ^room, System::Text::Encoding ^defEncoding);
extern void convert_room_to_native(Room ^room, RoomStruct &rs);
extern AGSString load_room_file(RoomStruct &rs, const AGSString &filename);
extern void save_room_file(RoomStruct &rs, const AGSString &path);
extern void SetNativeRoomBackground(RoomStruct &room, int backgroundNumber,
    SysBitmap ^bmp, bool useExactPalette, bool sharePalette);
extern void validate_mask(AGSBitmap *toValidate, const char *name, int maxColour);


namespace AGS
{
namespace Native
{

NativeRoom::NativeRoom(AGS::Types::Room ^room)
{
    _rs = new RoomStruct();
    convert_room_to_native(room, *_rs);
}

NativeRoom::NativeRoom(System::String ^filename, System::Text::Encoding ^defEncoding)
{
    _rs = new RoomStruct();
    AGSString roomFileName = TextHelper::ConvertUTF8(filename);
    AGSString errorMsg = load_room_file(*_rs, roomFileName);
    if (!errorMsg.IsEmpty())
    {
        delete _rs;
        throw gcnew AGSEditorException(TextHelper::ConvertUTF8(errorMsg));
    }
}

NativeRoom::~NativeRoom()
{
    delete _rs;
}

AGS::Types::Room^ NativeRoom::ConvertToManagedRoom(int roomNumber, System::Text::Encoding ^defEncoding)
{
    Room ^room = gcnew Room(roomNumber);
    convert_room_from_native(*_rs, room, defEncoding);
    return room;
}

SysBitmap ^NativeRoom::GetBackground(int bgnum)
{
    if (bgnum < 0 || bgnum > MAX_ROOM_BGFRAMES)
    {
        throw gcnew AGSEditorException(System::String::Format(
            "Invalid background number {0}", bgnum));
    }
    return ConvertBlockToBitmap32(_rs->BgFrames[bgnum].Graphic.get(), _rs->Width, _rs->Height);
}

SysBitmap ^NativeRoom::GetAreaMask(AGS::Types::RoomAreaMaskType maskType)
{
    RoomAreaMask nativeType = (RoomAreaMask)maskType;
    if (nativeType < kRoomArea_First || nativeType > kRoomArea_Last)
    {
        throw gcnew AGSEditorException(System::String::Format("Invalid mask type {0}", (int)nativeType));
    }

    AGSBitmap *mask = _rs->GetMask(nativeType);
    SysBitmap^ managedMask = ConvertBlockToBitmap(mask);
    // Palette entry 0 alpha value is hardcoded to 0, probably because it's convenient
    // for rendering area 0 as invisible? However it creates issues when exporting 8-bit
    // image with transparency to different image formats (tested with .png). To make things
    // easier we take the alpha value out.
    auto palette = managedMask->Palette;
    palette->Entries[0] = System::Drawing::Color::FromArgb(255, palette->Entries[0]);
    managedMask->Palette = palette;
    return managedMask;
}

void NativeRoom::SetBackground(int bgnum, SysBitmap ^bmp, bool useExactPalette, bool sharePalette)
{ // NOTE: this operation uses too much from the native code to completely move here 
    SetNativeRoomBackground(*_rs, bgnum, bmp, useExactPalette, sharePalette);
}

void NativeRoom::SetAreaMask(AGS::Types::RoomAreaMaskType maskType, SysBitmap ^bmp)
{
    RoomAreaMask nativeType = (RoomAreaMask)maskType;
    if (nativeType < kRoomArea_First || nativeType > kRoomArea_Last)
    {
        throw gcnew AGSEditorException(System::String::Format("Invalid mask type {0}", (int)nativeType));
    }

    // Palette entry 0 alpha value is hardcoded to 0 originally, probably because it's
    // convenient for rendering area 0 as invisible? This was taken out when converting
    // the room to open format because it created issues with saving/loading to disk
    // in certain image formats (.png). Just in case we set the alpha back to 0 when
    // setting the mask back into crm on the off chance it might be used for something
    // internally somewhere
    auto palette = bmp->Palette;
    palette->Entries[0] = System::Drawing::Color::FromArgb(255, palette->Entries[0]);
    bmp->Palette = palette;

    RGB pal[256]; // dummy, used as a return value from CreateBlockFromBitmap
    AGSBitmap* newMask = CreateBlockFromBitmap(bmp, pal, false, false, nullptr);
    validate_mask(newMask, "imported", (nativeType == kRoomAreaHotspot) ? MAX_ROOM_HOTSPOTS : (MAX_WALK_AREAS + 1));
    _rs->SetMask(nativeType, newMask);
}

void NativeRoom::SaveToFile(System::String ^filename)
{
    AGSString roomFileName = TextHelper::ConvertUTF8(filename);
    save_room_file(*_rs, roomFileName);
}

} // namespace Native
} // namespace AGS
