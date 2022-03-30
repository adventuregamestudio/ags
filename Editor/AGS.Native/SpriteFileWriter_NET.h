#pragma once

#include <memory>
#include "ac/spritefile.h"

namespace AGS
{
namespace Native
{

public ref class SpriteFileWriter
{
public:
    enum class StorageFlags
    {
        OptimizeForSize = 0x01
    };

    SpriteFileWriter(System::String ^filename);
    ~SpriteFileWriter();

    // Initializes new sprite file with the given settings
    void Begin(int store_flags, AGS::Types::SpriteCompression compress);
    // Writes bitmap into the file without any additional convertions
    void WriteBitmap(System::Drawing::Bitmap ^image);
    // Converts bitmap according to the sprite's properties, and writes into the file
    void WriteBitmap(System::Drawing::Bitmap ^image, AGS::Types::SpriteImportTransparency transparency,
        bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
    // Writes an empty slot marker
    void WriteEmptySlot();
    // Finalizes current format; no further writing is possible after this
    void End();

private:
    AGS::Common::SpriteFileWriter *_nativeWriter = nullptr;
};

} // namespace Native
} // namespace AGS
