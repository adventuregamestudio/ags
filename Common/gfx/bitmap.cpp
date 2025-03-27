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
#include <array>
#include <memory>
#include "gfx/bitmap.h"
#include "gfx/image_file.h"
#include "util/memory.h"
#include "util/memorystream.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

// TODO: revise this construction later
namespace BitmapHelper
{

Bitmap *CreateBitmap(int width, int height, int color_depth)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->Create(width, height, color_depth))
	{
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

Bitmap *CreateClearBitmap(int width, int height, int color_depth, int clear_color)
{
    Bitmap *bitmap = new Bitmap();
    if (!bitmap->Create(width, height, color_depth))
    {
        delete bitmap;
        return nullptr;
    }
    bitmap->Clear(clear_color);
    return bitmap;
}

Bitmap *CreateTransparentBitmap(int width, int height, int color_depth)
{
    Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateTransparent(width, height, color_depth))
	{
		delete bitmap;
        return nullptr;
	}
	return bitmap;
}

Bitmap *CreateBitmap(PixelBuffer &&pxbuf)
{
    Bitmap *bitmap = new Bitmap();
    if (!bitmap->Create(std::move(pxbuf)))
    {
        delete bitmap;
        return nullptr;
    }
    return bitmap;
}

Bitmap *CreateSubBitmap(Bitmap *src, const Rect &rc)
{
	Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateSubBitmap(src, rc))
	{
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

Bitmap *CreateBitmapCopy(const Bitmap *src, int color_depth)
{
    Bitmap *bitmap = new Bitmap();
	if (!bitmap->CreateCopy(src, color_depth))
	{
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

Bitmap *CreateBitmapFromPixels(int width, int height, int dst_color_depth,
    const uint8_t *pixels, const int src_col_depth, const int src_pitch)
{
    std::unique_ptr<Bitmap> bitmap(new Bitmap(width, height, dst_color_depth));
    if (!bitmap)
        return nullptr;

    if (!PixelOp::CopyConvert(bitmap->GetDataForWriting(), ColorDepthToPixelFormat(dst_color_depth),
            bitmap->GetLineLength(), height, pixels, ColorDepthToPixelFormat(src_col_depth), src_pitch))
        return nullptr;

    return bitmap.release();
}

Bitmap *LoadFromFile(const char *filename, int dst_color_depth)
{
    std::unique_ptr<Stream> in (
            File::OpenFile(filename, FileOpenMode::kFile_Open, StreamMode::kStream_Read));
    if(!in)
        return nullptr;

    return BitmapHelper::LoadBitmap(in.get(), Path::GetFileExtension(filename), dst_color_depth, nullptr);
}

Bitmap *AdjustBitmapSize(const Bitmap *src, int width, int height)
{
    int oldw = src->GetWidth(), oldh = src->GetHeight();
    if ((oldw == width) && (oldh == height))
        return const_cast<Bitmap*>(src);
    Bitmap *bmp = BitmapHelper::CreateBitmap(width, height, src->GetColorDepth());
    bmp->StretchBlt(src, RectWH(0, 0, oldw, oldh), RectWH(0, 0, width, height));
    return bmp;
}

// TODO: maybe merge this algorithm with ApplyMask found below
template <typename TPx>
static void ReplaceColorImpl(uint8_t *data, int width, int height, int pitch, const TPx color1, const TPx color2)
{
    for (int y = 0; y < height; ++y, data += pitch)
    {
        TPx *data_p = reinterpret_cast<TPx*>(data);
        for (int x = 0; x < width; ++x)
        {
            if (data_p[x] == color1)
                data_p[x] = color2;
        }
    }
}

void ReplaceColor(Bitmap *bmp, int color1, int color2)
{
    switch (bmp->GetColorDepth())
    {
    case 8:
        ReplaceColorImpl<uint8_t>(bmp->GetDataForWriting(),
                                   bmp->GetWidth(), bmp->GetHeight(), bmp->GetLineLength(), color1, color2);
        break;
    case 16:
        ReplaceColorImpl<uint16_t>(bmp->GetDataForWriting(),
                                   bmp->GetWidth(), bmp->GetHeight(), bmp->GetLineLength(), color1, color2);
        break;
    case 32:
        ReplaceColorImpl<uint32_t>(bmp->GetDataForWriting(),
                                   bmp->GetWidth(), bmp->GetHeight(), bmp->GetLineLength(), color1, color2);
        break;
    default: assert(false); break;
    }
}

void MakeOpaque(Bitmap *bmp)
{
    if (bmp->GetColorDepth() < 32)
        return; // no alpha channel

    for (int i = 0; i < bmp->GetHeight(); ++i)
    {
        uint32_t *line = reinterpret_cast<uint32_t*>(bmp->GetScanLineForWriting(i));
        uint32_t *line_end = line + bmp->GetWidth();
        for (uint32_t *px = line; px != line_end; ++px)
            *px = makeacol32(getr32(*px), getg32(*px), getb32(*px), 255);
    }
}

void MakeOpaqueSkipMask(Bitmap *bmp)
{
    if (bmp->GetColorDepth() < 32)
        return; // no alpha channel

    for (int i = 0; i < bmp->GetHeight(); ++i)
    {
        uint32_t *line = reinterpret_cast<uint32_t*>(bmp->GetScanLineForWriting(i));
        uint32_t *line_end = line + bmp->GetWidth();
        for (uint32_t *px = line; px != line_end; ++px)
            if (*px != MASK_COLOR_32)
                *px = makeacol32(getr32(*px), getg32(*px), getb32(*px), 255);
    }
}

void ReplaceAlphaWithRGBMask(Bitmap *bmp, int alpha_threshold)
{
    if (bmp->GetColorDepth() < 32)
        return; // no alpha channel

    for (int i = 0; i < bmp->GetHeight(); ++i)
    {
        uint32_t *line = reinterpret_cast<uint32_t*>(bmp->GetScanLineForWriting(i));
        uint32_t *line_end = line + bmp->GetWidth();
        for (uint32_t *px = line; px != line_end; ++px)
            if (geta32(*px) <= alpha_threshold)
                *px = MASK_COLOR_32;
    }
}


// Functor that copies the "mask color" pixels from source to dest
template <class TPx, size_t BPP_>
struct PixelTransCpy
{
    static const size_t BPP = BPP_;
    inline void operator ()(uint8_t *dst, const uint8_t *src, uint32_t mask_color, bool /*use_alpha*/) const
    {
        if (*(TPx*)src == mask_color)
            *(TPx*)dst = mask_color;
    }
};

// Functor that tells to never skip a pixel in the mask
struct PixelNoSkip
{
    inline bool operator ()(uint8_t * /*data*/, uint32_t /*mask_color*/, bool /*use_alpha*/) const
    {
        return false;
    }
};

typedef PixelTransCpy<uint8_t,  1> PixelTransCpy8;
typedef PixelTransCpy<uint16_t, 2> PixelTransCpy16;

// Functor that copies the "mask color" pixels from source to dest, 24-bit depth
struct PixelTransCpy24
{
    static const size_t BPP = 3;
    inline void operator ()(uint8_t *dst, const uint8_t *src, uint32_t mask_color, bool /*use_alpha*/) const
    {
        const uint8_t *mcol_ptr = (const uint8_t*)&mask_color;
        if (src[0] == mcol_ptr[0] && src[1] == mcol_ptr[1] && src[2] == mcol_ptr[2])
        {
            dst[0] = mcol_ptr[0];
            dst[1] = mcol_ptr[1];
            dst[2] = mcol_ptr[2];
        }
    }
};

// Functor that copies the "mask color" pixels from source to dest, 32-bit depth, with alpha
struct PixelTransCpy32
{
    static const size_t BPP = 4;
    inline void operator ()(uint8_t *dst, const uint8_t *src, uint32_t mask_color, bool use_alpha) const
    {
        if (*(const uint32_t*)src == mask_color)
            *(uint32_t*)dst = mask_color;
        else if (use_alpha)
            dst[3] =  src[3]; // copy alpha channel
        else
            dst[3] = 0xFF; // set the alpha channel byte to opaque
    }
};

// Functor that tells to skip pixels if they match the mask color or have alpha = 0
struct PixelTransSkip32
{
    inline bool operator ()(const uint8_t *data, uint32_t mask_color, bool use_alpha) const
    {
        return *(uint32_t*)data == mask_color || (use_alpha && data[3] == 0);
    }
};

// Applies bitmap mask, using 2 functors:
// - one that tells whether to skip current pixel;
// - another that copies the color from src to dest
template <class FnPxProc, class FnSkip>
void ApplyMask(uint8_t *dst, const uint8_t *src, size_t pitch, size_t height,
    FnPxProc proc, FnSkip skip, uint32_t mask_color, bool dst_has_alpha, bool mask_has_alpha)
{
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < pitch; x += FnPxProc::BPP, src += FnPxProc::BPP, dst += FnPxProc::BPP)
        {
            if (!skip(dst, mask_color, dst_has_alpha))
                proc(dst, src, mask_color, mask_has_alpha);
        }
    }
}

void CopyTransparency(Bitmap *dst, const Bitmap *mask, bool dst_has_alpha, bool mask_has_alpha)
{
    color_t mask_color     = mask->GetMaskColor();
    uint8_t *dst_ptr       = dst->GetDataForWriting();
    const uint8_t *src_ptr = mask->GetData();
    const size_t bpp       = mask->GetBPP();
    const size_t pitch     = mask->GetLineLength();
    const size_t height    = mask->GetHeight();

    if (bpp == 1)
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy8(),  PixelNoSkip(), mask_color, dst_has_alpha, mask_has_alpha);
    else if (bpp == 2)
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy16(), PixelNoSkip(), mask_color, dst_has_alpha, mask_has_alpha);
    else if (bpp == 3)
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy24(), PixelNoSkip(), mask_color, dst_has_alpha, mask_has_alpha);
    else
        ApplyMask(dst_ptr, src_ptr, pitch, height, PixelTransCpy32(), PixelTransSkip32(), mask_color, dst_has_alpha, mask_has_alpha);
}

void ReadPixelsFromMemory(Bitmap *dst, const uint8_t *src_buffer, const size_t src_pitch, const size_t src_px_offset)
{
    PixelOp::CopyPixels(dst->GetDataForWriting(), dst->GetLineLength(), 0u,
        dst->GetBPP(), dst->GetHeight(), src_buffer, src_pitch, src_px_offset);
}

// Converts loaded bitmap to the requested color depth,
// optionally fixups color palette
static Bitmap *BitmapColorDepthFixup(Bitmap *bmp, int dst_color_depth, RGB *pal, bool src_has_alpha)
{
    int bmp_depth = bmp->GetColorDepth();
    int dest_depth = get_color_load_depth(bmp_depth, false);
    if (dest_depth != bmp_depth)
    {
        /* restore original palette except if it comes from the bitmap */
        if (bmp_depth != 8)
            pal = nullptr;

        BITMAP *al_bmp = fixup_loaded_bitmap(bmp->GetAllegroBitmap(), pal, dest_depth);
        if (!al_bmp)
            return nullptr;
        if (al_bmp != bmp->GetAllegroBitmap())
            bmp->WrapAllegroBitmap(al_bmp, false);
    }

    if ((dest_depth == 32) && !src_has_alpha)
        BitmapHelper::MakeOpaqueSkipMask(bmp);

    /* construct a fake palette if 8-bit mode is not involved */
    if ((bmp_depth != 8) && (dest_depth != 8) && pal)
        generate_332_palette(pal);

    return bmp;
}

Bitmap* LoadBitmap(Stream *in, const String& ext, int dst_color_depth, RGB *pal)
{
    PALETTE tmppal;
    /* we really need a palette */
    if (!pal)
        pal = tmppal;

    PixelBuffer pxbuf = ImageFile::LoadImage(in, ext, pal);
    if (!pxbuf)
        return nullptr;
    Bitmap *bmp = BitmapHelper::CreateBitmap(std::move(pxbuf));
    if (!bmp)
        return nullptr;
    // Perform color depth and palette fixups
    Bitmap *fixed_bmp = BitmapColorDepthFixup(bmp, dst_color_depth, pal, pxbuf.HasAlphaChannel());
    if (fixed_bmp != bmp)
        delete bmp;
    return fixed_bmp;
}

bool SaveBitmap(const Bitmap *bmp, const RGB* pal, Stream *out, const String& ext)
{
    return ImageFile::SaveImage(bmp->GetBitmapData(), pal, out, ext);
}

bool SaveToFile(const Bitmap* bmp, const char *filename, const RGB *pal)
{
    std::unique_ptr<Stream> out (
            File::OpenFile(filename, FileOpenMode::kFile_CreateAlways, StreamMode::kStream_Write));
    if (!out)
        return false;

    return SaveBitmap(bmp, pal, out.get(), Path::GetFileExtension(filename));
}

} // namespace BitmapHelper

} // namespace Common
} // namespace AGS
