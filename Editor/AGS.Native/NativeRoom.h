#pragma once

#include <memory>
#include "game/roomstruct.h"

namespace AGS
{
namespace Native
{

// Wraps a native Room object, provides means for converting into .NET object equivalents
public ref class NativeRoom
{
public:
    // Constructs native room object from the managed Room
    NativeRoom(AGS::Types::Room ^room);
    // Constructs native room object from the file (CRM)
    NativeRoom(System::String ^filename, System::Text::Encoding ^defEncoding);
    ~NativeRoom();

    // Generates a managed Room object from the room data;
    // this does not include backgrounds and masks, they are retrieved using
    // separate respective methods.
    AGS::Types::Room^ ConvertToManagedRoom(int roomNumber, System::Text::Encoding ^defEncoding);
    // Gets respective room background as a .NET Bitmap
    System::Drawing::Bitmap ^GetBackground(int bgnum);
    // Gets respective area mask as a .NET Bitmap
    System::Drawing::Bitmap ^GetAreaMask(AGS::Types::RoomAreaMaskType maskType);
    // Sets/replaces room background, converting from .NET Bitmap
    void SetBackground(int bgnum, System::Drawing::Bitmap ^bmp, bool useExactPalette, bool sharePalette);
    // Sets/replaces area mask, converting from .NET Bitmap
    void SetAreaMask(AGS::Types::RoomAreaMaskType maskType, System::Drawing::Bitmap ^bmp);
    // Saves current room object to the file (CRM)
    void SaveToFile(System::String ^filename);

private:
    AGS::Common::RoomStruct *_rs = nullptr;
};

} // namespace Native
} // namespace AGS
