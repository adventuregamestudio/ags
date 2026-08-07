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
#include "data/sprite_utils.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/string_types.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

static const String DefaultPattern = "spr%06d";
static const String DefaultRegexPattern = "spr\\d{6}";
static const String DefaultExtension = "png";
static const CstrArr<kNumSprCompressTypes> CompressionNames = {{"none", "rle", "lzw", "deflate"}};

String GetCompressionName(SpriteCompression compress)
{
    return String::Wrapper(StrUtil::SelectCStr<kNumSprCompressTypes>(
        CompressionNames, compress, "unknown"));
}

SpriteCompression CompressionFromName(const String &compress_name)
{
    return StrUtil::ParseEnum<SpriteCompression, kNumSprCompressTypes>(
        compress_name, CompressionNames, kSprCompress_None);
}

bool ResolveImageFilePattern(const String &pattern, String &res_pattern)
{
    String regex_pattern;
    return ResolveImageFilePattern(pattern, res_pattern, regex_pattern);
}

bool ResolveImageFilePattern(const String &pattern, String &res_pattern, String &regex_pattern)
{
    if (pattern.IsNullOrSpace())
    {
        res_pattern = String::FromFormat("%s.%s", DefaultPattern.GetCStr(), DefaultExtension.GetCStr());
        regex_pattern = String::FromFormat("%s.%s", DefaultRegexPattern.GetCStr(), DefaultExtension.GetCStr());
        return true;
    }

    res_pattern = "";
    regex_pattern = "";
    bool found_spritenum = false;
    const auto parts = pattern.Split('%');
    // Each second part is assumed a placeholder between two '%'
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i % 2 == 0)
        {
            res_pattern.Append(parts[i]);
            regex_pattern.Append(parts[i]);
        }
        else if (i < parts.size() - 1)
        {
            // Resolve placeholder
            if ((parts[i].GetLast() == 'N' || parts[i].GetLast() == 'n')
                && (parts[i].GetLength() == 2) && std::isdigit(parts[i][0]))
            {
                res_pattern.AppendFmt("%%0%dd", parts[i][0] - '0');
                regex_pattern.AppendFmt("\\d{%d}", parts[i][0] - '0');
                found_spritenum = true;
            }
            else
            {
                res_pattern.Append("_");
                regex_pattern.Append("_");
            }
        }
    }

    // We cannot export multiple sprites without a sprite number placeholder
    if (!found_spritenum)
        return false;

    if (Path::GetFileExtension(res_pattern).IsEmpty())
    {
        res_pattern = String::FromFormat("%s.%s", res_pattern.GetCStr(), DefaultExtension.GetCStr());
        regex_pattern = String::FromFormat("%s.%s", regex_pattern.GetCStr(), DefaultExtension.GetCStr());
    }
    return true;
}

HError MakeListOfFiles(std::vector<String> &files, const String &asset_dir, const std::regex &regex)
{
    for (FindFile ff = FindFile::Open(asset_dir, regex, true, false, 0);
        !ff.AtEnd(); ff.Next())
    {
        String filename = ff.Current();
        files.push_back(filename);
    }
    return HError::None();
}

static bool DoesPaletteHaveAlpha(const Palette &pal)
{
    for (const auto &rgb : pal)
        if ((rgb.a != 0) && (rgb.a != 255))
            return true;
    return false;
}

static bool DoesBitmapHaveAlpha(const PixelBuffer &image, const Palette *pal)
{
    if (PixelFormatHasAlpha(image.GetFormat()))
        return true;

    if (PixelFormatIndexed(image.GetFormat()) && pal)
        return DoesPaletteHaveAlpha(*pal);

    return false;
}

// Forces transparency color to the palette index 0
// This must be done, because AGS (or rather Allegro 4) hardcodes transparent color index as 0.
static void NormalizePaletteTransparency(BitmapData &bm_data, Palette &dst_pal, const int pal_len = 256)
{
    assert(PixelFormatIndexed(bm_data.GetFormat()));
    // Determine actual transparency index in the palette
    int transparency_index = -1;
    for (int i = 0; i < pal_len; ++i)
    {
        if (dst_pal[i].a == 0)
        {
            transparency_index = i;
            break;
        }
    }

    // If there's no transparent color available in the palette,
    // then try to make a dummy transparent slot, so that we could
    // swap it with the color at original index 0.
    if (transparency_index < 0)
    {
        // If there's still free slots left in palette, then simply add a dummy slot in the end.
        if (pal_len < 256)
        {
            transparency_index = pal_len;
        }
        // If palette has all 256 slots, then look up for any slot
        // which is not used within the image, and turn it into "transparency".
        else
        {
            bool used[256] = { 0 };
            const uint8_t *px_ptr = bm_data.GetData();
            const uint8_t *px_end = px_ptr + bm_data.GetDataSize();
            for (; px_ptr != px_end; ++px_ptr)
            {
                used[*px_ptr] = true;
            }
            const bool *unused_at = std::find(used, used + 256, false);
            if (unused_at < used + 256)
                transparency_index = unused_at - used;
        }

        if (transparency_index >= 0)
        {
            dst_pal[transparency_index] = { 0,0,0,0 };
        }
    }

    // If transparent color is not 0, then swap found transparent index
    // with color at index 0
    if (transparency_index > 0)
    {
        RGB toswap = dst_pal[0];
        dst_pal[0] = dst_pal[transparency_index];
        dst_pal[transparency_index] = toswap;

        uint8_t *px_ptr = bm_data.GetData();
        uint8_t *px_end = px_ptr + bm_data.GetDataSize();
        for (; px_ptr != px_end; ++px_ptr)
        {
            if (*px_ptr == 0)
                *px_ptr = static_cast<uint8_t>(transparency_index);
            else if (*px_ptr == static_cast<uint8_t>(transparency_index))
                *px_ptr = 0;
        }
    }
}

// Converts imported image's palette to the native AGS format
static void ConvertPaletteToGameFormat(BitmapData &bm_data, const int game_color_depth,
    const Palette &src_pal, Palette &dst_pal, bool normalize_trans)
{
    assert(PixelFormatIndexed(bm_data.GetFormat()));
    // FIXME: currently we do not have means to get used palette slots here.
    // for that we'd need to pass palette as a struct with "slot count" field,
    // and also be able to read that in Image::LoadImage().
    const int pal_len = 256;
    // Copy palette, fixing colors if necessary
    for (int i = 0; i < 256; ++i)
    {
        if (i >= pal_len)
        {
            // BMP files can have an arbitrary palette size, fill any
            // missing colours with transparent black
            dst_pal[i].r = 0;
            dst_pal[i].g = 0;
            dst_pal[i].b = 0;
            dst_pal[i].a = 0;
        }
        else if (game_color_depth == 1)
        {
            // allegro palette is 0-63
            dst_pal[i].r = src_pal[i].r / 4;
            dst_pal[i].g = src_pal[i].g / 4;
            dst_pal[i].b = src_pal[i].b / 4;
            dst_pal[i].a = 255; // opaque
        }
        else
        {
            dst_pal[i].r = src_pal[i].r;
            dst_pal[i].g = src_pal[i].g;
            dst_pal[i].b = src_pal[i].b;
            dst_pal[i].a = src_pal[i].a;
        }
    }

    if (normalize_trans)
    {
        NormalizePaletteTransparency(bm_data, dst_pal, pal_len);
    }
}

template <typename T> T makecol_frompal_opaque(const RGB &c);
template <> uint16_t makecol_frompal_opaque(const RGB &c) { return makecol16(c.r, c.g, c.b); }
template <> uint32_t makecol_frompal_opaque(const RGB &c) { return makeacol32(c.r, c.g, c.b, 255); }

template <typename PxType>
static void Convert8BitToHiColorImpl(const uint8_t *src_buf, const uint8_t *src_end, const Palette &src_pal,
    uint8_t *dst_buf, uint32_t src_width, size_t src_stride, size_t dst_stride)
{
    for (; src_buf < src_end; src_buf += src_stride, dst_buf += dst_stride)
    {
        PxType *dst_ptr = reinterpret_cast<PxType*>(dst_buf);
        for (const uint8_t *src_ptr = src_buf; src_ptr < src_buf + src_width; ++src_ptr, ++dst_ptr)
        {
            *dst_ptr = makecol_frompal_opaque<PxType>(src_pal[*src_ptr]);
        }
    }
}

template <typename PxType>
static void Convert8BitToHiColorImpl(const uint8_t *src_buf, const uint8_t *src_end, const Palette &src_pal,
    uint8_t *dst_buf, uint32_t src_width, size_t src_stride, size_t dst_stride, const uint32_t maskcolor, const uint32_t safe_magenta)
{
    for (; src_buf < src_end; src_buf += src_stride, dst_buf += dst_stride)
    {
        PxType *dst_ptr = reinterpret_cast<PxType*>(dst_buf);
        for (const uint8_t *src_ptr = src_buf; src_ptr < src_buf + src_width; ++src_ptr, ++dst_ptr)
        {
            const int px = *src_ptr;
            const PxType color = makecol_frompal_opaque<PxType>(src_pal[px]);
            if (px == 0)
                *dst_ptr = maskcolor;
            else if (color == maskcolor) // replace magenta with close match
                *dst_ptr = safe_magenta;
            else
                *dst_ptr = color;
        }
    }
}

// Converts 8-bit pixel data with RGB palette to either 16-bit or 32-bit buffer.
// TODO: this should be a part of PixelOp::CopyConvert
// TODO: rewrite as a template function, having respective integer type as pixel size.
static void Convert8BitToHiColor(const BitmapData &src, const Palette &src_pal, PixelBuffer &dst, const bool keep_transparency)
{
    assert(dst.GetBytesPerPixel() == 16 || dst.GetBytesPerPixel() == 32);
    const int dst_depth = PixelFormatToPixelBits(dst.GetFormat());

    if (keep_transparency)
    {
        const uint32_t maskcolor = GetDefaultMaskColor(dst.GetFormat());
        // define a safe magenta color to use to preserve opacity in colors that match maskcolor
        // manually compose to use the full palette instead of allegro 0-63 restricted one (???)
        const uint32_t safe_magenta = (dst_depth == 16)
            ? makecol_depth(dst_depth, 255, 4, 255)  // 16 bit
            : makecol_depth(dst_depth, 255, 1, 255); // 24-32 bit

        if (dst_depth == 16)
        {
            Convert8BitToHiColorImpl<uint16_t>(src.GetData(), src.GetData() + src.GetStride() * src.GetHeight(), src_pal,
                dst.GetData(), src.GetWidth(), src.GetStride(), dst.GetStride(),
                maskcolor, safe_magenta);
        }
        else 
        {
            Convert8BitToHiColorImpl<uint32_t>(src.GetData(), src.GetData() + src.GetStride() * src.GetHeight(), src_pal,
                dst.GetData(), src.GetWidth(), src.GetStride(), dst.GetStride(),
                maskcolor, safe_magenta);
        }
    }
    else
    {
        if (dst_depth == 16)
        {
            Convert8BitToHiColorImpl<uint16_t>(src.GetData(), src.GetData() + src.GetStride() * src.GetHeight(), src_pal,
                dst.GetData(), src.GetWidth(), src.GetStride(), dst.GetStride());
        }
        else
        {
            Convert8BitToHiColorImpl<uint32_t>(src.GetData(), src.GetData() + src.GetStride() * src.GetHeight(), src_pal,
                dst.GetData(), src.GetWidth(), src.GetStride(), dst.GetStride());
        }
    }
}

// Converts 8-bit pixel data with ARGB palette to 32-bit buffer.
// TODO: this should be a part of PixelOp::CopyConvert
static void Convert8BitARGBTo32(const BitmapData &src, const Palette &src_pal, PixelBuffer &dst)
{
    assert(dst.GetBytesPerPixel() == 32);
    const uint8_t *src_buf = src.GetData();
    const uint8_t *src_end = src.GetData() + src.GetStride() * src.GetHeight();
    uint32_t *dst_buf = reinterpret_cast<uint32_t*>(dst.GetData());
    const uint32_t src_width = src.GetWidth();
    const size_t src_stride = src.GetStride();
    const size_t dst_stride = dst.GetStride();
    for (; src_buf < src_end; src_buf += src_stride, dst_buf += dst_stride)
    {
        uint32_t *dst_ptr = dst_buf;
        for (const uint8_t *src_ptr = src_buf; src_ptr < src_buf + src_width; ++src_ptr, ++dst_ptr)
        {
            const RGB &pal_color = src_pal[*src_ptr];
            *dst_ptr = makeacol32(pal_color.r, pal_color.g, pal_color.b, pal_color.a);
        }
    }
}

template <typename SrcPxType, typename DstPxType>
static void FixMaskColorImpl(const uint8_t *src_buf, const uint8_t *src_end,
    uint8_t *dst_buf, uint32_t src_width, size_t src_stride, size_t dst_stride,
    const uint32_t src_maskcolor, const uint32_t dst_maskcolor, const uint32_t safe_color)
{
    for (; src_buf < src_end; src_buf += src_stride, dst_buf += dst_stride)
    {
        DstPxType *dst_ptr = reinterpret_cast<DstPxType*>(dst_buf);
        for (const SrcPxType *src_line = reinterpret_cast<const SrcPxType*>(src_buf), *src_ptr = src_line;
            src_ptr < src_line + src_width; ++src_ptr, ++dst_ptr)
        {
            const SrcPxType src_color = *src_ptr;
            const DstPxType dst_color = *dst_ptr;
            if (src_color == src_maskcolor)
                *dst_ptr = dst_maskcolor;
            else if (dst_color == dst_maskcolor) // replace mask color with close match
                *dst_ptr = safe_color;
            // don't change other colors here
        }
    }
}

// Converts between two standard non-indexed formats (16 <-> 32 bit),
// optionally convert mask color from src to dest format, otherwise mask color *may* become normal color.
// TODO: this should be a part of PixelOp::CopyConvert
static void ConvertAndFixMaskColor(const BitmapData &src, PixelBuffer &dst, const bool keep_transparency)
{
    // FIXME: we do this two-step now, but ideally all of this should be done by PixelOp::CopyConvert.
    PixelOp::CopyConvert(src, dst, dst.GetFormat());

    // Idea is this: if we were asked to keep transparency, this means
    // that we need to keep all mask pixels from src to become mask pixels in dst,
    // but *also* any non-mask src pixel that ended up equal to the dst mask color
    // must be fixed to be the nearest non-mask color.
    if (keep_transparency)
    {
        const int src_depth = src.GetColorDepth();
        const int dst_depth = src.GetColorDepth();
        if (src_depth == 16 && dst_depth == 32)
        {
            FixMaskColorImpl<uint16_t, uint32_t>(src.GetData(), src.GetData() + src.GetHeight() * src.GetStride(), dst.GetData(),
                src.GetWidth(), src.GetStride(), dst.GetStride(),
                GetDefaultMaskColor(src.GetFormat()), GetDefaultMaskColor(dst.GetFormat()), replacement_mask_color(dst.GetColorDepth(), GetDefaultMaskColor(dst.GetFormat())));
        }
        else if (src_depth == 32 && dst_depth == 16)
        {
            FixMaskColorImpl<uint32_t, uint16_t>(src.GetData(), src.GetData() + src.GetHeight() * src.GetStride(), dst.GetData(),
                src.GetWidth(), src.GetStride(), dst.GetStride(),
                GetDefaultMaskColor(src.GetFormat()), GetDefaultMaskColor(dst.GetFormat()), replacement_mask_color(dst.GetColorDepth(), GetDefaultMaskColor(dst.GetFormat())));
        }
        else
        {
            assert(false);
        }
    }
}

// Takes source buffer and returns dst buffer with either:
// - a moved source buffer, when no changes whatsoever were necessary.
// - a moved and modified source buffer with the same pixel format, when only colors need to be replaces.
// - new pixel data, when full format conversion was necessary.
static HError ConvertToGameCompatible(PixelBuffer &src, PixelBuffer &dst, const Palette *src_pal,
    Palette *dst_pal, const int game_color_depth, const bool fix_color_depth,
    const bool import_alpha, const bool keep_transparency, const bool fix_pal)
{
    assert((src.GetColorDepth() > 8) || (src_pal && dst_pal));
    // First we unpack the pixels from the src pixel buffer to a buffer,
    // into the one of the standard formats compatible with our bitmap library.
    const int src_depth = src.GetColorDepth();
    int dst_depth = 0;
    switch (src.GetFormat())
    {
    case PixelFormat::kPxFmt_Indexed1:
    case PixelFormat::kPxFmt_Indexed4:    
    case PixelFormat::kPxFmt_Indexed8:
        dst_depth = 8; // convert any indexed to 8-bit
        break;
    case PixelFormat::kPxFmt_R5G5B5:
    case PixelFormat::kPxFmt_R5G6B5:
        dst_depth = 16; // convert to 16-bit
        break;
    case PixelFormat::kPxFmt_R8G8B8:
    case PixelFormat::kPxFmt_A8R8G8B8:
        dst_depth = 32; // convert to 32-bit
        break;
    default:
        return new Error(String::FromFormat("Unsupported pixel format: %s", PixelFormatName(src.GetFormat())));
    }

    if ((game_color_depth == 8) && (src_depth > 8))
    {
        new Error(String::FromFormat("Cannot import a hi-colour or true-colour image into a 256-colour game."));
    }

    // Convert pixels to the format, supported by the bitmap library.
    // This is necessary e.g. in case of 1-bit or 4-bit images that must be
    // converted to a standard 8-bit image.
    PixelBuffer compat_buf;
    if (src_depth != dst_depth)
    {
        if (!PixelOp::CopyConvert(src, compat_buf, ColorDepthToPixelFormat(dst_depth)))
        {
            new Error(String::FromFormat("Failed to convert an input image into the compatible game format."));
        }
    }

    // Which buffer to use as a source further?
    PixelBuffer *use_buf = compat_buf ? &compat_buf : &src;

    // Second step: prepare the image for the game,
    // following sprite specs and game's default color depth.
    // For indexed images: convert loaded palette to the library format.
    if ((src_depth <= 8) && src_pal && dst_pal)
    {
        ConvertPaletteToGameFormat(*use_buf, game_color_depth, *src_pal, *dst_pal, keep_transparency && fix_pal);
    }

    // Now to upgrade bitmap to the game's color depth, if necessary.
    const int final_depth = game_color_depth;
    const bool need_fix_color_depth = (dst_depth != final_depth) && (fix_color_depth);
    PixelBuffer final_buf;
    if (need_fix_color_depth)
    {
        final_buf = PixelBuffer(src.GetWidth(), src.GetHeight(), ColorDepthToPixelFormat(final_depth));

        if ((dst_depth == 8) && (final_depth > 8))
        {
            if (import_alpha && (final_depth == 32))
                Convert8BitARGBTo32(*use_buf, *dst_pal, final_buf);
            else
                Convert8BitToHiColor(*use_buf, *dst_pal, final_buf, keep_transparency);
        }
        else
        {
            ConvertAndFixMaskColor(*use_buf, final_buf, keep_transparency);
        }

        use_buf = &final_buf;
    }
    
    // Move output data to the destination buffer (this may be src buffer too!)
    dst = std::move(*use_buf);
    return HError::None();
}

// Removes all transparency pixels (change them to a close non-trnasparent colour)
// TODO: should be a part of PixelOp namespace
static void RemoveTransparency(BitmapData &bm_data, const int transcol)
{
    // FIXME: this replacement is kind of non-sensical for arbitrary image; by the looks of its code
    // is purposed for 8-bit paletted image, where color 0 is replaced with color 16 which is also "black".
    // Then, for hi-res color depth, IIRC there was a function in Allegro called something like
    // "best_color_match" or similar, which may be of better use here, instead of "transcol - 1".
    int r_color;
    if (transcol == 0)
        r_color = 16;
    else
        r_color = transcol - 1;

    for (int y = 0; y < bm_data.GetHeight(); ++y)
    {
        for (int x = 0; x < bm_data.GetWidth(); ++x)
        {
            if (bm_data.GetPixel(x, y) == transcol)
                bm_data.SetPixel(x, y, r_color);
        }
    }
}

// FIXME: this function apparently does about the same as FixMaskColorImpl
// find a way to reorganize this mess, and reuse code more.
static void MakeColorTransparent(BitmapData &bm_data, const Palette &src_pal, int src_color_depth, const int transcol)
{
    const int maskcolor = GetDefaultMaskColor(bm_data.GetFormat());
    int r_color = 16;
    if (bm_data.GetColorDepth() > 8)
    {
        if (src_color_depth == 8)
            r_color = makecol_depth(bm_data.GetColorDepth(), src_pal[0].r, src_pal[0].g, src_pal[0].b);
        else
            r_color = 0;
    }
    // swap all transparent pixels with index 0 pixels (???)
    for (int y = 0; y < bm_data.GetHeight(); ++y)
    {
        for (int x = 0; x < bm_data.GetWidth(); ++x)
        {
            if (bm_data.GetPixel(x, y) == transcol)
                bm_data.SetPixel(x, y, maskcolor);
            else if (bm_data.GetPixel(x, y) == maskcolor)
                bm_data.SetPixel(x, y, r_color);
        }
    }
}

// Adjusts sprite's transparency using the chosen method
static void SortOutTransparency(BitmapData &bm_data, const SpriteImportTransparency sprite_trans, int trans_index,
    const Palette &src_pal, const int src_color_depth, int &transcol)
{
    if (sprite_trans == kSpriteImport_LeaveAsIs)
    {
        transcol = GetDefaultMaskColor(bm_data.GetFormat());
        return;
    }

    if (sprite_trans == kSpriteImport_NoTransparency)
    {
        transcol = GetDefaultMaskColor(bm_data.GetFormat());
        RemoveTransparency(bm_data, transcol);
        return;
    }

    const int dst_depth = bm_data.GetColorDepth();
    switch (sprite_trans)
    {
    case kSpriteImport_TopLeft:
        transcol = bm_data.GetPixel(0, 0);
        break;
    case kSpriteImport_BottomLeft:
        transcol = bm_data.GetPixel(0, (bm_data.GetHeight()) - 1);
        break;
    case kSpriteImport_TopRight:
        transcol = bm_data.GetPixel((bm_data.GetWidth()) - 1, 0);
        break;
    case kSpriteImport_BottomRight:
        transcol = bm_data.GetPixel((bm_data.GetWidth()) - 1, (bm_data.GetHeight()) - 1);
        break;
    case kSpriteImport_PaletteIndex:
        assert(trans_index >= 0 && trans_index < 256);
        trans_index = (trans_index >= 0 && trans_index < 256) ? trans_index : 0;
        if (src_color_depth == 8)
        {
            if (dst_depth == 8)
                transcol = trans_index;
            else if (trans_index == 0)
                // on conversion slot 0 was replaced by a standard mask color
                transcol = GetDefaultMaskColor(bm_data.GetFormat());
            else
                transcol = makecol_depth(bm_data.GetColorDepth(), src_pal[trans_index].r, src_pal[trans_index].g, src_pal[trans_index].b);
        }
        else
        {
            transcol = GetDefaultMaskColor(bm_data.GetFormat());
        }
        break;
    case kSpriteImport_PaletteIndex0:
    default:
        transcol = GetDefaultMaskColor(bm_data.GetFormat());
        break;
    }

    MakeColorTransparent(bm_data, src_pal, src_color_depth, transcol);
}

// Adjusts 8-bit sprite's palette
static void SortOutPalette(BitmapData &bm_data, Palette &pal, const Palette &game_pal,
    const std::array<PaletteColourType, PAL_SIZE> &paluses, bool use_bg_slots, int transcol)
{
    if (transcol != 0)
        pal[transcol] = pal[0];

    PaletteOp::SetRGB(pal, 0, 0, 0, 0); // set index 0 to black
    Palette dst_pal;
    for (int i = 0; i < PAL_SIZE; ++i)
    {
        if (use_bg_slots) // use either slots
            dst_pal[i] = game_pal[i];
        else if (paluses[i] == kPaletteColourType_Background) // else ignore background slots
            PaletteOp::SetRGB(dst_pal, i, 0, 0, 0);
        else
            dst_pal[i] = game_pal[i];
    }
    PaletteOp::Remap(bm_data, pal, dst_pal);
}

void MergePalettes(Palette &dest_pal, const Palette &game_pal, const Palette &room_pal, const std::array<PaletteColourType, PAL_SIZE> &pal_uses)
{
    std::copy(game_pal.begin(), game_pal.end(), dest_pal.begin());
    for (size_t i = 0; i < PAL_SIZE; ++i)
        if (pal_uses[i] == kPaletteColourType_Background)
            dest_pal[i] = room_pal[i];
}

HError ConvertSpriteForGame(PixelBuffer &image, Palette *pal,
    PixelBuffer &dst_image, const GameColorSettings &game_color_opt, const RoomPaletteCache &room_cache, const SpriteData &sprite)
{
    const int game_color_depth = static_cast<int>(game_color_opt.ColorDepth) * 8;
    // Safety check: if requested alpha channel, test if bitmap contains one
    const bool alpha_channel = sprite.ImportAlphaChannel && (game_color_depth == 32) && DoesBitmapHaveAlpha(image, pal);
    const int src_color_depth = image.GetColorDepth();

    Palette img_pal_buf;
    const SpriteImportTransparency sprite_trans = sprite.TransparentColour;
    const int trans_color = sprite.TransparentColourIndex;
    const bool keep_trans = (sprite_trans != kSpriteImport_NoTransparency);
    const bool fix_palette = (sprite_trans != kSpriteImport_PaletteIndex0) && (sprite_trans != kSpriteImport_PaletteIndex);
    const bool remap_colors = PixelFormatIndexed(image.GetFormat()) && (sprite.RemapToGamePalette || sprite.RemapToRoomPalette);
    const bool use_room_pal = sprite.RemapToRoomPalette;
    PixelBuffer final_buf;
    HError err = ConvertToGameCompatible(image, final_buf, pal, &img_pal_buf, game_color_depth, true /* fix color depth */,
            alpha_channel, keep_trans, fix_palette);
    if (!err)
        return err;

    // Prior to transparency conversion, deal with 32-bit alpha channel:
    // either convert zero-alpha pixels to standard mask color, or to opaque color
    if (final_buf.GetColorDepth() == 32)
    {
        if (alpha_channel)
        {
            PixelOp::ReplaceAlphaWithRGBMask(final_buf);
        }
        else
        {
            PixelOp::MakeOpaqueSkipMask(final_buf);
        }
    }

    int transcol = 0;
    SortOutTransparency(final_buf, sprite_trans, trans_color, img_pal_buf, src_color_depth, transcol);
    if (game_color_depth == 8)
    {
        if (remap_colors)
        {
            // Merge game and room palettes
            Palette *room_pal = nullptr;
            if (sprite.ColoursLockedToRoom >= 0)
            {
                auto it_pal = room_cache.find(sprite.ColoursLockedToRoom);
                if (it_pal != room_cache.end())
                    room_pal = it_pal->second.get();
            }

            if (room_pal)
            {
                Palette combined_pal;
                MergePalettes(combined_pal, game_color_opt.Palette, *room_pal, game_color_opt.PalUses);
                SortOutPalette(final_buf, img_pal_buf, combined_pal, game_color_opt.PalUses, use_room_pal, transcol);
            }
            else
            {
                SortOutPalette(final_buf, img_pal_buf, game_color_opt.Palette, game_color_opt.PalUses, use_room_pal, transcol);
            }
        }
    }

    dst_image = std::move(final_buf);
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
