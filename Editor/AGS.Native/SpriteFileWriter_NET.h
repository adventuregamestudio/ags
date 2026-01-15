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
#pragma once

#include <memory>
#include "ac/spritefile.h"
#include "SpriteFileReader_NET.h"

namespace AGS
{
namespace Native
{

public ref class SpriteFileWriter : public SpriteFile
{
public:
    SpriteFileWriter(System::String ^filename);
    ~SpriteFileWriter() { this->!SpriteFileWriter(); }
    !SpriteFileWriter();

    // Initializes new sprite file with the given settings
    void Begin(int store_flags, AGS::Types::SpriteCompression compress);
    // Writes bitmap into the file without any additional convertions
    void WriteBitmap(System::Drawing::Bitmap ^image);
    // Converts bitmap according to the sprite's properties, and writes into the file
    void WriteBitmap(System::Drawing::Bitmap ^image, AGS::Types::SpriteImportTransparency transparency,
        int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
    // Writes a native bitmap into the file without any additional convertions
    void WriteNativeBitmap(NativeBitmap ^bitmap);
    // Writes a raw sprite data presented in internal spritefile format
    void WriteRawData(RawSpriteData^ data);
    // Writes an empty slot marker
    void WriteEmptySlot();
    // Finalizes current format; no further writing is possible after this
    void End();

private:
    AGS::Common::SpriteFileWriter *_nativeWriter = nullptr;
};

} // namespace Native
} // namespace AGS
