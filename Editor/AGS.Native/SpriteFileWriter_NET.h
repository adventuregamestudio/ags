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

public ref class SpriteFileIndex
{
public:
    SpriteFileIndex(const AGS::Common::SpriteFileIndex *index);
    ~SpriteFileIndex() { this->!SpriteFileIndex(); }
    !SpriteFileIndex();

    const AGS::Common::SpriteFileIndex *GetIndex() { return _index; }

private:
    AGS::Common::SpriteFileIndex *_index = nullptr;
};

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
    // Finalizes current format; no further writing is possible after this.
    // Returns sprite index object which may be used to write sprite index file.
    SpriteFileIndex ^End();

private:
    AGS::Common::SpriteFileWriter *_nativeWriter = nullptr;
};

static public ref class SpriteIndexFileWriter
{
public:
    static void WriteSpriteIndex(System::String ^filename, SpriteFileIndex ^index);
};

} // namespace Native
} // namespace AGS
