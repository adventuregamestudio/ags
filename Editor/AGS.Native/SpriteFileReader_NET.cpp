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
#include "SpriteFileReader_NET.h"
#include <allegro.h>
#include "NativeUtils.h"
#include "gfx/bitmap.h"
#include "util/file.h"

using AGSBitmap = AGS::Common::Bitmap;
using AGSString = AGS::Common::String;
using AGSStream = AGS::Common::Stream;

extern System::Drawing::Bitmap^ ConvertBlockToBitmap(AGS::Common::Bitmap *bitmap, bool useAlphaChannel);

namespace AGS
{
namespace Native
{

SpriteFileReader::SpriteFileReader(System::String ^filename)
{
    AGSString fn = TextHelper::ConvertUTF8(filename);
    std::unique_ptr<AGSStream> out(AGS::Common::File::OpenFileRead(fn));
    _nativeReader = new AGS::Common::SpriteFile();
    _metrics = new std::vector<Size>();
    _nativeReader->OpenFile(std::move(out), std::unique_ptr<AGSStream>(), *_metrics);
}

SpriteFileReader::SpriteFileReader(System::String ^spritesetFilename, System::String ^indexFilename)
{
    AGSString fn = TextHelper::ConvertUTF8(spritesetFilename);
    AGSString index_fn = TextHelper::ConvertUTF8(indexFilename);
    std::unique_ptr<AGSStream> out_sprfile(AGS::Common::File::OpenFileRead(fn));
    std::unique_ptr<AGSStream> out_index(AGS::Common::File::OpenFileRead(index_fn));
    _nativeReader = new AGS::Common::SpriteFile();
    _metrics = new std::vector<Size>();
    _nativeReader->OpenFile(std::move(out_sprfile), std::move(out_index), *_metrics);
}

SpriteFileReader::!SpriteFileReader()
{
    delete _nativeReader;
    delete _metrics;
}

System::Drawing::Bitmap ^SpriteFileReader::LoadSprite(int spriteIndex, bool useAlphaChannel)
{
    AGSBitmap *sprite;
    if (!_nativeReader->LoadSprite(spriteIndex, sprite))
        return nullptr;

    System::Drawing::Bitmap ^bmp = ConvertBlockToBitmap(sprite, useAlphaChannel);
    delete sprite;
    return bmp;
}

NativeBitmap ^SpriteFileReader::LoadSpriteAsNativeBitmap(int spriteIndex)
{
    AGSBitmap *sprite;
    if (!_nativeReader->LoadSprite(spriteIndex, sprite))
        return nullptr;

    return gcnew NativeBitmap(sprite);
}

SpriteFile::RawSpriteData ^SpriteFileReader::LoadSpriteAsRawData(int spriteIndex)
{
    RawSpriteData ^raw_data = gcnew RawSpriteData();
    if (!_nativeReader->LoadRawData(spriteIndex, *(raw_data->GetHeader()), *(raw_data->GetData())))
    {
        delete raw_data;
        return nullptr;
    }
    return raw_data;
}

bool SpriteFileReader::LoadSpriteAsRawData(int spriteIndex, RawSpriteData ^rawData)
{
    return _nativeReader->LoadRawData(spriteIndex, *(rawData->GetHeader()), *(rawData->GetData())).HasError();
}

} // namespace Native
} // namespace AGS
