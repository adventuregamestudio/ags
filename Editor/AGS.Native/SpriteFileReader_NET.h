//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

namespace AGS
{
namespace Native
{

public ref class SpriteFile
{
public:
    enum class StorageFlags
    {
        OptimizeForSize = 0x01
    };

    ref class RawSpriteData
    {
    public:
        RawSpriteData()
        {
            _hdr = new AGS::Common::SpriteDatHeader();
            _data = new std::vector<uint8_t>();
        }

        ~RawSpriteData()
        {
            this->!RawSpriteData();
        }

        !RawSpriteData()
        {
            delete _hdr;
            delete _data;
        }

        AGS::Common::SpriteDatHeader *GetHeader()
        {
            return _hdr;
        }

        std::vector<uint8_t> *GetData()
        {
            return _data;
        }

    private:
        AGS::Common::SpriteDatHeader *_hdr = nullptr;
        std::vector<uint8_t> *_data = nullptr;
    };
};

public ref class NativeBitmap
{
public:
    NativeBitmap() {}
    NativeBitmap(AGS::Common::Bitmap *bitmap)
        : _bitmap(bitmap) {}

    ~NativeBitmap()
    {
        this->!NativeBitmap();
    }

    !NativeBitmap()
    {
        delete _bitmap;
    }

    AGS::Common::Bitmap *GetNativePtr()
    {
        return _bitmap;
    }

private:
    AGS::Common::Bitmap *_bitmap = nullptr;
};

public ref class SpriteFileReader : public SpriteFile
{
public:
    SpriteFileReader(System::String ^filename);
    SpriteFileReader(System::String ^spritesetFilename, System::String ^indexFilename);
    ~SpriteFileReader() { this->!SpriteFileReader(); }
    !SpriteFileReader();

    System::Drawing::Bitmap ^LoadSprite(int spriteIndex);
    NativeBitmap  ^LoadSpriteAsNativeBitmap(int spriteIndex);
    RawSpriteData ^LoadSpriteAsRawData(int spriteIndex);
    bool           LoadSpriteAsRawData(int spriteIndex, RawSpriteData ^rawData);

private:
    AGS::Common::SpriteFile *_nativeReader = nullptr;
    std::vector<AGS::Common::GraphicResolution> *_metrics = nullptr;
};

} // namespace Native
} // namespace AGS
