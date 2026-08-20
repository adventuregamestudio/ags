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
#include "gtest/gtest.h"
#include "data/sprite_utils.h"
#include "util/memory_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

//-----------------------------------------------------------------------------
// HELPER FUNCTIONS
//-----------------------------------------------------------------------------

GameColorSettings GetDefColorSet32()
{
    GameColorSettings set;
    set.ColorDepth = kGameColorDepth_TrueColor;
    return set;
}

GameColorSettings GetDefColorSet16()
{
    GameColorSettings set;
    set.ColorDepth = kGameColorDepth_HighColor;
    return set;
}

GameColorSettings GetDefColorSet8()
{
    GameColorSettings set;
    set.ColorDepth = kGameColorDepth_Palette;
    return set;
}

SpriteData GetDefImport(int width, int height, int color_depth)
{
    SpriteData data;
    data.AlphaChannel = color_depth == 32;
    data.ColorDepth = color_depth;
    data.Height = height;
    data.ImportAlphaChannel = color_depth == 32;
    data.ImportHeight = height;
    data.ImportWidth = width;
    return data;
}

PixelBuffer GetDefPixels8(Palette &pal)
{
    // WARNING: remember that Allegro 4's palette operations
    // have 64-color-range precision, so all colors must be multiple of 4
    std::fill(pal.begin(), pal.end(), RGB{0,0,0,0});
    // palette 0 is a default transparent slot
    pal[0] = { 0, 0, 0 };
    pal[1] = { 255, 0, 0 };
    pal[2] = { 0, 255, 0 };
    pal[3] = { 0, 0, 255 };
    pal[4] = { 255, 255, 0 };
    pal[5] = { 255, 0, 255 };
    pal[6] = { 0, 255, 255 };
    pal[7] = { 255, 255, 255 };
    pal[8] = { 128, 0, 0 };
    pal[9] = { 0, 128, 0 };
    pal[10] = { 0, 0, 128 };
    pal[11] = { 128, 128, 0 };
    pal[12] = { 128, 0, 128 };
    pal[13] = { 0, 128, 128 };
    pal[14] = { 128, 128, 128 };
    pal[15] = { 64, 64, 64 };
    pal[16] = { 32, 32, 32 };

    PixelBuffer image(4, 4, kPxFmt_Indexed8);
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image.GetData());
    pixels[0] = 1; // top-left
    pixels[1] = 2;
    pixels[2] = 3;
    pixels[3] = 4; // top-right
    pixels[4] = 5;
    pixels[5] = 0;
    pixels[6] = 7;
    pixels[7] = 8;
    pixels[8] = 9;
    pixels[9] = 0;
    pixels[10] = 11;
    pixels[11] = 12;
    pixels[12] = 13; // bottom-left
    pixels[13] = 14;
    pixels[14] = 15;
    pixels[15] = 16; // bottom-right
    return image;
}

PixelBuffer GetDefPixels16()
{
    PixelBuffer image(4, 4, kPxFmt_R5G6B5);
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image.GetData());
    pixels[0] = 0xFFFF; // top-left
    pixels[1] = 0x5555;
    pixels[2] = MASK_COLOR_16;
    pixels[3] = 0xEEEE; // top-right
    pixels[4] = 0xFFFF;
    pixels[5] = 0xEEEE;
    pixels[6] = 0xDDDD;
    pixels[7] = MASK_COLOR_16;
    pixels[8] = 0xDDDD;
    pixels[9] = 0xEEEE;
    pixels[10] = 0x7777;
    pixels[11] = 0xFFFF;
    pixels[12] = 0xDDDD; // bottom-left
    pixels[13] = 0xAAAA;
    pixels[14] = MASK_COLOR_16;
    pixels[15] = 0xCCCC; // bottom-right
    return image;
}

PixelBuffer GetDefPixels32()
{
    PixelBuffer image(4, 4, kPxFmt_A8R8G8B8);
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image.GetData());
    pixels[0] = 0xFFFFFFFF; // top-left
    pixels[1] = 0x00555555;
    pixels[2] = MASK_COLOR_32;
    pixels[3] = 0xFFEEEEEE; // top-right
    pixels[4] = 0xFFFFFFFF;
    pixels[5] = 0xFFEEEEEE;
    pixels[6] = 0xFFDDDDDD;
    pixels[7] = MASK_COLOR_32;
    pixels[8] = 0xFFDDDDDD;
    pixels[9] = 0xFFEEEEEE;
    pixels[10] = 0x00777777;
    pixels[11] = 0xFFFFFFFF;
    pixels[12] = 0xFFDDDDDD; // bottom-left
    pixels[13] = 0x00AAAAAA;
    pixels[14] = MASK_COLOR_32;
    pixels[15] = 0xFFCCCCCC; // bottom-right
    return image;
}

void PrintPixelData8(const BitmapData &bm_data1, const BitmapData &bm_data2)
{
    for (size_t i = 0; i < bm_data1.GetDataSize(); ++i)
    {
        printf("--- %3d : %3d\n", ((uint8_t*)bm_data1.GetData())[i], ((uint8_t*)bm_data2.GetData())[i]);
    }
}

void PrintPixelData16(const BitmapData &bm_data1, const BitmapData &bm_data2)
{
    for (size_t i = 0; i < bm_data1.GetDataSize() / 2; ++i)
    {
        printf("--- %8x : %8x\n", ((uint16_t*)bm_data1.GetData())[i], ((uint16_t*)bm_data2.GetData())[i]);
    }
}

void PrintPixelData32(const BitmapData &bm_data1, const BitmapData &bm_data2)
{
    for (size_t i = 0; i < bm_data1.GetDataSize() / 4; ++i)
    {
        printf("--- %8x : %8x\n", ((uint32_t*)bm_data1.GetData())[i], ((uint32_t*)bm_data2.GetData())[i]);
    }
}

template<typename PxType>
void TestPixelData4x4(const BitmapData &bm_expect, const BitmapData &bm_result)
{
    const PxType *expect_p = reinterpret_cast<const PxType *>(bm_expect.GetData());
    const PxType *result_p = reinterpret_cast<const PxType *>(bm_result.GetData());
    for (size_t i = 0; i < 16; ++i)
        EXPECT_EQ(expect_p[i], result_p[i]) << "[" << i << "]: expect " << expect_p[i] << " (0x" << std::hex << expect_p[i] << ") got " << std::dec << result_p[i] << std::hex << " (0x" << result_p[i] << ")";
}

PixelBuffer ImportDefault8Sprite(const GameColorSettings *use_color_set, const RoomPaletteCache *room_pal_cache, int room_to_use,
    GameColorDepth convert_to_depth = kGameColorDepth_Palette, SpriteImportTransparency trans = kSpriteImport_LeaveAsIs)
{
    Palette pal;
    PixelBuffer image = GetDefPixels8(pal);
    // If we are going to "import" this 8-bit sprite into 8-bit destination
    // then we must convert its palette to Allegro 4's 16-bit RGB palette,
    // otherwise some color algorithms will break (e.g. bestfit_color).
    if (convert_to_depth == kGameColorDepth_Palette)
    {
        for (auto &p : pal)
        {
            p.r /= 4;
            p.g /= 4;
            p.b /= 4;
            p.a /= 4;
        }
    }

    PixelBuffer dest;
    GameColorSettings set;
    if (use_color_set)
        set = *use_color_set;
    else
        set = GetDefColorSet8();
    set.ColorDepth = convert_to_depth;
    SpriteData sprite = GetDefImport(4, 4, 8);
    if (use_color_set)
        sprite.RemapToGamePalette = true;
    if (room_pal_cache)
    {
        sprite.RemapToRoomPalette = true;
        sprite.ColoursLockedToRoom = room_to_use;
    }
    sprite.TransparentColour = trans;
    const auto *src_data = image.GetData();
    HError err = ConvertSpriteForGame(image, &pal, dest, set, room_pal_cache, sprite);
    EXPECT_TRUE(err);
    EXPECT_TRUE(dest.GetData() != nullptr);
    if (convert_to_depth == kGameColorDepth_Palette)
        EXPECT_EQ(src_data, dest.GetData()); // no format conversion, moved original buffer
    else
        EXPECT_NE(src_data, dest.GetData()); // format conversion, created new buffer
    return dest;
}

PixelBuffer ImportDefault8Sprite(GameColorDepth convert_to_depth = kGameColorDepth_Palette, SpriteImportTransparency trans = kSpriteImport_LeaveAsIs)
{
    return ImportDefault8Sprite(nullptr, nullptr, -1, convert_to_depth, trans);
}

PixelBuffer ImportDefault16Sprite(GameColorDepth convert_to_depth = kGameColorDepth_HighColor, SpriteImportTransparency trans = kSpriteImport_LeaveAsIs)
{
    auto image = GetDefPixels16();
    PixelBuffer dest;
    auto set = GetDefColorSet16();
    set.ColorDepth = convert_to_depth;
    SpriteData sprite = GetDefImport(4, 4, 16);
    sprite.TransparentColour = trans;
    const auto *src_data = image.GetData();
    HError err = ConvertSpriteForGame(image, nullptr, dest, set, nullptr, sprite);
    EXPECT_TRUE(err);
    EXPECT_TRUE(dest.GetData() != nullptr);
    if (convert_to_depth == kGameColorDepth_HighColor)
        EXPECT_EQ(src_data, dest.GetData()); // no format conversion, moved original buffer
    else
        EXPECT_NE(src_data, dest.GetData()); // format conversion, created new buffer
    return dest;
}

PixelBuffer ImportDefault32Sprite(GameColorDepth convert_to_depth = kGameColorDepth_TrueColor, SpriteImportTransparency trans = kSpriteImport_LeaveAsIs)
{
    auto image = GetDefPixels32();
    PixelBuffer dest;
    auto set = GetDefColorSet32();
    set.ColorDepth = convert_to_depth;
    SpriteData sprite = GetDefImport(4, 4, 32);
    sprite.TransparentColour = trans;
    const auto *src_data = image.GetData();
    HError err = ConvertSpriteForGame(image, nullptr, dest, set, nullptr, sprite);
    EXPECT_TRUE(err);
    EXPECT_TRUE(dest.GetData() != nullptr);
    if (convert_to_depth == kGameColorDepth_TrueColor)
        EXPECT_EQ(src_data, dest.GetData()); // no format conversion, moved original buffer
    else
        EXPECT_NE(src_data, dest.GetData()); // format conversion, created new buffer
    return dest;
}

//-----------------------------------------------------------------------------
// TESTS
//-----------------------------------------------------------------------------

TEST(SpriteImport, DirectImportNoConversion8)
{
    auto dest = ImportDefault8Sprite();
    EXPECT_EQ(4 * 4, dest.GetDataSize());
}

TEST(SpriteImport, DirectImportNoConversion16)
{
    auto dest = ImportDefault16Sprite();
    EXPECT_EQ(4 * 4 * 2, dest.GetDataSize());
}

TEST(SpriteImport, DirectImportNoConversion32)
{
    auto dest = ImportDefault32Sprite();
    EXPECT_EQ(4 * 4 * 4, dest.GetDataSize());
}

TEST(SpriteImport, DirectImportConvert8To16)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_HighColor);
    EXPECT_EQ(kPxFmt_R5G6B5, dest.GetFormat());
    EXPECT_EQ(4 * 4 * 2, dest.GetDataSize());

    auto image2 = PixelBuffer(4, 4, kPxFmt_R5G6B5);
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    pixels[0] = makecol16(255, 0, 0);
    pixels[1] = makecol16(0, 255, 0);
    pixels[2] = makecol16(0, 0, 255);
    pixels[3] = makecol16(255, 255, 0);
    pixels[4] = 0xF83F; //(255, 0, 255) => (255, 4, 255) prevent becoming MASK_COLOR_16 (see Convert8BitToHiColor)
    pixels[5] = MASK_COLOR_16;
    pixels[6] = makecol16(255, 255, 255);
    pixels[7] = makecol16(128, 0, 0);
    pixels[8] = makecol16(0, 128, 0);
    pixels[9] = MASK_COLOR_16;
    pixels[10] = makecol16(128, 128, 0);
    pixels[11] = makecol16(128, 0, 128);
    pixels[12] = makecol16(0, 128, 128);
    pixels[13] = makecol16(128, 128, 128);
    pixels[14] = makecol16(64, 64, 64);
    pixels[15] = makecol16(32, 32, 32);
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, DirectImportConvert8To32)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_TrueColor);
    EXPECT_EQ(kPxFmt_A8R8G8B8, dest.GetFormat());
    EXPECT_EQ(4 * 4 * 4, dest.GetDataSize());

    auto image2 = PixelBuffer(4, 4, kPxFmt_A8R8G8B8);
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[0] = 0xFFFF0000;
    pixels[1] = 0xFF00FF00;
    pixels[2] = 0xFF0000FF;
    pixels[3] = 0xFFFFFF00;
    pixels[4] = 0xFFFF00FF;
    pixels[5] = MASK_COLOR_32;
    pixels[6] = 0xFFFFFFFF;
    pixels[7] = 0xFF800000;
    pixels[8] = 0xFF008000;
    pixels[9] = MASK_COLOR_32;
    pixels[10] = 0xFF808000;
    pixels[11] = 0xFF800080;
    pixels[12] = 0xFF008080;
    pixels[13] = 0xFF808080;
    pixels[14] = 0xFF404040;
    pixels[15] = 0xFF202020;
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, DirectImportConvert16To32)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_TrueColor);
    EXPECT_EQ(kPxFmt_A8R8G8B8, dest.GetFormat());
    EXPECT_EQ(4 * 4 * 4, dest.GetDataSize());
}

TEST(SpriteImport, DirectImportConvert32To16)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_HighColor);
    EXPECT_EQ(kPxFmt_R5G6B5, dest.GetFormat());
    EXPECT_EQ(4 * 4 * 2, dest.GetDataSize());
}

TEST(SpriteImport, Transparency8LeaveAsIs)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_Palette, kSpriteImport_LeaveAsIs);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Transparency16LeaveAsIs)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_HighColor, kSpriteImport_LeaveAsIs);

    auto image2 = GetDefPixels16();
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, Transparency32LeaveAsIs)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_TrueColor, kSpriteImport_LeaveAsIs);

    auto image2 = GetDefPixels32();
    // Even though the import spec is LeaveAsIs, the conversion replaces
    // any zero-alpha pixels with the mask color (0x00FF00FF)
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[1]  = MASK_COLOR_32; // 0x00555555 -> MASK_COLOR_32
    pixels[10] = MASK_COLOR_32; // 0x00777777 -> MASK_COLOR_32
    pixels[13] = MASK_COLOR_32; // 0x00AAAAAA -> MASK_COLOR_32
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, Transparency8NoTransparency)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_Palette, kSpriteImport_NoTransparency);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    // Replace color 0 with 16 (alternate black)
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[5] = 16;
    pixels[9] = 16;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Transparency16NoTransparency)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_HighColor, kSpriteImport_NoTransparency);

    auto image2 = GetDefPixels16();
    // No transparency import replaces mask color pixels with the nearest color
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    pixels[2]  = MASK_COLOR_16 - 1;
    pixels[7]  = MASK_COLOR_16 - 1;
    pixels[14] = MASK_COLOR_16 - 1;
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, Transparency32NoTransparency)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_TrueColor, kSpriteImport_NoTransparency);

    auto image2 = GetDefPixels32();
    // Zero alpha pixels are first replaced with the mask color
    // No transparency import replaces any mask color pixels with the nearest color
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[1]  = MASK_COLOR_32 - 1; // 0x00555555 -> MASK_COLOR_32 -> NO TRANS
    pixels[2]  = MASK_COLOR_32 - 1; // MASK_COLOR_32 -> NO TRANS
    pixels[7]  = MASK_COLOR_32 - 1; // MASK_COLOR_32 -> NO TRANS
    pixels[10] = MASK_COLOR_32 - 1; // 0x00777777 -> MASK_COLOR_32 -> NO TRANS
    pixels[13] = MASK_COLOR_32 - 1; // 0x00AAAAAA -> MASK_COLOR_32 -> NO TRANS
    pixels[14] = MASK_COLOR_32 - 1; // MASK_COLOR_32 -> NO TRANS
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, Transparency8TopLeft)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_Palette, kSpriteImport_TopLeft);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    // Replace top-left pixel values with 0
    // Replace color 0 with 16 (alternate black)
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[0] = 0;
    pixels[5] = 16;
    pixels[9] = 16;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Transparency16TopLeft)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_HighColor, kSpriteImport_TopLeft);

    auto image2 = GetDefPixels16();
    // Pick top-left pixel's value and replace all pixels with this value to the mask color
    // Existing mask color pixels are replaced with zero-alpha black
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    pixels[0] = MASK_COLOR_16;
    pixels[2] = 0;
    pixels[4] = MASK_COLOR_16;
    pixels[7] = 0;
    pixels[11] = MASK_COLOR_16;
    pixels[14] = 0;
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, Transparency32TopLeft)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_TrueColor, kSpriteImport_TopLeft);

    auto image2 = GetDefPixels32();
    // Pick top-left pixel's value and replace all pixels with this value to the mask color;
    // Existing mask color pixels (and zero-alpha pixels) are replaced with zero-alpha black
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[0] = MASK_COLOR_32;
    pixels[1] = 0;
    pixels[2] = 0;
    pixels[4] = MASK_COLOR_32;
    pixels[7] = 0;
    pixels[10] = 0;
    pixels[11] = MASK_COLOR_32;
    pixels[13] = 0;
    pixels[14] = 0;
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, Transparency8TopRight)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_Palette, kSpriteImport_TopRight);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    // Replace top-right pixel values with 0
    // Replace color 0 with 16 (alternate black)
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[3] = 0;
    pixels[5] = 16;
    pixels[9] = 16;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Transparency16TopRight)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_HighColor, kSpriteImport_TopRight);

    auto image2 = GetDefPixels16();
    // Pick top-right pixel's value and replace all pixels with this value to the mask color
    // Existing mask color pixels are replaced with zero-alpha black
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    pixels[2] = 0;
    pixels[3] = MASK_COLOR_16; // top-right
    pixels[5] = MASK_COLOR_16;
    pixels[7] = 0;
    pixels[9] = MASK_COLOR_16;
    pixels[14] = 0;
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, Transparency32TopRight)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_TrueColor, kSpriteImport_TopRight);

    auto image2 = GetDefPixels32();
    // Pick top-right pixel's value and replace all pixels with this value to the mask color;
    // Existing mask color pixels (and zero-alpha pixels) are replaced with zero-alpha black
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[1] = 0;
    pixels[2] = 0;
    pixels[3] = MASK_COLOR_32; // top-right
    pixels[5] = MASK_COLOR_32;
    pixels[7] = 0;
    pixels[9] = MASK_COLOR_32;
    pixels[10] = 0;
    pixels[13] = 0;
    pixels[14] = 0;
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, Transparency8BottomLeft)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_Palette, kSpriteImport_BottomLeft);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    // Replace bottom-left pixel values with 0
    // Replace color 0 with 16 (alternate black)
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[5] = 16;
    pixels[9] = 16;
    pixels[12] = 0;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Transparency16BottomLeft)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_HighColor, kSpriteImport_BottomLeft);

    auto image2 = GetDefPixels16();
    // Pick top-right pixel's value and replace all pixels with this value to the mask color
    // Existing mask color pixels are replaced with zero-alpha black
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    pixels[2] = 0;
    pixels[6] = MASK_COLOR_16;
    pixels[7] = 0;
    pixels[8] = MASK_COLOR_16;
    pixels[12] = MASK_COLOR_16; // bottom-left
    pixels[14] = 0;
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, Transparency32BottomLeft)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_TrueColor, kSpriteImport_BottomLeft);

    auto image2 = GetDefPixels32();
    // Pick top-right pixel's value and replace all pixels with this value to the mask color;
    // Existing mask color pixels (and zero-alpha pixels) are replaced with zero-alpha black
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[1] = 0;
    pixels[2] = 0;
    pixels[6] = MASK_COLOR_32;
    pixels[7] = 0;
    pixels[8] = MASK_COLOR_32;
    pixels[10] = 0;
    pixels[12] = MASK_COLOR_32;
    pixels[13] = 0;
    pixels[14] = 0;
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, Transparency8BottomRight)
{
    auto dest = ImportDefault8Sprite(kGameColorDepth_Palette, kSpriteImport_BottomRight);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    // Replace bottom-left pixel values with 0
    // Replace color 0 with 16 (alternate black)
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[5] = 16;
    pixels[9] = 16;
    pixels[15] = 0;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Transparency16BottomRight)
{
    auto dest = ImportDefault16Sprite(kGameColorDepth_HighColor, kSpriteImport_BottomRight);

    auto image2 = GetDefPixels16();
    // Pick top-right pixel's value and replace all pixels with this value to the mask color
    // Existing mask color pixels are replaced with zero-alpha black
    uint16_t *pixels = reinterpret_cast<uint16_t*>(image2.GetData());
    pixels[2] = 0;
    pixels[7] = 0;
    pixels[14] = 0;
    pixels[15] = MASK_COLOR_16;
    /* PrintPixelData16(dest, image2); */

    TestPixelData4x4<uint16_t>(image2, dest);
}

TEST(SpriteImport, Transparency32BottomRight)
{
    auto dest = ImportDefault32Sprite(kGameColorDepth_TrueColor, kSpriteImport_BottomRight);

    auto image2 = GetDefPixels32();
    // Pick top-right pixel's value and replace all pixels with this value to the mask color;
    // Existing mask color pixels (and zero-alpha pixels) are replaced with zero-alpha black
    uint32_t *pixels = reinterpret_cast<uint32_t*>(image2.GetData());
    pixels[1] = 0;
    pixels[2] = 0;
    pixels[7] = 0;
    pixels[10] = 0;
    pixels[13] = 0;
    pixels[14] = 0;
    pixels[15] = MASK_COLOR_32;
    /* PrintPixelData32(dest, image2); */

    TestPixelData4x4<uint32_t>(image2, dest);
}

TEST(SpriteImport, Import8RemapToGamePalette)
{
    // WARNING: remember that Allegro 4's palette operations
    // have 64-color-range precision, so all colors must be multiple of 4
    GameColorSettings color_set;
    color_set.Palette[0] = { 32, 32, 32 };
    color_set.Palette[1] = { 64, 64, 64 };
    color_set.Palette[2] = { 128, 128, 128 };
    color_set.Palette[3] = { 0, 128, 128 };
    color_set.Palette[4] = { 128, 0, 128 };
    color_set.Palette[5] = { 128, 128, 0 };
    color_set.Palette[6] = { 0, 0, 128 };
    color_set.Palette[7] = { 0, 128, 0 };
    color_set.Palette[8] = { 128, 0, 0 };
    color_set.Palette[9] = { 255, 255, 255 };
    color_set.Palette[10] = { 0, 255, 255 };
    color_set.Palette[11] = { 255, 0, 255 };
    color_set.Palette[12] = { 255, 255, 0 };
    color_set.Palette[13] = { 0, 0, 255 };
    color_set.Palette[14] = { 0, 255, 0 };
    color_set.Palette[15] = { 255, 0, 0 };
    color_set.Palette[16] = { 0, 0, 0 };

    // Convert to Allegro 4's 16-bit RGB palette
    for (auto &p : color_set.Palette)
    {
        p.r /= 4;
        p.g /= 4;
        p.b /= 4;
        p.a /= 4;
    }

    auto dest = ImportDefault8Sprite(&color_set, nullptr, -1, kGameColorDepth_Palette, kSpriteImport_LeaveAsIs);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[0] = 15;
    pixels[1] = 14;
    pixels[2] = 13;
    pixels[3] = 12;
    pixels[4] = 11;
    pixels[5] = 0;  // transparency is always at 0
    pixels[6] = 9;
    pixels[7] = 8;
    pixels[8] = 7;
    pixels[9] = 0;  // transparency is always at 0
    pixels[10] = 5;
    pixels[11] = 4;
    pixels[12] = 3;
    pixels[13] = 2;
    pixels[14] = 1;
    pixels[15] = 1;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}

TEST(SpriteImport, Import8RemapToRoomPalette)
{
    // WARNING: remember that Allegro 4's palette operations
    // have 64-color-range precision, so all colors must be multiple of 4
    GameColorSettings color_set;
    color_set.Palette[0] = { 32, 32, 32 };
    color_set.Palette[1] = { 64, 64, 64 };
    color_set.Palette[2] = { 128, 128, 128 };
    color_set.Palette[3] = { 0, 128, 128 };
    color_set.Palette[4] = { 128, 0, 128 };
    color_set.Palette[5] = { 128, 128, 0 };
    color_set.Palette[6] = { 0, 0, 128 };
    color_set.Palette[7] = { 0, 128, 0 };
    color_set.Palette[8] = { 128, 0, 0 };
    color_set.Palette[9] = { 255, 255, 255 };
    color_set.Palette[10] = { 0, 255, 255 };
    color_set.Palette[11] = { 255, 0, 255 };
    color_set.Palette[12] = { 255, 255, 0 };
    color_set.Palette[13] = { 0, 0, 255 };
    color_set.Palette[14] = { 0, 255, 0 };
    color_set.Palette[15] = { 255, 0, 0 };
    color_set.Palette[16] = { 0, 0, 0 };
    // Give some color slots to the room background
    for (int i = 10; i < PAL_SIZE; ++i)
        color_set.PalUses[i] = kPaletteColourType_Background;

    // Convert to Allegro 4's 16-bit RGB palette
    for (auto &p : color_set.Palette)
    {
        p.r /= 4;
        p.g /= 4;
        p.b /= 4;
        p.a /= 4;
    }

    Palette room_palette;
    std::fill(room_palette.begin(), room_palette.end(), RGB{0,0,0,0});
    room_palette[10] = { 0, 0, 0 };
    room_palette[11] = { 255, 0, 0 };
    room_palette[12] = { 0, 255, 0 };
    room_palette[13] = { 0, 0, 255 };
    room_palette[14] = { 255, 255, 0 };
    room_palette[15] = { 255, 0, 255 };
    room_palette[16] = { 0, 255, 255 };

    // Convert to Allegro 4's 16-bit RGB palette
    for (auto &p : room_palette)
    {
        p.r /= 4;
        p.g /= 4;
        p.b /= 4;
        p.a /= 4;
    }

    RoomPaletteCache cache;
    cache[5] = std::make_unique<Palette>(std::move(room_palette));

    auto dest = ImportDefault8Sprite(&color_set, &cache, 5, kGameColorDepth_Palette, kSpriteImport_LeaveAsIs);

    Palette pal2;
    auto image2 = GetDefPixels8(pal2);
    uint8_t *pixels = reinterpret_cast<uint8_t*>(image2.GetData());
    pixels[0] = 11;
    pixels[1] = 12;
    pixels[2] = 13;
    pixels[3] = 14;
    pixels[4] = 15;
    pixels[5] = 0;  // transparency is always at 0
    pixels[6] = 9;
    pixels[7] = 8;
    pixels[8] = 7;
    pixels[9] = 0;  // transparency is always at 0
    pixels[10] = 5;
    pixels[11] = 4;
    pixels[12] = 3;
    pixels[13] = 2;
    pixels[14] = 1;
    pixels[15] = 1;
    /* PrintPixelData8(dest, image2); */

    TestPixelData4x4<uint8_t>(image2, dest);
}
