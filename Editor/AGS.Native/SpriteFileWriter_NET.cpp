#include "SpriteFileWriter_NET.h"
#include <allegro.h>
#include "NativeUtils.h"
#include "gfx/bitmap.h"
#include "util/file.h"

using AGSBitmap = AGS::Common::Bitmap;
using AGSString = AGS::Common::String;
using AGSStream = AGS::Common::Stream;

extern AGSBitmap *CreateBlockFromBitmap(System::Drawing::Bitmap ^bmp, RGB *imgpal, bool fixColourDepth,
    bool keepTransparency, int *originalColDepth);
extern AGSBitmap *CreateNativeBitmap(System::Drawing::Bitmap ^bmp, int spriteImportMethod, bool remapColours,
    bool useRoomBackgroundColours, bool alphaChannel, int *flags);
extern void pre_save_sprite(AGSBitmap *image);

namespace AGS
{
namespace Native
{

SpriteFileWriter::SpriteFileWriter(System::String ^filename)
{
    AGSString fn = TextHelper::ConvertUTF8(filename);
    std::unique_ptr<AGSStream> out(AGS::Common::File::CreateFile(fn));
    _nativeWriter = new AGS::Common::SpriteFileWriter(std::move(out));
}

SpriteFileWriter::~SpriteFileWriter()
{
    delete _nativeWriter;
}

void SpriteFileWriter::Begin(int store_flags, AGS::Types::SpriteCompression compress)
{
    _nativeWriter->Begin(store_flags, (AGS::Common::SpriteCompression)compress);
}

void SpriteFileWriter::WriteBitmap(System::Drawing::Bitmap ^image)
{
    RGB imgPalBuf[256];
    int importedColourDepth;
    std::unique_ptr<AGSBitmap> native_bmp(CreateBlockFromBitmap(image, imgPalBuf, true, true, &importedColourDepth));
    pre_save_sprite(native_bmp.get()); // RGB swaps
    _nativeWriter->WriteBitmap(native_bmp.get());
}

void SpriteFileWriter::WriteBitmap(System::Drawing::Bitmap ^image, AGS::Types::SpriteImportTransparency transparency,
    bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
{
    std::unique_ptr<AGSBitmap> native_bmp(CreateNativeBitmap(image, (int)transparency, remapColours,
        useRoomBackgroundColours, alphaChannel, nullptr));
    pre_save_sprite(native_bmp.get()); // RGB swaps
    _nativeWriter->WriteBitmap(native_bmp.get());
}

void SpriteFileWriter::WriteEmptySlot()
{
    _nativeWriter->WriteEmptySlot();
}

void SpriteFileWriter::End()
{
    _nativeWriter->Finalize();
}

} // namespace Native
} // namespace AGS
